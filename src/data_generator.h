#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H


#include <xtensor/xarray.hpp>

#include "../deps/glob/glob.h"

//#include "common.h"


class DataGenerator
{
public:
	DataGenerator(unsigned int batch_size, std::string file_dir, std::string file_pattern);
	~DataGenerator();

	bool next(xt::xarray<float>& a_inputs, xt::xarray<float>& a_outputs);

	void reset();
public:
	const unsigned int batch_size;
	const std::string file_dir;

private:
	glob::Glob glob_;

	unsigned int cur_pos_ = 0;  // position in current file
	int file_no_ = -1;  // incremented each time a file is read (-1 after reset)
	unsigned int n_file_inputs_ = 0;
	std::string cur_file_;

	// keep one file loaded in memory (TODO reduce size as batches are read?)
	xt::xarray<float> a_inputs_;
	xt::xarray<float> a_outputs_;
};


#endif // DATA_GENERATOR_H
