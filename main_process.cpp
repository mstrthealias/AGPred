#define NOMINMAX

#include <xtensor/xio.hpp>
#include <xtensor/xarray.hpp>
#include <xtensor/xnpy.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xset_operation.hpp>
#include <xtensor/xmanipulation.hpp>

#include <ta_libc.h>

#include <string>
#include <iostream>
#include <vector>
#include <cfloat>

#include "src/common.h"
#include "src/preprocess.h"
#include "src/defs.h"
#include "src/consolidate.h"


static const int SRC_SAMPLES = 2126;
static const int SRC_COLS = 6;



xt::xarray<real_t> temp_array()
{
    /* // (3, 6)
    return {
        {0., 1.1, 1.2, 1.3, 1.4, 10},
        {1., 2.1, 2.2, 2.3, NAN, 20},
        {2., 3.1, 3.2, 3.3, 3.4, 30},
    };*/
    // (6, 3)
    return {
        {0., 1., 2., 3., 4., 5., 6., 7., 8.},
        {1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 3.0, 3.0, 3.0},
        {1.2, 2.2, 3.2, 1.2, 2.2, 3.2, 1.2, 2.2, 3.2},
        {1.3, 2.3, 3.3, 1.3, 2.3, 3.3, 1.3, 2.3, 3.3},
        {1.4, NAN, 3.4, 1.4, NAN, 3.4, 1.4, NAN, 3.4},
    };
}


xt::xarray<double> load_npy_src(const std::string& filename)
{
    std::cout << "Loading " << filename << "..." << std::endl;
    auto data = xt::load_npy<double>(filename);
    std::cout << "Loaded " << filename << "." << std::endl;
    return data;
}

int yar1(const xt::xarray<real_t>& input)
{

    TA_RetCode retCode;
    int outBegIdx = 0;
    int outNBElement = 0;

    // Access data directly through xt::view
    const auto close_p = xt::xarray<real_t>(xt::row(input, 4));

    std::cout << "input: " << close_p << std::endl;

    //std::copy(input.crbegin(), input.crend(), b.begin());

    size_t n_rows = input.shape().at(1);
    //std::cout << "input.shape: " << input.shape() << std::endl;
    //std::cout << "close_p.shape: " << close_p.shape() << std::endl;

    std::vector<double> res(n_rows + 1);

    //TA_MAType_SMA
#ifdef AGPRED_DOUBLE_P
    retCode = TA_LINEARREG_SLOPE(0, n_rows, close_p.data(), 3, &outBegIdx, &outNBElement, res.data());
#else
    retCode = TA_S_LINEARREG_SLOPE(0, n_rows, close_p.data(), 3, &outBegIdx, &outNBElement, res.data());
#endif
    if (retCode != TA_SUCCESS)
    {
        std::cout << "LINEARREG_SLOPE error: " << retCode << std::endl;
        return retCode;
    }
    std::cout << "outBegIdx: " << outBegIdx << std::endl;
    std::cout << "outNBElement: " << outNBElement << std::endl;

    res.pop_back();
    // res.push_back(0.0);
    //std::cout << "res: " << res  << std::endl;

    return 0;
}

const auto _xt_r = xt::placeholders::_r;
const auto _xt_ = xt::placeholders::_;

int main(int argc, char* argv[])
{
    /*
    xt::xarray<real_t> a_tmp = temp_array();
    std::cout << "a_tmp: " << a_tmp << std::endl;
    std::cout << "a_tmp.shape: " << a_tmp.shape() << std::endl;

    auto v = _xt_nonans(a_tmp, 4);
    std::cout << "v: " << v << std::endl;
    std::cout << "v.shape: " << v.shape() << std::endl;

    //yar1(a_tmp);
    return 0;
    */
		
    TA_RetCode retCode;
    int outBegIdx = 0;
    int outNBElement = 0;

    retCode = TA_Initialize();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib initialize error: " << retCode << std::endl;
        return retCode;
    }
    std::cout << "TA-Lib initialized.\n";
 
    std::string npy_file = "pyfiles/_tmp.AAPL." + std::to_string(TIMEFRAME) + ".tohlcv.npy";
    xt::xarray<double> input = load_npy_src(npy_file);

    dfs_map_t dfs = { {TIMEFRAME, xt::xarray<real_t>(input)} };
    dfs_ts_map_t dfs_ts = { {TIMEFRAME, xt::xarray<timestamp_us_t>(xt::view(input, ColPos::In::timestamp, xt::all()))} };

    const std::map<const char*, const int> interval_map = {
            {"1min", 1}
    };

    //yar1(input);
    process_step1to3_2a("APPL", dfs_ts, dfs, TIMEFRAME, false, interval_map);


    retCode = TA_Shutdown();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib shutdown error: " << retCode << std::endl;
        return retCode;
    }
    return 0;
} 
