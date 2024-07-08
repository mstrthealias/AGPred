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
//constexpr real_t PREDICTION_THRESHHOLD_LONG = 0.55f;
//constexpr real_t PREDICTION_THRESHHOLD_SHORT = 0.55f;
//constexpr real_t PREDICTION_THRESHHOLD_LONG3 = 0.91f;
//constexpr real_t PREDICTION_THRESHHOLD_SHORT3 = 0.91f;
constexpr real_t PREDICTION_THRESHHOLD_LONG = 0.61f;
constexpr real_t PREDICTION_THRESHHOLD_SHORT = 0.61f;
constexpr real_t PREDICTION_THRESHHOLD_LONG3 = 0.91f;
constexpr real_t PREDICTION_THRESHHOLD_SHORT3 = 0.91f;

constexpr timestamp_us_t X0_BLOCK_US = 5 * MIN_TO_US;
constexpr timestamp_us_t X3_BLOCK_US = 3 * MIN_TO_US;
constexpr timestamp_us_t X3_BOOST_US = 3 * MIN_TO_US;



timestamp_us_t TFModelAlgo::last_ts = 0;
timestamp_us_t TFModelAlgo::block_long_til_ts = 0;
timestamp_us_t TFModelAlgo::block_short_til_ts = 0;
timestamp_us_t TFModelAlgo::boost_long_til_ts = 0;
timestamp_us_t TFModelAlgo::boost_short_til_ts = 0;



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

const std::string MODEL_DIR = "pyfiles/model";
//const std::string MODEL_DIR = "pyfiles/model_long.bc";
const std::string MODEL_DIR_SHORT = "pyfiles/model_short.bc";
//const std::string MODEL_DIR_LONG = "pyfiles/model_long.bc";

std::unique_ptr<tensorflow::SavedModelBundle> bundle;
std::unique_ptr<tensorflow::SignatureDef> sig_def;

static tensorflow::Tensor t_features(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS - 61 - 1 }));  // 105 - 61
static tensorflow::Tensor t_candles(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_INTERVALS, NUM_TIMESTEMPS, 61 }));
static tensorflow::Tensor t_trades(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_TRADES, 5 }));
static tensorflow::Tensor t_quotes(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, NUM_QUOTES, 7 }));

static std::vector<tensorflow::Tensor> out;


void TFModelAlgo::initStatics() {
    bundle = std::make_unique<tensorflow::SavedModelBundle>();
    sig_def = std::make_unique<tensorflow::SignatureDef>(setup_bundle(*bundle));
}


const tensorflow::SignatureDef agpred::setup_bundle(tensorflow::SavedModelBundle& bundle)
{
    // setup a TF session/bundle for running the SavedModel

    tensorflow::SessionOptions session_options;  // fe. set CPU:0/GPU:0
    tensorflow::RunOptions run_options;  // https://www.tensorflow.org/api_docs/python/tf/compat/v1/RunOptions

    const auto model_dir = MODEL_DIR;
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




std::array<bool, TFModelAlgo::signal_size> TFModelAlgo::operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const
{
    auto ret_false = []() -> std::array<bool, signal_size> {
        if constexpr (signal_size > 3)
            return { false, false, false, false };
        else if constexpr (signal_size > 2)
            return { false, false, false };
        else if constexpr (signal_size > 1)
            return { false, false };
        else
            return { false };
    };

    // track timestamp of latest nbbo
    last_ts = snapshot.nbbo.timestamp;

    // skip if nan/infinite values found
    auto nan_sum = xt::sum(!xt::isfinite(raw))(0) + xt::sum(!xt::isfinite(processed))(0);
    if (nan_sum > 0) {
        std::cout << "SKIP calc_prediction_signal() " << nan_sum << " nan/infinite value(s) found" << std::endl;
        return ret_false();
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
            return ret_false();
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
            return ret_false();
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
    const auto outputs_long_low = sig_def->outputs().at("profit3");  // profit_low

    std::vector<std::string> output_tensor_names;
    if constexpr (signal_size > 3) {
        const auto outputs_short_high = sig_def->outputs().at("short_high");
        const auto outputs_long3 = sig_def->outputs().at("profit3");
        const auto outputs_short3 = sig_def->outputs().at("short3");
        output_tensor_names = {
            outputs_long_low.name(),
            outputs_short_high.name(),
            outputs_long3.name(),
            outputs_short3.name()
        };
    }
    else if constexpr (signal_size > 2) {
        const auto outputs_short_high = sig_def->outputs().at("short_high");
        const auto outputs_long3 = sig_def->outputs().at("profit3");
        output_tensor_names = {
            outputs_long_low.name(),
            outputs_short_high.name(),
            outputs_long3.name()
        };
    }
    else if constexpr (signal_size > 1) {
        const auto outputs_short_high = sig_def->outputs().at("short_high");
        output_tensor_names = {
            outputs_long_low.name(),
            outputs_short_high.name()
        };
    }
    else {
        output_tensor_names = {
            outputs_long_low.name()
        };
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
        std::cout << "long0: " << out[0].DebugString() << std::endl;
        std::cout << "short0: " << out[1].DebugString() << std::endl;
        std::cout << "long3: " << out[2].DebugString() << std::endl;
        std::cout << "short3: " << out[3].DebugString() << std::endl;
    }

    const auto dim = get_tensor_shape(out[0])[0];
    xt::xarray<float> predictions_l0 = xt::zeros<float>({ dim });
    xt::xarray<float> predictions_s0 = xt::zeros<float>({ dim });
    xt::xarray<float> predictions_l3 = xt::zeros<float>({ dim });
    xt::xarray<float> predictions_s3 = xt::zeros<float>({ dim });
    real_t l0_prob = 0.0f;
    real_t s0_prob = 0.0f;
    real_t l3_prob = 0.0f;
    real_t s3_prob = 0.0f;
    // long0
    {
        auto& res = out[0];
        auto shape = get_tensor_shape(res);

        assert(shape.size() == 2);
        assert(shape[0] == 1);
        //std::cout << "long_low SHAPE SIZE: " << shape.size() << std::endl;
        //std::cout << "long_low SHAPE[0]: " << shape[0] << std::endl;

        // we only care about the first dimension of shape
        for (int row = 0; row < shape[0]; ++row) {
            predictions_l0(row) = res.tensor<float, 2>()(row, 0);
        }

        if constexpr (MODEL_OUTPUT_LOGITS) {
            // sigmoid: convert to probability
            predictions_l0 = vec_sigmoid(predictions_l0);
        }
        if constexpr (DEBUG_PRINT_PREDICTIONS && signal_size <= 1) {
            std::cout << "long0 predictions: " << predictions_l0 << std::endl;
        }
        l0_prob = predictions_l0(0);

        // convert probability to 0/1
        predictions_l0 = vec_prediction(predictions_l0, PREDICTION_THRESHHOLD_LONG);
    }
    // short0
    if constexpr (signal_size > 1)
    {
        auto& res = out[1];
        auto shape = get_tensor_shape(res);

        assert(shape.size() == 2);
        assert(shape[0] == 1);

        // we only care about the first dimension of shape
        for (int row = 0; row < shape[0]; ++row) {
            predictions_s0(row) = res.tensor<float, 2>()(row, 0);
        }

        if constexpr (MODEL_OUTPUT_LOGITS) {
            // sigmoid: convert to probability
            predictions_s0 = vec_sigmoid(predictions_s0);
        }
        if constexpr (DEBUG_PRINT_PREDICTIONS) {
            std::cout << "l0/s0 predictions: " << l0_prob << ", " << predictions_s0(0) << std::endl;
        }
        s0_prob = predictions_s0(0);

        // convert probability to 0/1
        predictions_s0 = vec_prediction(predictions_s0, PREDICTION_THRESHHOLD_SHORT);
    }
    // long3
    if constexpr (signal_size > 2)
    {
        auto& res = out[2];
        auto shape = get_tensor_shape(res);

        assert(shape.size() == 2);
        assert(shape[0] == 1);

        // we only care about the first dimension of shape
        for (int row = 0; row < shape[0]; ++row) {
            predictions_l3(row) = res.tensor<float, 2>()(row, 0);
        }

        if constexpr (MODEL_OUTPUT_LOGITS) {
            // sigmoid: convert to probability
            predictions_l3 = vec_sigmoid(predictions_l3);
        }
        if constexpr (DEBUG_PRINT_PREDICTIONS) {
            //std::cout << "long3 predictions: " << predictions_l3 << std::endl;
        }
        l3_prob = predictions_l3(0);

        // convert probability to 0/1
        predictions_l3 = vec_prediction(predictions_l3, PREDICTION_THRESHHOLD_LONG3);
    }
    // short3
    if constexpr (signal_size > 3)
    {
        auto& res = out[3];
        auto shape = get_tensor_shape(res);

        assert(shape.size() == 2);
        assert(shape[0] == 1);

        // we only care about the first dimension of shape
        for (int row = 0; row < shape[0]; ++row) {
            predictions_s3(row) = res.tensor<float, 2>()(row, 0);
        }

        if constexpr (MODEL_OUTPUT_LOGITS) {
            // sigmoid: convert to probability
            predictions_s3 = vec_sigmoid(predictions_s3);
        }
        if constexpr (DEBUG_PRINT_PREDICTIONS) {
            //std::cout << "short3 predictions: " << predictions_s3 << std::endl;
        }
        s3_prob = predictions_s3(0);

        // convert probability to 0/1
        predictions_s3 = vec_prediction(predictions_s3, PREDICTION_THRESHHOLD_SHORT3);
    }

    if constexpr (signal_size > 3) {
        //const auto l0 = predictions_l0(0) > 0;
        //const auto s0 = predictions_s0(0) > 0;
        //return { l0, s0, l0, s0 };


        const auto l0 = l0_prob - s0_prob > 0.57f;  // predictions_l0(0) > 0;
        const auto s0 = s0_prob - l0_prob > 0.57f;  // predictions_s0(0) > 0;
        const auto l3 = predictions_l3(0) > 0;
        const auto s3 = predictions_s3(0) > 0;

        auto l0res = l0_prob - s0_prob > 0.79f;
        auto s0res = s0_prob - l0_prob > 0.79f;
        //auto l3res = l3_prob - s3_prob > 0.91f;
        //auto s3res = s3_prob - l3_prob > 0.91f;
        if (s0res || s3_prob > 0.97f) {
            block_long_til_ts = last_ts + X0_BLOCK_US;
        }
        else if (l0res || l3_prob > 0.97f) {
            block_short_til_ts = last_ts + X0_BLOCK_US;
        }
        if (l0 && block_long_til_ts && last_ts < block_long_til_ts) {
            std::cout << "long0 BLOCKED" << std::endl;
            l0res = false;
        }
        if (s0 && block_short_til_ts && last_ts < block_short_til_ts) {
            std::cout << "short0 BLOCKED" << std::endl;
            s0res = false;
        }
        return { l0res, s0res, (l0 || l3), (s0 || s3) };


        //auto l0res = l0;
        //auto s0res = s0;
        //if (l0) {
        //    block_short_til_ts = last_ts + X0_BLOCK_US;
        //}
        //if (l3) {
        //    block_short_til_ts = last_ts + X3_BLOCK_US;
        //    boost_short_til_ts = last_ts + X3_BOOST_US;
        //}
        //if (s0) {
        //    block_long_til_ts = last_ts + X0_BLOCK_US;
        //}
        //if (s3) {
        //    block_long_til_ts = last_ts + X3_BLOCK_US;
        //    boost_long_til_ts = last_ts + X3_BOOST_US;
        //}
        //if (l0 && block_long_til_ts && last_ts < block_long_til_ts) {
        //    std::cout << "long0 BLOCKED" << std::endl;
        //    l0res = false;
        //}
        //if (s0 && block_short_til_ts && last_ts < block_short_til_ts) {
        //    std::cout << "short0 BLOCKED" << std::endl;
        //    s0res = false;
        //}
        ////return { l0, s0, l3, s3 };
        //return { l0res, s0res, (l0 || l3), (s0 || s3) };
    }
    else if constexpr (signal_size > 2) {
        return { predictions_l0(0) > 0, predictions_s0(0) > 0, predictions_l3(0) > 0 };
    }
    else if constexpr (signal_size > 1) {
        return { predictions_l0(0) > 0, predictions_s0(0) > 0 };
    }
    else {
        return { predictions_l0(0) > 0 };
    }
}
