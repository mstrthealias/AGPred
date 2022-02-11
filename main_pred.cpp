#define NOMINMAX

#include <tensorflow/cc/saved_model/loader.h>
#include <tensorflow/cc/saved_model/tag_constants.h>
#include <tensorflow/core/public/session_options.h>
#include <tensorflow/core/framework/tensor.h>
#include <tensorflow/core/lib/io/path.h>
// #include <tensorflow/cc/ops/array_ops.h>

#include <xtensor/xarray.hpp>
#include <xtensor/xnpy.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <cfloat>

static const int IMG_SIZE = 36828;
static const int NUM_SAMPLES = 255;  // 10240;

static const int SHAPE_1 = 33;
static const int SHAPE_2 = 124;
static const int SHAPE_3 = 9;

static const int SRC_SAMPLES = 2126;
static const int SRC_COLS = 6;


void test_tf() {
    //tensorflow::Session* session;
    //tensorflow::Status status = NewSession(tensorflow::SessionOptions(), &session);
    //if (!status.ok()) {
    //    std::cout << status.ToString() << "\n";
    //    return;
    //}
    //std::cout << "Session successfully created.\n";
}

tensorflow::Tensor load_npy_img(const std::string& filename) {
    std::cout << "Loading " << filename << "..." << std::endl;
    auto data = xt::load_npy<float>(filename);
    //data.reshape({ NUM_SAMPLES, SHAPE_1, SHAPE_2, SHAPE_3 });
    std::cout << "Loaded " << filename << "." << std::endl;

    std::cout << "Setting up input tensor..." << std::endl;
    /*tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({NUM_SAMPLES, IMG_SIZE}));
    for (int i = 0; i < NUM_SAMPLES; i++)
        for (int j = 0; j < IMG_SIZE; j++)
            t.tensor<float, 2>()(i, j) = data(i, j);*/
    tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ NUM_SAMPLES, SHAPE_1, SHAPE_2, SHAPE_3 }));
    for (int i = 0; i < NUM_SAMPLES; i++)
        for (int j = 0; j < SHAPE_1; j++)
            for (int k = 0; k < SHAPE_2; k++)
                for (int l = 0; l < SHAPE_3; l++)
                    t.tensor<float, 4>()(i, j, k, l) = data(i, j, k, l);
    std::cout << "Tensor ready." << std::endl;
    //std::cout << "Tensor:" << t.shape() << std::endl;
    //std::cout << "Tensor:" << t.dtype() << std::endl;
    return t;
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

std::string _input_name = "features";
std::string _output_name = "profit3";


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
    std::string npy_file = "pyfiles/_EVAL.1.above_range.inputs.pkl.npy";
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
    auto inputs = model_def.inputs().at(_input_name);
    auto input_name = inputs.name();
    auto outputs = model_def.outputs().at(_output_name);
    auto output_name = outputs.name();

    auto input = load_npy_img(npy_file);

    TF_CHECK_OK(bundle.session->Run({{input_name, input}},
                                    {output_name},
                                    {},
                                    &out));
    std::cout << out[0].DebugString() << std::endl;

    auto res = out[0];
    auto shape = get_tensor_shape(res);

    std::cout << "SHAPE SIZE: " << shape.size() << std::endl;
    std::cout << "SHAPE[0]: " << shape[0] << std::endl;

    // we only care about the first dimension of shape
    xt::xarray<float> predictions = xt::zeros<float>({shape[0]});
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

    xt::dump_npy(prediction_npy_file, predictions);
    return 0;
}
