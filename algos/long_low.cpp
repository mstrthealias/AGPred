#define NOMINMAX

#include "long_low.h"

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/core/public/session_options.h>
#include <tensorflow/core/framework/tensor.h>
#include <tensorflow/core/lib/io/path.h>

#include <xtensor/xarray.hpp>
#include <xtensor/xnpy.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xvectorize.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <cfloat>
#include <cmath>


using namespace agpred;


// TODO verify there are no memory issues in this translation unit...
//      Why? defining MODEL_DIR in this scope added new lines before the actual directory?


constexpr bool DEBUG_PRINT_SHAPES = false;
constexpr bool DEBUG_PRINT_PREDICTIONS = false;

constexpr double PREDICTION_THRESHHOLD = 0.51;


xtensor_quotes cur_quotes;
xtensor_trades cur_trades;


template <typename M>
void print_keys(const M& sig_map) {
    for (auto const& p : sig_map) {
        std::cout << "key : " << p.first << std::endl;
    }
}

std::vector<int> get_tensor_shape(const tensorflow::Tensor& tensor)
{
    std::vector<int> shape;
    auto num_dimensions = tensor.shape().dims();
    for (int i = 0; i < num_dimensions; i++) {
        shape.push_back(tensor.shape().dim_size(i));
    }
    return shape;
}

double mk_vec_sigmoid(double x) {
    /// Specifically, `y = 1 / (1 + exp(-x))`.
    return 1.0 / (1 + std::exp(-x));
}

double mk_vec_prediction(double p, double t) {
    /// Specifically, `y = 1 / (1 + exp(-x))`.
    return p > t ? 1 : 0;
}

const auto vec_sigmoid = xt::vectorize(mk_vec_sigmoid);
const auto vec_prediction = xt::vectorize(mk_vec_prediction);



const tensorflow::SignatureDef setup_bundle(tensorflow::SavedModelBundle& bundle)
{
    const std::string MODEL_DIR = "pyfiles/model";

    // setup a TF session/bundle for running the SavedModel

    tensorflow::SessionOptions session_options;  // fe. set CPU:0/GPU:0
    tensorflow::RunOptions run_options;  // https://www.tensorflow.org/api_docs/python/tf/compat/v1/RunOptions

    if (!tensorflow::MaybeSavedModelDirectory(MODEL_DIR)) {
        std::cerr << "Invalid model directory: " << MODEL_DIR << std::endl;
        throw std::logic_error("invalid model directory");
    }

    // TF_CHECK_OK takes the status and checks whether it works.
    auto status = tensorflow::LoadSavedModel(session_options,
        run_options,
        MODEL_DIR,
        { tensorflow::kSavedModelTagServe },  // (tag_constants)
        &bundle);
    if (!status.ok()) {
        std::cerr << "LoadSaveModel failed: " << status;
        throw std::runtime_error("LoadSavedModel: " + status.error_message());
    }

    const auto& sig_map = bundle.meta_graph_def.signature_def();
    //print_keys(sig_map);
    std::string sig_def = "serving_default";
    return sig_map.at(sig_def);
}


LongLowAlgo::LongLowAlgo(const std::string& name, bool inverse)
    : AlgoBase(name), 
        sig_def_(setup_bundle(bundle_)),
        inputs_features_(sig_def_.inputs().at("features")),
        inputs_candles_(sig_def_.inputs().at("candles")),
        inputs_trades_(sig_def_.inputs().at("trades")), 
        inputs_quotes_(sig_def_.inputs().at("quotes")),
        outputs_long_low_(sig_def_.outputs().at("profit_low")),
        inverse_(inverse)
{
}


int LongLowAlgo::calc_prediction_signal(const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const
{
    // vector to save predictions/outputs 
    std::vector<tensorflow::Tensor> out;

    // copy inputs/data into tensors...
    tensorflow::Tensor t_features(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, 7, 17, 39 }));
    tensorflow::Tensor t_candles(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, 7, 17, 61 }));
    tensorflow::Tensor t_trades(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, 15000, 5 }));
    tensorflow::Tensor t_quotes(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, 15000, 7 }));
    {
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "processed.shape: " << processed.shape() << std::endl;

        auto x_numeric = xt::view(processed, xt::newaxis(), xt::all(), xt::all(), xt::range(1, 40));
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_numeric.shape: " << x_numeric.shape() << std::endl;
        std::copy(x_numeric.cbegin(), x_numeric.cend(), t_features.flat<float>().data());

        auto x_candles = xt::view(processed, xt::newaxis(), xt::all(), xt::all(), xt::range(40, 101));
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_candles.shape: " << x_candles.shape() << std::endl;
        std::copy(x_candles.cbegin(), x_candles.cend(), t_candles.flat<float>().data());
    }
    {
        // note: most recent quotes at bottom (order asc)
        trades.toTensor(cur_trades);
        //cur_trades;  //(15000, 6)
        //if constexpr(DEBUG_PRINT_SHAPES) 
        //    std::cout << "cur_trades.shape: " << cur_trades.shape() << std::endl;
        auto x_trades = xt::view(cur_trades, xt::newaxis(), xt::all(), xt::range(1, 6));  //(1, 15000, 5)
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_trades.shape: " << x_trades.shape() << std::endl;
        std::copy(x_trades.cbegin(), x_trades.cend(), t_trades.flat<float>().data());
    }
    {
        // note: most recent quotes at bottom (order asc)
        quotes.toTensor(cur_quotes);
        //cur_quotes;  //(15000, 8)
        //if constexpr (DEBUG_PRINT_SHAPES)
        //    std::cout << "cur_quotes.shape: " << cur_quotes.shape() << std::endl;
        auto x_quotes = xt::view(cur_quotes, xt::newaxis(), xt::all(), xt::range(1, 8));  //(1, 15000, 7)
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_quotes.shape: " << x_quotes.shape() << std::endl;
        std::copy(x_quotes.cbegin(), x_quotes.cend(), t_quotes.flat<float>().data());
    }

    TF_CHECK_OK(bundle_.session->Run(
        { 
            { inputs_features_.name(), t_features }, 
            { inputs_candles_.name(), t_candles },
            { inputs_trades_.name(), t_trades },
            { inputs_quotes_.name(), t_quotes } },
        { outputs_long_low_.name() },
        {},
        &out));

    if constexpr (DEBUG_PRINT_PREDICTIONS)
        std::cout << "long_low: " << out[0].DebugString() << std::endl;

    auto& res = out[0];
    auto shape = get_tensor_shape(res);

    assert(shape.size() == 2);
    assert(shape[0] == 1);
    //std::cout << "long_low SHAPE SIZE: " << shape.size() << std::endl;
    //std::cout << "long_low SHAPE[0]: " << shape[0] << std::endl;

    // we only care about the first dimension of shape
    xt::xarray<float> predictions = xt::zeros<float>({ shape[0] });
    for (int row = 0; row < shape[0]; row++) {
        /*float max = FLT_MIN;
        int max_idx = -1;
        for(int col = 0; col < shape[1]; col++) {
            auto val = res.tensor<float, 2>()(row, col);
            if(max < val) {
                max_idx = col;
                max = val;
            }
        }
        predictions(row) = max_idx;*/
        predictions(row) = res.tensor<float, 2>()(row, 0);
    }

    // perform sigmoid and convert probability to 0/1
    predictions = vec_sigmoid(predictions);
    predictions = vec_prediction(predictions, PREDICTION_THRESHHOLD);

    if constexpr (DEBUG_PRINT_PREDICTIONS)
        std::cout << "long_low predictions: " << predictions << std::endl;

	return predictions(0) > 0 ? 1 : 0;
}

