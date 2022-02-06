#include "data_generator.h"

#include <xtensor/xarray.hpp>
#include <xtensor/xnpy.hpp>
#include <xtensor/xdynamic_view.hpp>
#include <xtensor/xio.hpp>
#include <stdexcept>

#include "common.h"


//class substreambuf final : public std::streambuf
//{
//public:
//
//    substreambuf(std::streambuf* sbuf, std::size_t start, std::size_t len) : m_sbuf(sbuf), m_start(start), m_len(len), m_pos(0)
//    {
//        std::streampos p = m_sbuf->pubseekpos(start);
//        assert(p != std::streampos(-1));
//        setbuf(NULL, 0);
//    }
//
//protected:
//
//    int underflow() override
//    {
//        if (m_pos + std::streamsize(1) >= m_len)
//            return traits_type::eof();
//        return m_sbuf->sgetc();
//    }
//
//    int uflow() override
//    {
//        if (m_pos + std::streamsize(1) > m_len)
//            return traits_type::eof();
//        m_pos += std::streamsize(1);
//        return m_sbuf->sbumpc();
//    }
//
//    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
//    {
//        std::streampos cursor;
//
//        if (way == std::ios_base::beg)
//            cursor = off;
//        else if (way == std::ios_base::cur)
//            cursor = m_pos + off;
//        else if (way == std::ios_base::end)
//            cursor = m_len - off;
//
//        if (cursor < 0 || cursor >= m_len)
//            return std::streampos(-1);
//        m_pos = cursor;
//        if (m_sbuf->pubseekpos(m_start + m_pos, std::ios_base::beg) == std::streampos(-1))
//            return std::streampos(-1);
//
//        return m_pos;
//    }
//
//    std::streampos seekpos(std::streampos sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
//    {
//        if (sp < 0 || sp >= m_len)
//            return std::streampos(-1);
//        m_pos = sp;
//        if (m_sbuf->pubseekpos(m_start + m_pos, std::ios_base::beg) == std::streampos(-1))
//            return std::streampos(-1);
//        return m_pos;
//    }
//
//private:
//    std::streambuf* m_sbuf;
//    std::streampos m_start;
//    std::streamsize m_len;
//    std::streampos m_pos;
//};


//int test_load()
//{
//	std::string npy_file_inputs = "pyfiles/EVAL.0.inputs.npy";
//	std::string npy_file_outputs = "pyfiles/EVAL.0.outputs.npy";
//
//	xt::xarray<float> inputs;
//	xt::xarray<float> outputs;
//
//	try
//	{
//		std::cout << "Loading " << npy_file_inputs << "..." << std::endl;
//		inputs = xt::load_npy<float>(npy_file_inputs);
//		std::cout << "Loaded " << npy_file_inputs << "." << std::endl;
//	}
//	catch (std::exception& e)
//	{
//		std::cout << "Inputs load failed: " << e.what() << std::endl;
//		return 0;
//	}
//	std::cout << "Inputs shape: " << inputs.shape() << std::endl;
//
//	try
//	{
//		std::cout << "Loading " << npy_file_outputs << "..." << std::endl;
//		outputs = xt::load_npy<float>(npy_file_outputs);
//		std::cout << "Loaded " << npy_file_outputs << "." << std::endl;
//	}
//	catch (std::exception& e)
//	{
//		std::cout << "Outputs load failed: " << e.what() << std::endl;
//		return 0;
//	}
//	std::cout << "Outputs shape: " << outputs.shape() << std::endl;
//
//
//	return 0;
//}


DataGenerator::DataGenerator(unsigned int batch_size, std::string file_dir, std::string file_pattern) : batch_size(batch_size), file_dir(file_dir), glob_(file_dir + "/" + file_pattern)
{
}

DataGenerator::~DataGenerator()
{
}


bool DataGenerator::next(xt::xarray<float>& a_inputs, xt::xarray<float>& a_outputs)
{
    if (n_file_inputs_ == 0 || cur_pos_ + batch_size > n_file_inputs_)
    {
        // load next file...

        if (!glob_)
            return false;  // no more data

        // advance next file...
        file_no_++;
        cur_pos_ = 0;
        cur_file_ = file_dir + "/" + glob_.GetFileName();
        glob_.Next();  // advance glob file pointer now...

        std::cout << "> Loading " << cur_file_ << "..." << std::endl;
        try
        {
            std::ifstream file_stream(cur_file_, std::ios::binary);
            if (!file_stream.is_open())
                throw std::runtime_error("Unable to open/read file");  // no throw just to catch yar

            a_inputs_ = xt::load_npy<float>(file_stream);
            a_outputs_ = xt::load_npy<float>(file_stream);

            // choose profit5 output?
            auto tmp_row = xt::dynamic_view(a_outputs_, { xt::all(), 3 });  // TODO choose output?
            xt::xarray<float> a_tmp = xt::zeros<float>(tmp_row.shape());
            std::copy(tmp_row.crbegin(), tmp_row.crend(), a_tmp.begin());
            a_outputs_ = xt::expand_dims(a_tmp, 1);
            
            n_file_inputs_ = static_cast<unsigned int>(a_inputs_.shape()[0]);  // number of inputs in this file...
        }
        catch (std::exception& e)
        {
            std::cout << "> DataGenerator load failed: " << e.what() << std::endl;
            return false;
        }
        //std::cout << "> Loaded " << cur_file_ << "." << std::endl;
        //
        //std::cout << "> Inputs file shape: " << a_inputs_.shape() << std::endl;
        //std::cout << "> Outputs file shape: " << a_outputs_.shape() << std::endl;
    }

    // load next batch inside active file
    {
        auto cur_inputs = xt::dynamic_view(a_inputs_, { xt::range(cur_pos_, cur_pos_ + batch_size),  xt::all(), xt::all(), xt::all() });
        a_inputs = xt::zeros<float>(cur_inputs.shape());
        // TODO this copy too slow? only slow in Debug??
        std::copy(cur_inputs.crbegin(), cur_inputs.crend(), a_inputs.begin());
    }
    {
        auto cur_outputs = xt::dynamic_view(a_outputs_, { xt::range(cur_pos_, cur_pos_ + batch_size),  xt::all() });
        a_outputs = xt::zeros<float>(cur_outputs.shape());
        std::copy(cur_outputs.crbegin(), cur_outputs.crend(), a_outputs.begin());
    }
    cur_pos_ += batch_size;

    return true;
}


void DataGenerator::reset()
{
	cur_pos_ = 0;
	file_no_ = -1;
	cur_file_ = "";
	glob_.Reset();
}
