#include <xtensor/xarray.hpp>

#include <tiny-cuda-nn/common.h>
#include <tiny-cuda-nn/config.h>

#include <nlohmann/json.hpp>

// #include "src/defs.h"

#include "src/common.h"
#include "src/data_generator.h"


using json = nlohmann::json;

using namespace tcnn;


const uint32_t batch_size = 512;  // 1 << 16;
const uint32_t n_epochs = 150;

const uint32_t n_input_dims_to_encode = 9;  // (timesteps, cols, timeframes)
const uint32_t n_input_dims_to_pass_through = 2;  // TODO 2?
//const uint32_t n_input_dims = 3;  // TODO pass through dims?
const uint32_t n_row_size = 127458;  // TODO ? 127458=73*291*6

const uint32_t n_output_dims = 1;


bool generate_training_batch(DataGenerator& data_gen, GPUMatrix<float>& inputs, GPUMatrix<float>& outputs)
{
	xt::xarray<float> a_inputs;
	xt::xarray<float> a_outputs;

	if (!data_gen.next(a_inputs, a_outputs))
		return false;  // no more data

	//std::cout << "Inputs shape: " << a_inputs.shape() << std::endl;
	//std::cout << "Outputs shape: " << a_outputs.shape() << std::endl;
	
	//std::cout << "sizeof(a_inputs): " << a_inputs.size() * sizeof(float) << std::endl;
	//std::cout << "sizeof(a_outputs): " << a_outputs.size() * sizeof(float) << std::endl;
	//std::cout << "sizeof(inputs): " << inputs.n_bytes() << std::endl;
	//std::cout << "sizeof(outputs): " << outputs.n_bytes() << std::endl;
	
	CUDA_CHECK_THROW(cudaMemcpy(inputs.data(), a_inputs.data(), a_inputs.size() * sizeof(float), cudaMemcpyHostToDevice));
	CUDA_CHECK_THROW(cudaMemcpy(outputs.data(), a_outputs.data(), a_outputs.size() * sizeof(float), cudaMemcpyHostToDevice));

	//std::cout << "Copied..." << std::endl;

	return true;
}

bool generate_pred_inputs(DataGenerator& data_gen, GPUMatrix<float>& inputs)
{
	// data_gen.reset();  // TODO fix reset

	xt::xarray<float> a_inputs;
	xt::xarray<float> a_outputs;
	
	if (!data_gen.next(a_inputs, a_outputs))
		return false;  // no more data

	std::cout << "Prediction inputs shape: " << a_inputs.shape() << std::endl;

	//std::cout << "sizeof(a_inputs): " << a_inputs.size() * sizeof(float) << std::endl;
	//std::cout << "sizeof(inputs): " << inputs.n_bytes() << std::endl;

	CUDA_CHECK_THROW(cudaMemcpy(inputs.data(), a_inputs.data(), a_inputs.size() * sizeof(float), cudaMemcpyHostToDevice));

	std::cout << "Prediction copied..." << std::endl;

	return true;
}


int main(int argc, char* argv[])
{
	json config = {
		{"loss", {
			{"otype", "L2"}
		}},
		{"optimizer", {
			{"otype", "Adam"},
			{"learning_rate", 1e-3},
		}},
		{"encoding", {
			{"otype", "OneBlob"},
			{"n_bins", 32},
		}},
		{"network", {
			{"otype", "FullyFusedMLP"},
			{"n_neurons", 128},
			{"n_hidden_layers", 5},
			{"activation", "ReLU"},
			{"output_activation", "Sigmoid"},
		}},
	};

	auto tpl = create_from_config(n_input_dims_to_encode, n_input_dims_to_pass_through, n_output_dims, config);
	auto losses = std::get<0>(tpl);
	auto optimizer = std::get<1>(tpl);
	auto network = std::get<2>(tpl);
	auto trainer = std::get<3>(tpl);

	// Prepare data generator
	
	// Train the model
	GPUMatrix<float> training_batch_inputs(n_row_size, batch_size);  // TODO n_input_dims?
	GPUMatrix<float> training_batch_targets(n_output_dims, batch_size);  // TODO n_output_dims?

	training_batch_inputs.initialize_constant(0);
	training_batch_targets.initialize_constant(0);
	/*cudaMemset(&training_batch_inputs, 0, sizeof training_batch_inputs);
	cudaMemset(&training_batch_targets, 0, sizeof training_batch_targets);*/

	//DataGenerator data_gen(batch_size, "pyfiles", "TRAIN.*.merged.npy");
	//generate_training_batch(data_gen, training_batch_inputs, training_batch_targets);
	//return 0;

	// create a cuda stream
	cudaStream_t stream1;
	cudaStreamCreate(&stream1);

	int epoch = 0;
	for (int i = 0; i < n_epochs; ++i) {
		std::cout << "load file..." << std::endl;
		DataGenerator data_gen(batch_size, "pyfiles", "TRAIN.*.merged.npy");
		std::cout << "loaded file." << std::endl;
		while (generate_training_batch(data_gen, training_batch_inputs, training_batch_targets))
		{
			float loss;
			trainer->training_step(stream1, training_batch_inputs, training_batch_targets, &loss);
			std::cout << "epoch=" << epoch << " iteration=" << i << " loss=" << loss << std::endl;
		}
		epoch++;
	}

	// wait until all cuda operations are complete
	cudaStreamSynchronize(stream1);

	DataGenerator data_gen_eval(batch_size, "pyfiles", "EVAL.*.merged.npy");

	// Use the model
	GPUMatrix<float> inference_inputs(n_row_size, batch_size);
	training_batch_inputs.initialize_constant(0);
	generate_pred_inputs(data_gen_eval, inference_inputs);
	
	GPUMatrix<float> inference_outputs(n_output_dims, batch_size);
	network->inference(stream1, inference_inputs, inference_outputs);

	// wait until all cuda operations are complete
	cudaStreamSynchronize(stream1);

	//std::cout << "inference_outputs[0]: " << *inference_outputs.data() << std::endl;

}