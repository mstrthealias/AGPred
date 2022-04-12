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



constexpr bool DEBUG_PRINT_SHAPES = false;
constexpr bool DEBUG_PRINT_PREDICTIONS = false;

constexpr bool MODEL_OUTPUT_LOGITS = false;
constexpr real_t PREDICTION_THRESHHOLD_LONG = 0.51f;
constexpr real_t PREDICTION_THRESHHOLD_SHORT = 0.51f;


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

real_t mk_vec_sigmoid(real_t x) {
    /// Specifically, `y = 1 / (1 + exp(-x))`.
    return 1.0 / (1 + std::exp(-x));
}

real_t mk_vec_prediction(real_t p, real_t t) {
    /// Specifically, `y = 1 / (1 + exp(-x))`.
    return p > t ? 1 : 0;
}

const auto vec_sigmoid = xt::vectorize(mk_vec_sigmoid);
const auto vec_prediction = xt::vectorize(mk_vec_prediction);

const std::string MODEL_DIR = "pyfiles/model_long.bc";
const std::string MODEL_DIR_SHORT = "pyfiles/model_short.bc";
//const std::string MODEL_DIR_LONG = "pyfiles/model_long";

std::unique_ptr<tensorflow::SavedModelBundle> bundle;
std::unique_ptr<tensorflow::SignatureDef> sig_def;
/*const auto inputs_features = sig_def.inputs().at("features");
const auto inputs_candles = sig_def.inputs().at("candles");
const auto inputs_trades = sig_def.inputs().at("trades");
const auto inputs_quotes = sig_def.inputs().at("quotes");
const auto outputs_long_low = sig_def.outputs().at((LongLowAlgo::SINGLE_BEAR_OUTPUT ? "short_high" : "profit_low"));
const auto outputs_short_high = sig_def.outputs().at((LongLowAlgo::DUAL_OUTPUT || LongLowAlgo::SINGLE_BEAR_OUTPUT ? "short_high" : "profit_low"));  // TODO use macro? */
/*struct ModelSig {
    tensorflow::SignatureDef sig_def;
    tensorflow::TensorInfo inputs_features;
    tensorflow::TensorInfo inputs_candles;
    tensorflow::TensorInfo inputs_trades;
    tensorflow::TensorInfo inputs_quotes;
    tensorflow::TensorInfo outputs_long_low;
    tensorflow::TensorInfo outputs_short_high;
};*/

static tensorflow::Tensor t_features(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS - 61 - 1 }));  // 105 - 61
static tensorflow::Tensor t_candles(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_INTERVALS, NUM_TIMESTEMPS, 61 }));
static tensorflow::Tensor t_trades(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_TRADES, 5 }));
static tensorflow::Tensor t_quotes(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_QUOTES, 7 }));

static std::vector<tensorflow::Tensor> out;


void LongLowAlgo::initStatics() {
    bundle = std::make_unique<tensorflow::SavedModelBundle>();
    sig_def = std::make_unique<tensorflow::SignatureDef>(setup_bundle(*bundle));
}


const tensorflow::SignatureDef agpred::setup_bundle(tensorflow::SavedModelBundle& bundle)
{
    // setup a TF session/bundle for running the SavedModel

    tensorflow::SessionOptions session_options;  // fe. set CPU:0/GPU:0
    tensorflow::RunOptions run_options;  // https://www.tensorflow.org/api_docs/python/tf/compat/v1/RunOptions

    const auto model_dir = (LongLowAlgo::SINGLE_BEAR_OUTPUT ? MODEL_DIR_SHORT : MODEL_DIR);
    if (!tensorflow::MaybeSavedModelDirectory(model_dir)) {
        std::cerr << "Invalid model directory: " << model_dir << std::endl;
        throw std::logic_error("invalid model directory");
    }

    // TF_CHECK_OK takes the status and checks whether it works.
    auto status = tensorflow::LoadSavedModel(session_options,
        run_options,
        model_dir,
        { tensorflow::kSavedModelTagServe },  // (tag_constants)
        &bundle);
    if (!status.ok()) {
        std::cerr << "LoadSaveModel failed: " << status;
        throw std::runtime_error("LoadSavedModel: " + status.error_message());
    }
    std::cerr << "Loaded: " << model_dir << std::endl;

    const auto& sig_map = bundle.meta_graph_def.signature_def();
    //print_keys(sig_map);
    return sig_map.at("serving_default");
}



LongLowAlgo::LongLowAlgo(const std::string& name, bool inverse)
    : AlgoBase(name),
        inverse_(inverse)
{
}



int LongLowAlgo::calc_prediction_signal(const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const
{
    // skip if nan/infinite values found
    auto nan_sum = xt::sum(!xt::isfinite(raw))(0) + xt::sum(!xt::isfinite(processed))(0);
    if (nan_sum > 0) {
        std::cout << "SKIP calc_prediction_signal() " << nan_sum << " nan/infinite value(s) found" << std::endl;
        return 0;
    }

    //const auto atr = raw(0, 0, ColPos::In::alt3);
    //std::cout << "ATR: " << atr << std::endl;  //(17)

    // vector to save predictions/outputs 
    out.clear();

    // copy inputs/data into tensors...
    {
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "processed.shape: " << processed.shape() << std::endl;

        auto x_numeric = xt::view(processed, xt::newaxis(), xt::all(), xt::all(), xt::range(1, NUM_COLUMNS - 61));
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_numeric.shape: " << x_numeric.shape() << std::endl;


        /*{
            auto x_raw = xt::xarray<real_t>(xt::view(raw, 0, xt::all(), ColPos::In::ask));
            std::cout << "raw.shape: " << raw.shape() << std::endl;
            std::cout << "x_raw_ask: " << x_raw << std::endl;
            std::cout << "x_raw_ask.shape: " << x_raw.shape() << std::endl;
        }
        {
            for (ptrdiff_t i = ColPos::In::volume; i <= ColPos::In::bid_low; ++i) {

                auto x_numeric_1min = xt::view(processed, Timeframes::_1min, xt::all(), i);
                //std::cout << "x_numeric_1min[" << i << "].shape: " << x_numeric_1min.shape() << std::endl;
                
                auto x_min = xt::amin(x_numeric_1min)(0);
                if (x_min <= 0) {
                    //std::cout << "x_numeric_1min " << x_numeric_1min << std::endl;
                    //std::cout << "x_numeric_1min_cpy " << x_numeric_1min_cpy << std::endl;

                    std::cout << "x_numeric_1min[" << i << "] MIN: " << x_min << std::endl;
                }
            }

            for (ptrdiff_t i = 1; i < ColPos::NUM_COLS; ++i) {

                auto x_numeric_1min = xt::view(processed, Timeframes::_1min, xt::all(), i);
                //std::cout << "x_numeric_1min[" << i << "].shape: " << x_numeric_1min.shape() << std::endl;
                xt::xarray<real_t> x_numeric_1min_cpy = vec_cleanup_float_errs(x_numeric_1min);
                auto x_sum = xt::sum(x_numeric_1min)(0);
                auto x_sum_cpy = xt::sum(x_numeric_1min_cpy)(0);
                if (!dbl_equal(x_sum, x_sum_cpy)) {
                    //std::cout << "x_numeric_1min " << x_numeric_1min << std::endl;
                    //std::cout << "x_numeric_1min_cpy " << x_numeric_1min_cpy << std::endl;
                    std::cout << "x_numeric_1min[" << i << "] SUM: " << x_sum << " " << x_sum_cpy << std::endl;
                }
            }
        }*/


        //xt::xarray<real_t> x_numeric_cpy = vec_cleanup_float_errs(x_numeric);  // TODO do this elsewhere
        //std::cout << "numeric SUM: " << xt::sum(x_numeric) << " " << xt::sum(x_numeric_cpy) << std::endl;
        //std::copy(x_numeric_cpy.cbegin(), x_numeric_cpy.cend(), t_features.flat<float>().data());
        std::copy(x_numeric.cbegin(), x_numeric.cend(), t_features.flat<float>().data());

        auto x_candles = xt::view(processed, xt::newaxis(), xt::all(), xt::all(), xt::range(NUM_COLUMNS - 61, NUM_COLUMNS));
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_candles.shape: " << x_candles.shape() << std::endl;

        std::copy(x_candles.cbegin(), x_candles.cend(), t_candles.flat<float>().data());
    }
    {
        // note: most recent trades at bottom (order asc)
        trades.toTensor(cur_trades);
        //cur_trades;  //(15000, 6)
        //if constexpr(DEBUG_PRINT_SHAPES) 
        //    std::cout << "cur_trades.shape: " << cur_trades.shape() << std::endl;

        // skip if nan/infinite values found
        auto invalid_sum = xt::sum(!xt::isfinite(cur_trades))(0);
        if (invalid_sum > 0) {
            std::cout << "SKIP calc_prediction_signal() " << invalid_sum << " nan/infinite trade value(s) found" << std::endl;
            return 0;
        }

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

        // skip if nan/infinite values found
        auto invalid_sum = xt::sum(!xt::isfinite(cur_quotes))(0);
        if (invalid_sum > 0) {
            std::cout << "SKIP calc_prediction_signal() " << invalid_sum << " nan/infinite quote value(s) found" << std::endl;
            return 0;
        }

        auto x_quotes = xt::view(cur_quotes, xt::newaxis(), xt::all(), xt::range(1, 8));  //(1, 15000, 7)
        if constexpr (DEBUG_PRINT_SHAPES)
            std::cout << "x_quotes.shape: " << x_quotes.shape() << std::endl;
        std::copy(x_quotes.cbegin(), x_quotes.cend(), t_quotes.flat<float>().data());
    }

    const auto inputs_features = sig_def->inputs().at("features");
    const auto inputs_candles = sig_def->inputs().at("candles");
    const auto inputs_trades = sig_def->inputs().at("trades");
    const auto inputs_quotes = sig_def->inputs().at("quotes");
    const auto outputs_long_low = sig_def->outputs().at((LongLowAlgo::SINGLE_BEAR_OUTPUT ? "short_high" : "profit_low"));
    const auto outputs_short_high = sig_def->outputs().at((LongLowAlgo::DUAL_OUTPUT || LongLowAlgo::SINGLE_BEAR_OUTPUT ? "short_high" : "profit_low"));  // TODO use macro? 

    std::vector<std::string> output_tensor_names;
    if constexpr (DUAL_OUTPUT) 
    {
        output_tensor_names = { outputs_long_low.name(), outputs_short_high.name() };
    }
    else if constexpr (SINGLE_BEAR_OUTPUT)
    {
        output_tensor_names = { outputs_short_high.name() };
    }
    else
    {
        output_tensor_names = { outputs_long_low.name() };
    }

    TF_CHECK_OK(bundle->session->Run(
        { 
            { inputs_features.name(), t_features }, 
            { inputs_candles.name(), t_candles },
            { inputs_trades.name(), t_trades },
            { inputs_quotes.name(), t_quotes } },
        output_tensor_names,
        {},
        &out));

    if constexpr (false && DEBUG_PRINT_PREDICTIONS) 
    {
        if constexpr (DUAL_OUTPUT || SINGLE_BEAR_OUTPUT)
        {
            std::cout << "short_high: " << out[(SINGLE_BEAR_OUTPUT ? 0 : 1)].DebugString() << std::endl;
        }
        if constexpr (!SINGLE_BEAR_OUTPUT)
        {
            std::cout << "long_low: " << out[0].DebugString() << std::endl;
        }
    }

    auto dim = get_tensor_shape(out[0])[0];
    xt::xarray<float> predictions_long = xt::zeros<float>({ dim });
    xt::xarray<float> predictions_short = xt::zeros<float>({ dim });
    if constexpr (!SINGLE_BEAR_OUTPUT)
    {
        auto& res = out[0];
        auto shape = get_tensor_shape(res);

        assert(shape.size() == 2);
        assert(shape[0] == 1);
        //std::cout << "long_low SHAPE SIZE: " << shape.size() << std::endl;
        //std::cout << "long_low SHAPE[0]: " << shape[0] << std::endl;

        // we only care about the first dimension of shape
        for (int row = 0; row < shape[0]; ++row) {
            predictions_long(row) = res.tensor<float, 2>()(row, 0);
        }

        if constexpr (MODEL_OUTPUT_LOGITS) {
            // sigmoid: convert to probability
            predictions_long = vec_sigmoid(predictions_long);
        }
        // convert probability to 0/1
        predictions_long = vec_prediction(predictions_long, PREDICTION_THRESHHOLD_LONG);
        if constexpr (DEBUG_PRINT_PREDICTIONS) {
            std::cout << "long_low predictions: " << predictions_long << std::endl;
        }
    }
    if constexpr (DUAL_OUTPUT || SINGLE_BEAR_OUTPUT)
    {
        auto& res = out[(SINGLE_BEAR_OUTPUT ? 0 : 1)];
        auto shape = get_tensor_shape(res);

        assert(shape.size() == 2);
        assert(shape[0] == 1);
        //std::cout << "short_high SHAPE SIZE: " << shape.size() << std::endl;
        //std::cout << "short_high SHAPE[0]: " << shape[0] << std::endl;

        // we only care about the first dimension of shape
        for (int row = 0; row < shape[0]; ++row) {
            predictions_short(row) = res.tensor<float, 2>()(row, 0);
        }

        if constexpr (MODEL_OUTPUT_LOGITS) {
            // sigmoid: convert to probability
            predictions_short = vec_sigmoid(predictions_short);
        }
        // convert probability to 0/1
        predictions_short = vec_prediction(predictions_short, PREDICTION_THRESHHOLD_SHORT);
        if constexpr (DEBUG_PRINT_PREDICTIONS) {
            std::cout << "short_high predictions: " << predictions_short << std::endl;
        }
    }

    if constexpr (DUAL_OUTPUT)
    {
        if (predictions_long(0) > 0 && predictions_short(0) > 0)
            return 0;
        return predictions_long(0) > 0 ? 1
            : predictions_short(0) > 0 ? -1
            : 0;
    }
    else if constexpr (SINGLE_BEAR_OUTPUT)
    {
        return predictions_short(0) > 0 ? -1
            : 0;
    }
    else
    {
        return predictions_long(0) > 0 ? 1
            : 0;
    }
}

