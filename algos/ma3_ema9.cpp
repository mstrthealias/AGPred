#include "ma3_ema9.h"

#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>

#include <ta_libc.h>

#include "../src/util.h"


using namespace agpred;


int agpred::calc_raw_signal(const xtensor_raw& raw)
{

	//std::cout << "dumpRaw() data_processed.shape:" << std::endl << raw.shape() << std::endl;

	// copy 1 minute interval
	xt::xarray<real_t> data_1min = xt::view(raw, static_cast<ptrdiff_t>(0), xt::all(), xt::all());
	//std::cout << "dumpRaw() data_1min.shape:" << std::endl << data_1min.shape() << std::endl;

	// transpose that-way sorts correct order
	data_1min = xt::transpose(data_1min, { 1, 0 });
	
	// reverse order
	if (_xt_2d_sort(data_1min, ColPos::In::timestamp, false))
	{
		//std::cout << "...sorted" << std::endl;
	}
	
	//xt::xarray<real_t> row0row0 = xt::row(data_1min, ColPos::In::timestamp);
	//std::cout << "dumpRaw() row[0]row[0].shape:" << std::endl << row0row0.shape() << std::endl;
	//std::cout << "dumpRaw() row[0]row[0]:" << std::endl << row0row0 << std::endl;


	// fetch MA3, EMA9

	const size_t n_len = data_1min.shape().at(1);
	// copy rows for TALib
	const auto r_close = xt::xarray<real_t>(xt::row(data_1min, ColPos::In::close));

	const auto base_adj = 1;
	const real_t& cur_close = r_close[n_len - base_adj];
	const real_t& prev_close = r_close[n_len - base_adj - 1];
	
	bool crossed_above_ma3 = false;
	bool crossed_below_ma3 = false;
	bool above_ema9 = false;
	bool below_ema9 = false;
	bool below_ma3;

	// MA3
	{
		std::vector<double> vals(n_len + 1);
		int outBegIdx = 0;
		int outNBElement = 0;
		std::fill(vals.begin(), vals.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		const TA_RetCode retCode = TA_MA(0, n_len, r_close.data(), 3, TA_MAType_SMA, &outBegIdx, &outNBElement, vals.data() + TA_MA_Lookback(3, TA_MAType_SMA));
#else
		const TA_RetCode retCode = TA_S_MA(0, n_len, r_close.data(), 3, TA_MAType_SMA, &outBegIdx, &outNBElement, vals.data() + TA_MA_Lookback(3, TA_MAType_SMA));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "SMA3 error: " << retCode << std::endl;
			return 0;  // retCode;
		}
		bool prev_above_ma3 = prev_close > vals[n_len - base_adj - 1];
		bool above_ma3 = cur_close > vals[n_len - base_adj];
		bool prev_below_ma3 = prev_close < vals[n_len - base_adj - 1];
		below_ma3 = cur_close < vals[n_len - base_adj];
		
		crossed_above_ma3 = above_ma3 && !prev_above_ma3;
		crossed_below_ma3 = below_ma3 && !prev_below_ma3;
	}

	// EMA9
	{
		std::vector<double> vals(n_len + 1);
		int outBegIdx = 0;
		int outNBElement = 0;
		std::fill(vals.begin(), vals.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		const TA_RetCode retCode = TA_MA(0, n_len, r_close.data(), 9, TA_MAType_EMA, &outBegIdx, &outNBElement, vals.data() + TA_MA_Lookback(9, TA_MAType_EMA));
#else
		const TA_RetCode retCode = TA_S_MA(0, n_len, r_close.data(), 9, TA_MAType_EMA, &outBegIdx, &outNBElement, vals.data() + TA_MA_Lookback(9, TA_MAType_EMA));
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "EMA9 error: " << retCode << std::endl;
			return 0;  // retCode;
		}
		above_ema9 = cur_close > vals[n_len - base_adj];
		below_ema9 = cur_close < vals[n_len - base_adj];
	}
	
	// fires signal when above ema9, and we crossed above ma3
	return (above_ema9 && crossed_above_ma3 ? 1 : (below_ema9 && crossed_below_ma3 ? -1 : 0));
	//return (above_ema9 && crossed_above_ma3 ? 1 : (below_ema9 || below_ma3 ? -1 : 0));
}
