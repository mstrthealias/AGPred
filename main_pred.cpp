#define NOMINMAX

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

#include "src/common.h"


static const int IMG_SIZE = 36828;
static const int NUM_SAMPLES = 1024;  // 10240;

static const int SHAPE_1 = 7;
static const int SHAPE_2 = 17;
static const int SHAPE_3 = 100;

static const int SRC_SAMPLES = 2126;
static const int SRC_COLS = 6;


real_t mk_vec_sigmoid(real_t x) {
    /// Specifically, `y = 1 / (1 + exp(-x))`.
    return 1 / (1 + std::exp(-x));
}

real_t mk_vec_prediction(real_t p, real_t t) {
    /// Specifically, `y = 1 / (1 + exp(-x))`.
    return p > t ? 1 : 0;
}

const auto vec_sigmoid = xt::vectorize(mk_vec_sigmoid);
const auto vec_prediction = xt::vectorize(mk_vec_prediction);


int load_npy_img(const std::string& filename, tensorflow::Tensor& t_features, tensorflow::Tensor& t_candles, tensorflow::Tensor& t_trades, tensorflow::Tensor& t_quotes) {
    std::cout << "Loading " << filename << "..." << std::endl;

    std::ifstream file_stream(filename, std::ios::binary);
    if (!file_stream.is_open())
        throw std::runtime_error("Unable to open/read file");  // no throw just to catch yar

    std::cout << "Loaded " << filename << "." << std::endl;

    {
        auto x_features = xt::load_npy<float>(file_stream);  //(1024, 7, 17, 100)
        std::cout << "x_features.shape: " << x_features.shape() << std::endl;

        auto x_numeric = xt::view(x_features, xt::all(), xt::all(), xt::all(), xt::range(0, 39));
        std::cout << "x_numeric.shape: " << x_numeric.shape() << std::endl;
        std::copy(x_numeric.cbegin(), x_numeric.cend(), t_features.flat<float>().data());

        auto x_candles = xt::view(x_features, xt::all(), xt::all(), xt::all(), xt::range(39, 100));
        std::cout << "x_candles.shape: " << x_candles.shape() << std::endl;
        std::copy(x_candles.cbegin(), x_candles.cend(), t_candles.flat<float>().data());

        /*std::cout << "Tensor[0,0,0,0]=" << t_features.tensor<float, 4>()(0, 0, 0, 0) << "; data[0,0,0,0]=" << x_features(0, 0, 0, 0) << std::endl;
        std::cout << "Tensor[1,1,1,1]=" << t_features.tensor<float, 4>()(1, 1, 1, 1) << "; data[1,1,1,1]=" << x_features(1, 1, 1, 1) << std::endl;
        std::cout << "Tensor[1000,1,1,1]=" << t_features.tensor<float, 4>()(1000, 1, 1, 1) << "; data[1000,1,1,1]=" << x_features(1000, 1, 1, 1) << std::endl;
        std::cout << "Tensor[1000,7,10,5]=" << t_features.tensor<float, 4>()(1000, 7, 10, 5) << "; data[1000,7,10,5]=" << x_features(1000, 7, 10, 5) << std::endl;*/
    }
    {
        auto x_trades = xt::load_npy<float>(file_stream);  //(1024, 15000, 5)
        std::cout << "x_trades.shape: " << x_trades.shape() << std::endl;
        std::copy(x_trades.cbegin(), x_trades.cend(), t_trades.flat<float>().data());
    }
    {
        auto x_quotes = xt::load_npy<float>(file_stream);  //(1024, 15000, 7)
        std::cout << "x_quotes.shape: " << x_quotes.shape() << std::endl;
        std::copy(x_quotes.cbegin(), x_quotes.cend(), t_quotes.flat<float>().data());
    }
    {
        auto y_outputs = xt::load_npy<float>(file_stream);  //(1024, 8)
        std::cout << "y_outputs.shape: " << y_outputs.shape() << std::endl;
    }

    std::cout << "Tensors ready." << std::endl;

    return 0;

    //std::cout << "Setting up input tensor..." << std::endl;
    ///*tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({NUM_SAMPLES, IMG_SIZE}));
    //for (int i = 0; i < NUM_SAMPLES; i++)
    //    for (int j = 0; j < IMG_SIZE; j++)
    //        t.tensor<float, 2>()(i, j) = data(i, j);*/
    //for (int i = 0; i < NUM_SAMPLES; i++)
    //    for (int j = 0; j < SHAPE_1; j++)
    //        for (int k = 0; k < SHAPE_2; k++)
    //            for (int l = 0; l < SHAPE_3; l++)
    //                t.tensor<float, 4>()(i, j, k, l) = data(i, j, k, l);
    //std::cout << "Tensor:" << t.shape() << std::endl;
    //std::cout << "Tensor:" << t.dtype() << std::endl;
    //return t_features;
}

std::vector<int> get_tensor_shape(const tensorflow::Tensor& tensor)
{
    std::vector<int> shape;
    auto num_dimensions = tensor.shape().dims();
    for(int i=0; i < num_dimensions; i++) {
        shape.push_back(tensor.shape().dim_size(i));
    }
    return shape;
}

template <typename M>
void print_keys(const M& sig_map) {
    for (auto const& p : sig_map) {
        std::cout << "key : " << p.first << std::endl;
    }
}

template <typename K, typename M>
bool assert_in(const K& k, const M& m) {
    return !(m.find(k) == m.end());
}

const std::string _input_name1 = "features";
const std::string _input_name2 = "candles";
const std::string _input_name3 = "trades";
const std::string _input_name4 = "quotes";
const std::string _output_name1 = "profit_low";
//const std::string _output_name2 = "short_high";


int main(int argc, char* argv[])
{
    // test_tf();

    // This is passed into LoadSavedModel to be populated.
    tensorflow::SavedModelBundle bundle;

    // From docs: "If 'target' is empty or unspecified, the local TensorFlow runtime
    // implementation will be used.  Otherwise, the TensorFlow engine
    // defined by 'target' will be used to perform all computations."
    tensorflow::SessionOptions session_options;

    // Run option flags here: https://www.tensorflow.org/api_docs/python/tf/compat/v1/RunOptions
    // We don't need any of these yet.
    tensorflow::RunOptions run_options;

    // Fills in this from a session run call
    std::vector<tensorflow::Tensor> out;

    tensorflow::string dir = "pyfiles/model";
    std::string npy_file = "pyfiles/EVAL.0.pkl";
    std::string prediction_npy_file = "pyfiles/predictions.npy";

    std::cout << "Found model: " << tensorflow::MaybeSavedModelDirectory(dir) << std::endl;
    // TF_CHECK_OK takes the status and checks whether it works.
    TF_CHECK_OK(tensorflow::LoadSavedModel(session_options,
        run_options,
        dir,
        // Refer to tag_constants. We just want to serve the model.
        { tensorflow::kSavedModelTagServe },
        &bundle));
    /*auto status = tensorflow::LoadSavedModel(session_options,
        run_options,
        dir,
        // Refer to tag_constants. We just want to serve the model.
        { tensorflow::kSavedModelTagServe },
        &bundle);
    if (!status.ok()) {
        std::cerr << "Failed: " << status;
        return -1;
    }*/

    auto sig_map = bundle.meta_graph_def.signature_def();

    // not sure why it's called this but upon running this for loop to check for keys we see it.
    print_keys(sig_map);
    std::string sig_def = "serving_default";
    auto model_def = sig_map.at(sig_def);

    auto inputs_features = model_def.inputs().at(_input_name1);
    auto input_name1 = inputs_features.name();

    auto inputs_candles = model_def.inputs().at(_input_name2);
    auto input_name2 = inputs_candles.name();

    auto inputs_trades = model_def.inputs().at(_input_name3);
    auto input_name3 = inputs_trades.name();

    auto inputs_quotes = model_def.inputs().at(_input_name4);
    auto input_name4 = inputs_quotes.name();

    auto outputs1 = model_def.outputs().at(_output_name1);
    auto output_name1 = outputs1.name();
    //auto outputs2 = model_def.outputs().at(_output_name2);
    //auto output_name2 = outputs2.name();

    tensorflow::Tensor t_features(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1024, 7, 17, 39 }));
    tensorflow::Tensor t_candles(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1024, 7, 17, 61 }));
    tensorflow::Tensor t_trades(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1024, 15000, 5 }));
    tensorflow::Tensor t_quotes(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1024, 15000, 7 }));

    int result = load_npy_img(npy_file, t_features, t_candles, t_trades, t_quotes);
    
    TF_CHECK_OK(bundle.session->Run({ {input_name1, t_features}, {input_name2, t_candles}, {input_name3, t_trades}, {input_name4, t_quotes} },
                                    { output_name1 },
                                    {},
                                    &out));
    std::cout << _output_name1 << ": " << out[0].DebugString() << std::endl;

    auto res = out[0];
    auto shape = get_tensor_shape(res);

    std::cout << "out[0] SHAPE SIZE: " << shape.size() << std::endl;
    std::cout << "out[0] SHAPE[0]: " << shape[0] << std::endl;

    /*{
        std::cout << _output_name2 << ": " << out[1].DebugString() << std::endl;

        auto res2 = out[1];
        auto shape2 = get_tensor_shape(res2);

        std::cout << "out[1] SHAPE SIZE: " << shape2.size() << std::endl;
        std::cout << "out[1] SHAPE[0]: " << shape2[0] << std::endl;
    }*/

    // we only care about the first dimension of shape
    xt::xarray<float> predictions = xt::zeros<float>({ shape[0] });
    for(int row = 0; row < shape[0]; row++) {
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
    predictions = vec_prediction(predictions, 0.51);
    //std::cout << "predictions: " << predictions << std::endl;

    xt::dump_npy(prediction_npy_file, predictions);

    return 0;
}


void tf_math_op() 
{
    /*{
#include <tensorflow/cc/ops/math_ops.h>
#include <tensorflow/cc/ops/array_ops.h>
#include <tensorflow/cc/client/client_session.h>

        tensorflow::Scope root = tensorflow::Scope::NewRootScope();

        auto a = tensorflow::ops::Placeholder(root, tensorflow::DT_FLOAT);

        tensorflow::ClientSession session(root);

        //std::vector<tensorflow::Output> res_outputs;
        std::vector<tensorflow::Tensor> res_outputs;

        /// Specifically, `y = 1 / (1 + exp(-x))`.

        auto sigmoid = tensorflow::ops::Sigmoid(root, res);

        tensorflow::Status s = session.Run({ {a, {1}} }, { sigmoid }, &res_outputs);
        if (!s.ok()) {
            std::cout << "Sigmoid error..." << std::endl;
        }

        std::cout << "res_sigmoid: " << res_outputs[0].DebugString() << std::endl;
    }*/
}
