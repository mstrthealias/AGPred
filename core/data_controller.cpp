#include "data_controller.h"

#include <array>
#include <iostream>
#include <tuple>
#include <algorithm>

#include <ta_libc.h>

#include <xtensor/xarray.hpp>
#include <xtensor/xtensor.hpp>
#include <xtensor/xfixed.hpp>
#include <xtensor/xnpy.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xdynamic_view.hpp>
#include <xtensor/xio.hpp>

#include "../src/consolidate.h"
#include "../adapters/polygon_io.h"  // TODO not directly include an adapter o.0
#include "../src/preprocess.h"


using namespace agpred;


static const json JSON_NULL = json::parse("null");

// TODO move this (stuff) into class?!  // TODO don't use a class?
// DataController should only be constructed once...
static bool dc_initialized = false;


/*template<typename T>
timestamp_us_t norm_1min_us(T ts)
{
	return static_cast<timestamp_us_t>(static_cast<timestamp_us_t>(ts) / static_cast<timestamp_us_t>(60e6))
		* static_cast<timestamp_us_t>(60e6);
}

template<typename T>
timestamp_us_t norm_1min_s_to_us(T ts) {
	return static_cast<timestamp_us_t>(static_cast<timestamp_us_t>(ts) / static_cast<timestamp_us_t>(60))
		* static_cast<timestamp_us_t>(60)
		* SEC_TO_US;
}*/

/*void zero_symbols_data(std::array<xtensor_raw, MAX_ACTIVE_SYMBOLS>& data)
{
	for (auto& symbolData : data)
		symbolData = xt::zeros<real_t>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
}

void zero_processed_data(std::array<xt::xtensor_fixed<real_t, shape_processed_t>, MAX_ACTIVE_SYMBOLS>& data)
{
	for (auto& symbolData : data)
		symbolData = xt::zeros<real_t>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
}*/


// Note: data_outputs is only populated if interval==TIMEFRAME
inline void run_preprocess(xtensor_processed_interval& data_processed, xtensor_outputs_interval& data_outputs, const Symbol& symbol, const int interval, const xtensor_ts_interval& ts_raw, const xtensor_raw_interval& data_raw)
{
	//std::cout << interval << "min raw:" << std::endl << data_raw << std::endl;
	//std::cout << interval << "min raw shape:" << std::endl << data_raw.shape() << std::endl;

	// must transpose raw data to (features, timesteps) for preprocess...
	xt::xarray<timestamp_us_t> ts_raw_transposed = xt::transpose(ts_raw, { 1, 0 });
	xt::xarray<real_t> data_raw_transposed = xt::transpose(data_raw, { 1, 0 });

	// process interval data (returned in ASC order, input is in DESC order):
	// TODO TIMEFRAME, training
	const bool training = TIMEFRAME == interval;

	/*
	auto& a_step1 = process_step1_single(symbol.symbol.c_str(), data_raw_transposed, training, TIMEFRAME, interval, false);
	auto processed = process_step2_single(symbol.symbol.c_str(), a_step1, training, TIMEFRAME, interval, false);

	// TODO skip creating outputs (elsewhere) if !training?
	xt::xarray<double> o_outputs;  // TODO include timestamp and/or close in outputs?
	if (training)
		o_outputs = xt::zeros<double>({ static_cast<int>(ColPos::_OUTPUT_NUM_COLS), static_cast<int>(a_step1.shape().at(1)) });
	process_step3_single(processed, o_outputs, symbol.symbol.c_str(), a_step1, training, TIMEFRAME, interval, false);
	*/

	process_step1_single_2a(ts_raw_transposed, data_raw_transposed, symbol.symbol.c_str(), training, TIMEFRAME, interval, false);
	auto processed = process_step2_single_2a(symbol.symbol.c_str(), ts_raw_transposed, data_raw_transposed, training, TIMEFRAME, interval, false);

	// TODO skip creating outputs (elsewhere) if !training?
	xt::xarray<double> o_outputs;  // TODO include timestamp and/or close in outputs?
	if (training)
		o_outputs = xt::zeros<double>({ static_cast<int>(ColPos::_OUTPUT_NUM_COLS), static_cast<int>(data_raw_transposed.shape().at(1)) });
	process_step3_single_2a(processed, o_outputs, ts_raw_transposed, symbol.symbol.c_str(), data_raw_transposed, training, TIMEFRAME, interval, false);

	if (training) {
		const auto outputs_transposed = xt::transpose(o_outputs, { 1, 0 });
		std::copy(outputs_transposed.crbegin(), outputs_transposed.crend(), data_outputs.rbegin());
	}

	// transpose to match format that's being copied into
	const auto processed_transposed = xt::transpose(processed, { 1, 0 });
	std::copy(processed_transposed.crbegin(), processed_transposed.crend(), data_processed.rbegin());

	if (DEBUG_PRINT_PROCESSED_DATA)
	{
		std::cout << interval << "min processed:" << std::endl << data_processed << std::endl;
		std::cout << interval << "min processed shape: " << data_processed.shape() << std::endl;

		/*
		// log timestamps with xt::row() for verification
		xt::xarray<real_t> processed_transposed_heap = xt::transpose(data_processed, { 1, 0 });
		xt::xarray<real_t> tss2 = xt::row(processed_transposed_heap, ColPos::In::timestamp);
		std::cout << interval << "min processed_transposed tss:" << std::endl << tss2 << std::endl;
		std::cout << interval << "min processed_transposed tss shape: " << tss2.shape() << std::endl;
		*/

		if (training) {
			std::cout << interval << "min outputs: " << data_outputs << std::endl;
			std::cout << interval << "min outputs shape: " << data_outputs.shape() << std::endl;
		}
	}
}

inline int run_alts(xtensor_raw_interval& data_raw, const Symbol& symbol, const int interval, const xtensor_ts_interval& ts_raw)
{
	const size_t n_len = data_raw.shape().at(0);

	const auto row_shape = xt::col(data_raw, ColPos::In::high).shape();

	// copy for input (in reverse order) for TALib
	xt::xarray<real_t> r_tmp = xt::zeros<real_t>(row_shape);
	xt::xarray<real_t> r_high = xt::zeros<real_t>(row_shape);
	xt::xarray<real_t> r_low = xt::zeros<real_t>(row_shape);
	xt::xarray<real_t> r_close = xt::zeros<real_t>(row_shape);
	auto v_high = xt::col(data_raw, ColPos::In::high);
	auto v_low = xt::col(data_raw, ColPos::In::low);
	auto v_close = xt::col(data_raw, ColPos::In::close);
	std::copy(v_high.crbegin(), v_high.crend(), r_high.begin());
	std::copy(v_low.crbegin(), v_low.crend(), r_low.begin());
	std::copy(v_close.crbegin(), v_close.crend(), r_close.begin());

	//// copy for input to TALib
	//const auto r_high = xt::xarray<real_t>(xt::col(data_raw, ColPos::In::high));
	//const auto r_low = xt::xarray<real_t>(xt::col(data_raw, ColPos::In::low));
	//const auto r_close = xt::xarray<real_t>(xt::col(data_raw, ColPos::In::close));

	// MA3
	{
		int taLookback = TA_MA_Lookback(3, TA_MAType_SMA);
		std::vector<double> vals(n_len + std::max(1, taLookback));
		int outBegIdx = 0;
		int outNBElement = 0;
		std::fill(vals.begin(), vals.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		const TA_RetCode retCode = TA_MA(0, n_len, r_close.data(), 3, TA_MAType_SMA, &outBegIdx, &outNBElement, vals.data() + taLookback);
#else
		const TA_RetCode retCode = TA_S_MA(0, n_len, r_close.data(), 3, TA_MAType_SMA, &outBegIdx, &outNBElement, vals.data() + taLookback);
		//const TA_RetCode retCode = TA_S_MA(n_len - 1, n_len - 1, r_close.data(), 3, TA_MAType_SMA, &outBegIdx, &outNBElement, vals.data() + n_len - 1);
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "SMA3 error: " << retCode << std::endl;
			return retCode;  // TODO
		}

		//std::copy(vals.cbegin(), vals.cend() - std::max(1, taLookback), xt::col(data_raw, ColPos::In::alt1).begin());
		std::copy(vals.cbegin(), vals.cend() - std::max(1, taLookback), r_tmp.begin());
		std::copy(r_tmp.crbegin(), r_tmp.crend(), xt::col(data_raw, ColPos::In::alt1).begin());

		//std::cout << "r_close" << std::endl << r_close << std::endl;
		//std::cout << "sma3" << std::endl << r_tmp << std::endl;
		//exit(0);
	}

	// EMA9
	{
		int taLookback = TA_MA_Lookback(9, TA_MAType_EMA);
		std::vector<double> vals(n_len + std::max(1, taLookback));
		int outBegIdx = 0;
		int outNBElement = 0;
		std::fill(vals.begin(), vals.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		const TA_RetCode retCode = TA_MA(0, n_len, r_close.data(), 9, TA_MAType_EMA, &outBegIdx, &outNBElement, vals.data() + taLookback);
#else
		const TA_RetCode retCode = TA_S_MA(0, n_len, r_close.data(), 9, TA_MAType_EMA, &outBegIdx, &outNBElement, vals.data() + taLookback);
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "EMA9 error: " << retCode << std::endl;
			return retCode;  // TODO
		}

		//std::copy(vals.cbegin(), vals.cend() - std::max(1, taLookback), xt::col(data_raw, ColPos::In::alt2).begin());
		std::copy(vals.cbegin(), vals.cend() - std::max(1, taLookback), r_tmp.begin());
		std::copy(r_tmp.crbegin(), r_tmp.crend(), xt::col(data_raw, ColPos::In::alt2).begin());
	}

	// Average True Range(ATR)
	{
		int taLookback = TA_ATR_Lookback(14);
		std::vector<double> vals(n_len + std::max(1, taLookback));
		int outBegIdx = 0;
		int outNBElement = 0;
		std::fill(vals.begin(), vals.end(), NAN);
#ifdef AGPRED_DOUBLE_P
		TA_RetCode retCode = TA_ATR(0, n_len, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, vals.data() + taLookback);
#else
		TA_RetCode retCode = TA_S_ATR(0, n_len, r_high.data(), r_low.data(), r_close.data(), 14, &outBegIdx, &outNBElement, vals.data() + taLookback);
#endif
		if (retCode != TA_SUCCESS)
		{
			std::cout << "ATR error: " << retCode << std::endl;
			return retCode;  // TODO
		}
		//std::copy(vals.cbegin(), vals.cend() - std::max(1, taLookback), xt::col(data_raw, ColPos::In::alt3).begin());
		std::copy(vals.cbegin(), vals.cend() - std::max(1, taLookback), r_tmp.begin());
		std::copy(r_tmp.crbegin(), r_tmp.crend(), xt::col(data_raw, ColPos::In::alt3).begin());
	}

	//std::cout << "ts_raw: " << xt::col(ts_raw, ColPos::In::timestamp) << std::endl;
	//std::cout << "r_close: " << r_close << std::endl;
	//std::cout << "SMA3: " << xt::col(data_raw, ColPos::In::alt1) << std::endl;
	//std::cout << "EMA9: " << xt::col(data_raw, ColPos::In::alt2) << std::endl;
	//std::cout << "ATR: " << xt::col(data_raw, ColPos::In::alt3) << std::endl;

	return 0;
}


constexpr ptrdiff_t ROW_POS = 0;

DataController::DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update on_update, const fn_update_outputs on_update_outputs) :
	mode_(mode),
	on_snapshot_(on_snapshot),
	on_update_(on_update),
	on_update_outputs_(on_update_outputs),
	cur_timesteps_(),
	latest_trades_(new std::array<trades_queue, MAX_ACTIVE_SYMBOLS>()),
	latest_quotes_(new std::array<quotes_queue, MAX_ACTIVE_SYMBOLS>()),
	symbols_outputs_(new std::array<xtensor_outputs_interval, MAX_ACTIVE_SYMBOLS>()),

	symbols_ts_1min_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_ts_5min_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_ts_15min_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_ts_1hr_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_ts_4hr_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_ts_1d_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_ts_1w_(new std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>()),

	symbols_1min_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_5min_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_15min_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_1hr_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_4hr_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_1d_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_1w_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),

	proc_symbols_1min_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),
	proc_symbols_5min_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),
	proc_symbols_15min_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),
	proc_symbols_1hr_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),
	proc_symbols_4hr_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),
	proc_symbols_1d_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),
	proc_symbols_1w_(new std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>()),

	// TODO some form of latest_* initialization that dynamically handles different MAX_ACTIVE_SYMBOLS sizes?
	latest_1min_({
		BarFullRef{
			{ (*symbols_ts_1min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[0](ROW_POS, ColPos::In::open), (*symbols_1min_)[0](ROW_POS, ColPos::In::high), (*symbols_1min_)[0](ROW_POS, ColPos::In::low), (*symbols_1min_)[0](ROW_POS, ColPos::In::close), (*symbols_1min_)[0](ROW_POS, ColPos::In::volume), (*symbols_1min_)[0](ROW_POS, ColPos::In::alt1), (*symbols_1min_)[0](ROW_POS, ColPos::In::alt2), (*symbols_1min_)[0](ROW_POS, ColPos::In::alt3) },
			(*symbols_1min_)[0](ROW_POS, ColPos::In::bid), (*symbols_1min_)[0](ROW_POS, ColPos::In::bid_high), (*symbols_1min_)[0](ROW_POS, ColPos::In::bid_low),
			(*symbols_1min_)[0](ROW_POS, ColPos::In::ask), (*symbols_1min_)[0](ROW_POS, ColPos::In::ask_high), (*symbols_1min_)[0](ROW_POS, ColPos::In::ask_low),
			(*symbols_1min_)[0](ROW_POS, ColPos::In::bid_size), (*symbols_1min_)[0](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{
			{ (*symbols_ts_1min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[1](ROW_POS, ColPos::In::open), (*symbols_1min_)[1](ROW_POS, ColPos::In::high), (*symbols_1min_)[1](ROW_POS, ColPos::In::low), (*symbols_1min_)[1](ROW_POS, ColPos::In::close), (*symbols_1min_)[1](ROW_POS, ColPos::In::volume), (*symbols_1min_)[1](ROW_POS, ColPos::In::alt1), (*symbols_1min_)[1](ROW_POS, ColPos::In::alt2), (*symbols_1min_)[1](ROW_POS, ColPos::In::alt3) },
			(*symbols_1min_)[1](ROW_POS, ColPos::In::bid), (*symbols_1min_)[1](ROW_POS, ColPos::In::bid_high), (*symbols_1min_)[1](ROW_POS, ColPos::In::bid_low),
			(*symbols_1min_)[1](ROW_POS, ColPos::In::ask), (*symbols_1min_)[1](ROW_POS, ColPos::In::ask_high), (*symbols_1min_)[1](ROW_POS, ColPos::In::ask_low),
			(*symbols_1min_)[1](ROW_POS, ColPos::In::bid_size), (*symbols_1min_)[1](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{
			{ (*symbols_ts_1min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[2](ROW_POS, ColPos::In::open), (*symbols_1min_)[2](ROW_POS, ColPos::In::high), (*symbols_1min_)[2](ROW_POS, ColPos::In::low), (*symbols_1min_)[2](ROW_POS, ColPos::In::close), (*symbols_1min_)[2](ROW_POS, ColPos::In::volume), (*symbols_1min_)[2](ROW_POS, ColPos::In::alt1), (*symbols_1min_)[2](ROW_POS, ColPos::In::alt2), (*symbols_1min_)[2](ROW_POS, ColPos::In::alt3) },
			(*symbols_1min_)[2](ROW_POS, ColPos::In::bid), (*symbols_1min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_1min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_1min_)[2](ROW_POS, ColPos::In::ask), (*symbols_1min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_1min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_1min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_1min_)[2](ROW_POS, ColPos::In::ask_size) } }),
	latest_5min_({
		BarFullRef{ { (*symbols_ts_5min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[0](ROW_POS, ColPos::In::open), (*symbols_5min_)[0](ROW_POS, ColPos::In::high), (*symbols_5min_)[0](ROW_POS, ColPos::In::low), (*symbols_5min_)[0](ROW_POS, ColPos::In::close), (*symbols_5min_)[0](ROW_POS, ColPos::In::volume), (*symbols_5min_)[0](ROW_POS, ColPos::In::alt1), (*symbols_5min_)[0](ROW_POS, ColPos::In::alt2), (*symbols_5min_)[0](ROW_POS, ColPos::In::alt3) },
			(*symbols_5min_)[2](ROW_POS, ColPos::In::bid), (*symbols_5min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_5min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_5min_)[2](ROW_POS, ColPos::In::ask), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_5min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{ { (*symbols_ts_5min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[1](ROW_POS, ColPos::In::open), (*symbols_5min_)[1](ROW_POS, ColPos::In::high), (*symbols_5min_)[1](ROW_POS, ColPos::In::low), (*symbols_5min_)[1](ROW_POS, ColPos::In::close), (*symbols_5min_)[1](ROW_POS, ColPos::In::volume), (*symbols_5min_)[1](ROW_POS, ColPos::In::alt1), (*symbols_5min_)[1](ROW_POS, ColPos::In::alt2), (*symbols_5min_)[1](ROW_POS, ColPos::In::alt3) },
			(*symbols_5min_)[2](ROW_POS, ColPos::In::bid), (*symbols_5min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_5min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_5min_)[2](ROW_POS, ColPos::In::ask), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_5min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{ { (*symbols_ts_5min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[2](ROW_POS, ColPos::In::open), (*symbols_5min_)[2](ROW_POS, ColPos::In::high), (*symbols_5min_)[2](ROW_POS, ColPos::In::low), (*symbols_5min_)[2](ROW_POS, ColPos::In::close), (*symbols_5min_)[2](ROW_POS, ColPos::In::volume), (*symbols_5min_)[2](ROW_POS, ColPos::In::alt1), (*symbols_5min_)[2](ROW_POS, ColPos::In::alt2), (*symbols_5min_)[2](ROW_POS, ColPos::In::alt3) },
			(*symbols_5min_)[2](ROW_POS, ColPos::In::bid), (*symbols_5min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_5min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_5min_)[2](ROW_POS, ColPos::In::ask), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_5min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_5min_)[2](ROW_POS, ColPos::In::ask_size) } }),
	latest_15min_({
		BarFullRef{ { (*symbols_ts_15min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[0](ROW_POS, ColPos::In::open), (*symbols_15min_)[0](ROW_POS, ColPos::In::high), (*symbols_15min_)[0](ROW_POS, ColPos::In::low), (*symbols_15min_)[0](ROW_POS, ColPos::In::close), (*symbols_15min_)[0](ROW_POS, ColPos::In::volume), (*symbols_15min_)[0](ROW_POS, ColPos::In::alt1), (*symbols_15min_)[0](ROW_POS, ColPos::In::alt2), (*symbols_15min_)[0](ROW_POS, ColPos::In::alt3) },
			(*symbols_15min_)[2](ROW_POS, ColPos::In::bid), (*symbols_15min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_15min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_15min_)[2](ROW_POS, ColPos::In::ask), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_15min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{ { (*symbols_ts_15min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[1](ROW_POS, ColPos::In::open), (*symbols_15min_)[1](ROW_POS, ColPos::In::high), (*symbols_15min_)[1](ROW_POS, ColPos::In::low), (*symbols_15min_)[1](ROW_POS, ColPos::In::close), (*symbols_15min_)[1](ROW_POS, ColPos::In::volume), (*symbols_15min_)[1](ROW_POS, ColPos::In::alt1), (*symbols_15min_)[1](ROW_POS, ColPos::In::alt2), (*symbols_15min_)[1](ROW_POS, ColPos::In::alt3) },
			(*symbols_15min_)[2](ROW_POS, ColPos::In::bid), (*symbols_15min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_15min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_15min_)[2](ROW_POS, ColPos::In::ask), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_15min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{ { (*symbols_ts_15min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[2](ROW_POS, ColPos::In::open), (*symbols_15min_)[2](ROW_POS, ColPos::In::high), (*symbols_15min_)[2](ROW_POS, ColPos::In::low), (*symbols_15min_)[2](ROW_POS, ColPos::In::close), (*symbols_15min_)[2](ROW_POS, ColPos::In::volume), (*symbols_15min_)[2](ROW_POS, ColPos::In::alt1), (*symbols_15min_)[2](ROW_POS, ColPos::In::alt2), (*symbols_15min_)[2](ROW_POS, ColPos::In::alt3) },
			(*symbols_15min_)[2](ROW_POS, ColPos::In::bid), (*symbols_15min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_15min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_15min_)[2](ROW_POS, ColPos::In::ask), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_15min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_15min_)[2](ROW_POS, ColPos::In::ask_size) } }),
	latest_1hr_({
		BarRef{ (*symbols_ts_1hr_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[0](ROW_POS, ColPos::In::open), (*symbols_1hr_)[0](ROW_POS, ColPos::In::high), (*symbols_1hr_)[0](ROW_POS, ColPos::In::low), (*symbols_1hr_)[0](ROW_POS, ColPos::In::close), (*symbols_1hr_)[0](ROW_POS, ColPos::In::volume), (*symbols_1hr_)[0](ROW_POS, ColPos::In::alt1), (*symbols_1hr_)[0](ROW_POS, ColPos::In::alt2), (*symbols_1hr_)[0](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_1hr_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[1](ROW_POS, ColPos::In::open), (*symbols_1hr_)[1](ROW_POS, ColPos::In::high), (*symbols_1hr_)[1](ROW_POS, ColPos::In::low), (*symbols_1hr_)[1](ROW_POS, ColPos::In::close), (*symbols_1hr_)[1](ROW_POS, ColPos::In::volume), (*symbols_1hr_)[1](ROW_POS, ColPos::In::alt1), (*symbols_1hr_)[1](ROW_POS, ColPos::In::alt2), (*symbols_1hr_)[1](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_1hr_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[2](ROW_POS, ColPos::In::open), (*symbols_1hr_)[2](ROW_POS, ColPos::In::high), (*symbols_1hr_)[2](ROW_POS, ColPos::In::low), (*symbols_1hr_)[2](ROW_POS, ColPos::In::close), (*symbols_1hr_)[2](ROW_POS, ColPos::In::volume), (*symbols_1hr_)[2](ROW_POS, ColPos::In::alt1), (*symbols_1hr_)[2](ROW_POS, ColPos::In::alt2), (*symbols_1hr_)[2](ROW_POS, ColPos::In::alt3) } }),
	latest_4hr_({
		BarRef{ (*symbols_ts_4hr_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[0](ROW_POS, ColPos::In::open), (*symbols_4hr_)[0](ROW_POS, ColPos::In::high), (*symbols_4hr_)[0](ROW_POS, ColPos::In::low), (*symbols_4hr_)[0](ROW_POS, ColPos::In::close), (*symbols_4hr_)[0](ROW_POS, ColPos::In::volume), (*symbols_4hr_)[0](ROW_POS, ColPos::In::alt1), (*symbols_4hr_)[0](ROW_POS, ColPos::In::alt2), (*symbols_4hr_)[0](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_4hr_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[1](ROW_POS, ColPos::In::open), (*symbols_4hr_)[1](ROW_POS, ColPos::In::high), (*symbols_4hr_)[1](ROW_POS, ColPos::In::low), (*symbols_4hr_)[1](ROW_POS, ColPos::In::close), (*symbols_4hr_)[1](ROW_POS, ColPos::In::volume), (*symbols_4hr_)[1](ROW_POS, ColPos::In::alt1), (*symbols_4hr_)[1](ROW_POS, ColPos::In::alt2), (*symbols_4hr_)[1](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_4hr_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[2](ROW_POS, ColPos::In::open), (*symbols_4hr_)[2](ROW_POS, ColPos::In::high), (*symbols_4hr_)[2](ROW_POS, ColPos::In::low), (*symbols_4hr_)[2](ROW_POS, ColPos::In::close), (*symbols_4hr_)[2](ROW_POS, ColPos::In::volume), (*symbols_4hr_)[2](ROW_POS, ColPos::In::alt1), (*symbols_4hr_)[2](ROW_POS, ColPos::In::alt2), (*symbols_4hr_)[2](ROW_POS, ColPos::In::alt3) } }),
	latest_1d_({
		BarRef{ (*symbols_ts_1d_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[0](ROW_POS, ColPos::In::open), (*symbols_1d_)[0](ROW_POS, ColPos::In::high), (*symbols_1d_)[0](ROW_POS, ColPos::In::low), (*symbols_1d_)[0](ROW_POS, ColPos::In::close), (*symbols_1d_)[0](ROW_POS, ColPos::In::volume), (*symbols_1d_)[0](ROW_POS, ColPos::In::alt1), (*symbols_1d_)[0](ROW_POS, ColPos::In::alt2), (*symbols_1d_)[0](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_1d_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[1](ROW_POS, ColPos::In::open), (*symbols_1d_)[1](ROW_POS, ColPos::In::high), (*symbols_1d_)[1](ROW_POS, ColPos::In::low), (*symbols_1d_)[1](ROW_POS, ColPos::In::close), (*symbols_1d_)[1](ROW_POS, ColPos::In::volume), (*symbols_1d_)[1](ROW_POS, ColPos::In::alt1), (*symbols_1d_)[1](ROW_POS, ColPos::In::alt2), (*symbols_1d_)[1](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_1d_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[2](ROW_POS, ColPos::In::open), (*symbols_1d_)[2](ROW_POS, ColPos::In::high), (*symbols_1d_)[2](ROW_POS, ColPos::In::low), (*symbols_1d_)[2](ROW_POS, ColPos::In::close), (*symbols_1d_)[2](ROW_POS, ColPos::In::volume), (*symbols_1d_)[2](ROW_POS, ColPos::In::alt1), (*symbols_1d_)[2](ROW_POS, ColPos::In::alt2), (*symbols_1d_)[2](ROW_POS, ColPos::In::alt3) } }),
	latest_1w_({
		BarRef{ (*symbols_ts_1w_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[0](ROW_POS, ColPos::In::open), (*symbols_1w_)[0](ROW_POS, ColPos::In::high), (*symbols_1w_)[0](ROW_POS, ColPos::In::low), (*symbols_1w_)[0](ROW_POS, ColPos::In::close), (*symbols_1w_)[0](ROW_POS, ColPos::In::volume), (*symbols_1w_)[0](ROW_POS, ColPos::In::alt1), (*symbols_1w_)[0](ROW_POS, ColPos::In::alt2), (*symbols_1w_)[0](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_1w_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[1](ROW_POS, ColPos::In::open), (*symbols_1w_)[1](ROW_POS, ColPos::In::high), (*symbols_1w_)[1](ROW_POS, ColPos::In::low), (*symbols_1w_)[1](ROW_POS, ColPos::In::close), (*symbols_1w_)[1](ROW_POS, ColPos::In::volume), (*symbols_1w_)[1](ROW_POS, ColPos::In::alt1), (*symbols_1w_)[1](ROW_POS, ColPos::In::alt2), (*symbols_1w_)[1](ROW_POS, ColPos::In::alt3) },
		BarRef{ (*symbols_ts_1w_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[2](ROW_POS, ColPos::In::open), (*symbols_1w_)[2](ROW_POS, ColPos::In::high), (*symbols_1w_)[2](ROW_POS, ColPos::In::low), (*symbols_1w_)[2](ROW_POS, ColPos::In::close), (*symbols_1w_)[2](ROW_POS, ColPos::In::volume), (*symbols_1w_)[2](ROW_POS, ColPos::In::alt1), (*symbols_1w_)[2](ROW_POS, ColPos::In::alt2), (*symbols_1w_)[2](ROW_POS, ColPos::In::alt3) } }),

	// TODO some form of snapshots_ initialization that dynamically handles different MAX_ACTIVE_SYMBOLS sizes?
	snapshots_({
		Snapshot{
				{ 0, 0.0, 0.0, 0, 0 },
				(*symbols_1min_)[0](ROW_POS, ColPos::In::close),
				latest_1min_[0],
				latest_5min_[0],
				latest_15min_[0],
				latest_1hr_[0] },
		Snapshot{
				{ 0, 0.0, 0.0, 0, 0 },
				(*symbols_1min_)[1](ROW_POS, ColPos::In::close),
				latest_1min_[1],
				latest_5min_[1],
				latest_15min_[1],
				latest_1hr_[1] },
		Snapshot{
				{ 0, 0.0, 0.0, 0, 0 },
				(*symbols_1min_)[2](ROW_POS, ColPos::In::close),
				latest_1min_[2],
				latest_5min_[2],
				latest_15min_[2],
				latest_1hr_[2] } })
{
	assert(dc_initialized == false);
	dc_initialized = true;
	//zero_symbols_data(symbols_data_);
	//zero_processed_data(processed_data_);

	symbols_pos_.clear();
	symbols_pos_rev_.clear();

	assert(symbols_pos_.size() == symbols_pos_rev_.size());
}

DataController::DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update on_update) :
	DataController(mode, on_snapshot, on_update, fn_update_outputs())
{
}

DataController::DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update_outputs on_update_outputs) :
	DataController(mode, on_snapshot, fn_update(), on_update_outputs)
{
}

DataController::DataController(const AGMode mode) :
	DataController(mode, fn_snapshot(), fn_update(), fn_update_outputs())
{
}

DataController::~DataController()
{
	if (latest_trades_ != nullptr)
	{
		delete latest_trades_;
		latest_trades_ = nullptr;
	}
	if (latest_quotes_ != nullptr)
	{
		delete latest_quotes_;
		latest_quotes_ = nullptr;
	}

	if (symbols_outputs_ != nullptr)
	{
		delete symbols_outputs_;
		symbols_outputs_ = nullptr;
	}

	// cleanup raw ts xarrays
	if (symbols_ts_1min_ != nullptr)
	{
		delete symbols_ts_1min_;
		symbols_ts_1min_ = nullptr;
	}
	if (symbols_ts_5min_ != nullptr)
	{
		delete symbols_ts_5min_;
		symbols_ts_5min_ = nullptr;
	}
	if (symbols_ts_15min_ != nullptr)
	{
		delete symbols_ts_15min_;
		symbols_ts_15min_ = nullptr;
	}
	if (symbols_ts_1hr_ != nullptr)
	{
		delete symbols_ts_1hr_;
		symbols_ts_1hr_ = nullptr;
	}
	if (symbols_ts_4hr_ != nullptr)
	{
		delete symbols_ts_4hr_;
		symbols_ts_4hr_ = nullptr;
	}
	if (symbols_ts_1d_ != nullptr)
	{
		delete symbols_ts_1d_;
		symbols_ts_1d_ = nullptr;
	}
	if (symbols_ts_1w_ != nullptr)
	{
		delete symbols_ts_1w_;
		symbols_ts_1w_ = nullptr;
	}

    // cleanup raw xarrays
	if (symbols_1min_ != nullptr)
	{
		delete symbols_1min_;
		symbols_1min_ = nullptr;
	}
	if (symbols_5min_ != nullptr)
	{
		delete symbols_5min_;
		symbols_5min_ = nullptr;
	}
	if (symbols_15min_ != nullptr)
	{
		delete symbols_15min_;
		symbols_15min_ = nullptr;
	}
	if (symbols_1hr_ != nullptr)
	{
		delete symbols_1hr_;
		symbols_1hr_ = nullptr;
	}
	if (symbols_4hr_ != nullptr)
	{
		delete symbols_4hr_;
		symbols_4hr_ = nullptr;
	}
	if (symbols_1d_ != nullptr)
	{
		delete symbols_1d_;
		symbols_1d_ = nullptr;
	}
	if (symbols_1w_ != nullptr)
	{
		delete symbols_1w_;
		symbols_1w_ = nullptr;
	}

	// cleanup processed xarrays
	if (proc_symbols_1min_ != nullptr)
	{
		delete proc_symbols_1min_;
		proc_symbols_1min_ = nullptr;
	}
	if (proc_symbols_5min_ != nullptr)
	{
		delete proc_symbols_5min_;
		proc_symbols_5min_ = nullptr;
	}
	if (proc_symbols_15min_ != nullptr)
	{
		delete proc_symbols_15min_;
		proc_symbols_15min_ = nullptr;
	}
	if (proc_symbols_1hr_ != nullptr)
	{
		delete proc_symbols_1hr_;
		proc_symbols_1hr_ = nullptr;
	}
	if (proc_symbols_4hr_ != nullptr)
	{
		delete proc_symbols_4hr_;
		proc_symbols_4hr_ = nullptr;
	}
	if (proc_symbols_1d_ != nullptr)
	{
		delete proc_symbols_1d_;
		proc_symbols_1d_ = nullptr;
	}
	if (proc_symbols_1w_ != nullptr)
	{
		delete proc_symbols_1w_;
		proc_symbols_1w_ = nullptr;
	}
}


void DataController::shift(const ShiftTriggers& triggers, const size_t& pos)
{
	// (1min bar is filled with latest data)

	//const timestamp_t& next_ts = triggers.next_ts;
	
	if (triggers.flush10sec)
	{
		//const auto& symbol = symbols_pos_rev_[pos];
		constexpr ptrdiff_t shift_pos = 1;

		// // TODO copy on stack or heap?? (otherwise allocated on the stack?!)
		// const auto data_copy = xt::roll(..., shift_pos, 0);

		if (triggers.flush1min)  // 1min
		{
			// shift down
			auto& data_1min = (*symbols_1min_)[pos];
			if (DEBUG_PRINT_DATA)
			{
				std::cout << "1min data (before):" << std::endl << data_1min << std::endl;
				//std::cout << "1min latest ts:" << static_cast<timestamp_t>(latest_1min_[pos].timestamp) << std::endl;
				//std::cout << "1min latest close:" << latest_1min_[pos].close << std::endl;
				//std::cout << "1min raw shape: " << data_1min.shape() << std::endl;
			}
			const auto data_copy = xt::roll(data_1min, shift_pos, 0);
			data_1min = data_copy;

			auto& ts_1min = (*symbols_ts_1min_)[pos];
			const auto ts_copy = xt::roll(ts_1min, shift_pos, 0);
			ts_1min = ts_copy;
		}
		if (triggers.flush5min)  // 5min
		{
			// shift down
			auto& data_5min = (*symbols_5min_)[pos];
			const auto data_copy = xt::roll(data_5min, shift_pos, 0);
			data_5min = data_copy;

			auto& ts_5min = (*symbols_ts_5min_)[pos];
			const auto ts_copy = xt::roll(ts_5min, shift_pos, 0);
			ts_5min = ts_copy;
			
			if (DEBUG_PRINT_DATA)
			{
				std::cout << "5min data:" << std::endl << data_5min << std::endl;
			}
		}

		if (triggers.flush15min)  // 15min
		{
			// shift down
			auto& data_15min = (*symbols_15min_)[pos];
			const auto data_copy = xt::roll(data_15min, shift_pos, 0);
			data_15min = data_copy;

			auto& ts_15min = (*symbols_ts_15min_)[pos];
			const auto ts_copy = xt::roll(ts_15min, shift_pos, 0);
			ts_15min = ts_copy;

			if (DEBUG_PRINT_DATA)
			{
				std::cout << "15min data:" << std::endl << data_15min << std::endl;
			}
		}

		if (triggers.flush1hr)  // 1hr
		{
			// shift down
			auto& data_1hr = (*symbols_1hr_)[pos];
			const auto data_copy = xt::roll(data_1hr, shift_pos, 0);
			data_1hr = data_copy;

			auto& ts_1hr = (*symbols_ts_1hr_)[pos];
			const auto ts_copy = xt::roll(ts_1hr, shift_pos, 0);
			ts_1hr = ts_copy;
			
			if (DEBUG_PRINT_DATA)
			{
				std::cout << "1hr data:" << std::endl << data_1hr << std::endl;
			}
		}
		if (triggers.flush4hr)  // 4hr
		{
			// copy into symbols_data_
			// shift down
			auto& data_4hr = (*symbols_4hr_)[pos];
			const auto data_copy = xt::roll(data_4hr, shift_pos, 0);
			data_4hr = data_copy;

			auto& ts_4hr = (*symbols_ts_4hr_)[pos];
			const auto ts_copy = xt::roll(ts_4hr, shift_pos, 0);
			ts_4hr = ts_copy;
			
			if (DEBUG_PRINT_DATA)
			{
				std::cout << "4hr data:" << std::endl << data_4hr << std::endl;
			}
		}
	}
}


void DataController::do_update(const ShiftTriggers& triggers, const size_t& pos)
{
	if (triggers.flush1min)
	{
		const auto& symbol = symbols_pos_rev_[pos];

		// TODO only update outputs if on_update_outputs_ is bound?
		auto& symbol_outputs = (*symbols_outputs_)[pos];

		// TODO run_alts on all timeframes???
		run_alts((*symbols_1min_)[pos], symbol, 1, (*symbols_ts_1min_)[pos]);

		// preprocess all timeframes now...
		// process data (returned in ASC order, opposite of input):
		run_preprocess((*proc_symbols_1min_)[pos], symbol_outputs, symbol, 1, (*symbols_ts_1min_)[pos], (*symbols_1min_)[pos]);
		run_preprocess((*proc_symbols_5min_)[pos], symbol_outputs, symbol, 5, (*symbols_ts_5min_)[pos], (*symbols_5min_)[pos]);
		run_preprocess((*proc_symbols_15min_)[pos], symbol_outputs, symbol, 15, (*symbols_ts_15min_)[pos], (*symbols_15min_)[pos]);
		run_preprocess((*proc_symbols_1hr_)[pos], symbol_outputs, symbol, 60, (*symbols_ts_1hr_)[pos], (*symbols_1hr_)[pos]);
		run_preprocess((*proc_symbols_4hr_)[pos], symbol_outputs, symbol, 240, (*symbols_ts_4hr_)[pos], (*symbols_4hr_)[pos]);

		// notify AccountController (or other listener)
		if (on_update_ || on_update_outputs_)
		{
			/* // make sure
			while ((*latest_trades_)[pos].size() > NUM_TRADES)
			{
				// TODO use circular buffer instead
				// trim the latest trades to NUM_TRADES (~15k) trades
				(*latest_trades_)[pos].pop();
			}

			while ((*latest_quotes_)[pos].size() > NUM_QUOTES)
			{
				// TODO use circular buffer instead
				// trim the latest quotes to NUM_QUOTES (~15k) quotes
				(*latest_quotes_)[pos].pop();
			}*/

			// TODO move some of this onto the heap? or maybe pass a view type into the callbacks?
			
			//copy raw data into a merged format passed to on_update
			const xtensor_raw data = xt::stack(xt::xtuple(
				xt::view((*symbols_1min_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all()),
				xt::view((*symbols_5min_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all()),
				xt::view((*symbols_15min_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all()),
				xt::view((*symbols_1hr_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all()),
				xt::view((*symbols_4hr_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all()),
				xt::view((*symbols_1d_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all()),
				xt::view((*symbols_1w_)[pos], xt::range(0, RT_REPORT_TIMESTEPS), xt::all())
			));
			//std::cout << "data.shape: " << data.shape() << std::endl;

			//copy processed data into a merged format passed to on_update
			const xtensor_processed data_processed = xt::stack(xt::xtuple(
				xt::view((*proc_symbols_1min_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all()),
				xt::view((*proc_symbols_5min_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all()),
				xt::view((*proc_symbols_15min_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all()),
				xt::view((*proc_symbols_1hr_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all()),
				xt::view((*proc_symbols_4hr_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all()),
				xt::view((*proc_symbols_1d_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all()),
				xt::view((*proc_symbols_1w_)[pos], xt::range(RT_MAX_TIMESTEPS - NUM_TIMESTEMPS, RT_MAX_TIMESTEPS), xt::all())
			));
			//std::cout << "data_processed.shape: " << data_processed.shape() << std::endl;

			if (on_update_outputs_)
				on_update_outputs_(symbol, snapshots_[pos], (*symbols_ts_1min_)[pos], data, data_processed, (*latest_quotes_)[pos], (*latest_trades_)[pos], symbol_outputs);
			else
				on_update_(symbol, snapshots_[pos], (*symbols_ts_1min_)[pos], data, data_processed, (*latest_quotes_)[pos], (*latest_trades_)[pos]);
		}

		/* // TODO remove save for verification
		if (triggers.flush15min)
		{
			xt::dump_npy("pyfiles/_out.AAPL.1min.npy", (*symbols_1min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.5min.npy", (*symbols_5min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.15min.npy", (*symbols_15min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.1hr.npy", (*symbols_1hr_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.4hr.npy", (*symbols_4hr_)[pos]);
		}*/
	}
}


void DataController::process_trade(const size_t& pos, const json& trade)
{
	// Note: assuming trade["T"] has been handled/verified
	//

	const timestamp_us_t ts = trade["t"].get<uint64_t>() / US_TO_NS;  //ns->us
	const real_t price = trade["p"].get<double>();
	const auto& vol = trade["s"].get<uint32_t>();
	const json& conds = trade.value("c", JSON_NULL);

	process_trade_finish(pos, conds, ts, price, vol);

	/*
	trade["T"]; // symbol
	trade["x"]; // Exchange ID
	trade["i"]; // trade ID
	trade["z"]; // tape (1 = NYSE, 2 = AMEX, 3 = Nasdaq)
	trade["p"]; // price
	trade["s"]; // size
	trade["c"]; // conditions
	trade["t"]; // timestamp (ns)
	trade["y"]; // Participant/Exchange Unix Timestamp (ns)
	*/
}

void DataController::process_trade_rt(const size_t& pos, const json& trade)
{
	// Note: assuming trade["ev"]/trade["sym"] has been handled/verified
	//
	
	const timestamp_us_t ts = trade["t"].get<uint64_t>() / US_TO_NS;  //ns->us
	const real_t price = trade["p"].get<double>();
	const auto& vol = trade["s"].get<uint32_t>();
	const json& conds = trade.value("c", JSON_NULL);
	
	process_trade_finish(pos, conds, ts, price, vol);

	/*
	trade["ev"]; // Event Type (T for trade)

	trade["sym"]; // symbol
	trade["x"]; // Exchange ID
	trade["i"]; // trade ID
	trade["z"]; // tape (1 = NYSE, 2 = AMEX, 3 = Nasdaq)
	trade["p"]; // price
	trade["s"]; // size
	trade["c"]; // conditions
	trade["t"]; // timestamp (ms)
	*/
}

void DataController::process_trade_data(const size_t& pos, const TradeData& trade)
{
	const auto ts = trade.trade.timestamp;

	// TODO this is a terrible way to handle conditions
	json cond_json;
	if (trade.cond[2] != TradeCondition::PLACEHOLDER)
		cond_json = json::parse("[" + std::to_string(static_cast<uint32_t>(trade.cond[0])) + "," + std::to_string(static_cast<uint32_t>(trade.cond[1])) + "," + std::to_string(static_cast<uint32_t>(trade.cond[2])) + "]");
	else if (trade.cond[1] != TradeCondition::PLACEHOLDER)
		cond_json = json::parse("[" + std::to_string(static_cast<uint32_t>(trade.cond[0])) + "," + std::to_string(static_cast<uint32_t>(trade.cond[1])) + "]");
	else if (trade.cond[0] != TradeCondition::PLACEHOLDER && trade.cond[0] != TradeCondition::Regular_Sale)
		cond_json = json::parse("[" + std::to_string(static_cast<uint32_t>(trade.cond[0])) + "]");
	else
		cond_json = JSON_NULL;
	
	process_trade_finish(pos, cond_json, ts, trade.trade.price, trade.trade.size);
}


inline void update_high_low(BarRef& bar, const real_t& price)
{
	if (price > bar.high)
		bar.high = price;
	if (bar.low == 0 || price < bar.low)
		bar.low = price;
}

inline void update_open_close(BarRef& bar, const real_t& price)
{
	bar.close = price;
	if (bar.open == 0)
	{
		// TODO should never get here?
		std::cout << "update_open_close() unexpected open=0" << std::endl;
		//std::cout << "process_trade_rt() update open/close; open=0; ts=" << ts << "; next_ts=" << next_ts << std::endl;
		bar.open = price;
		if (bar.high == 0)
			bar.high = price;
		if (bar.low == 0)
			bar.low = price;
	}
}

inline void populate_full_bar(BarFullRef& bar, const int interval_seconds, const timestamp_us_t& next_ts, const timestamp_us_t& ts, const real_t& price, const uint32_t vol, const real_t& prev_bid, const real_t& prev_ask, const uint32_t prev_bid_size, const uint32_t prev_ask_size/*, const std::array<real_t, 3> prev_alts*/)
{
	const timestamp_us_t interval_us = static_cast<timestamp_us_t>(interval_seconds) * SEC_TO_US;
	// reset this bar
	bar.timestamp = (ts / interval_us) * interval_us;  // TODO verify
	bar.ts_real = static_cast<real_t>(bar.timestamp / SEC_TO_US);
	//bar.timestamp = next_ts;
	if (bar.timestamp != next_ts && next_ts > interval_us) {
		std::cout << "populate_full_bar() Skip timestamp " << next_ts << " (to " << bar.timestamp << ")" << std::endl;
	}
	bar.volume = vol;
	bar.low = bar.high = bar.open = bar.close = price;

	// reset ask/bid high/low to latest bid/ask price
	bar.bid_high = bar.bid_low = bar.bid = prev_bid;
	bar.ask_high = bar.ask_low = bar.ask = prev_ask;

	// TODO sum?
	// TODO Note: leave ask/bid and ask_size/bid_size the same ??
	bar.bid_size = prev_bid_size;
	bar.ask_size = prev_ask_size;

	/*// copy alts
	bar.alt1 = prev_alts[0];
	bar.alt2 = prev_alts[1];
	bar.alt3 = prev_alts[2];*/
}

inline void populate_bar(BarRef& bar, const int interval_seconds, const timestamp_us_t& next_ts, const timestamp_us_t& ts, const real_t& price, const uint32_t vol)
{
	const timestamp_us_t interval_us = static_cast<timestamp_us_t>(interval_seconds) * SEC_TO_US;
	// reset this bar
	bar.timestamp = (ts / interval_us) * interval_us;  // TODO verify
	bar.ts_real = static_cast<real_t>(bar.timestamp / SEC_TO_US);
	//bar.timestamp = next_ts;
	if (bar.timestamp != next_ts && next_ts > interval_us) {
		std::cout << "populate_bar() Skip timestamp " << next_ts << " (to " << bar.timestamp << ")" << std::endl;
	}
	bar.volume = vol;
	bar.low = bar.high = bar.open = bar.close = price;
}

void DataController::process_trade_finish(const size_t& pos, const json& conds, const timestamp_us_t& ts, const real_t& price, const uint32_t vol)
{
	// flush into 1min bar
	const int interval_seconds = 60;
	const timestamp_us_t interval_us = static_cast<timestamp_us_t>(interval_seconds) * SEC_TO_US;
	timestamp_us_t& ts_step = cur_timesteps_[pos];

	bool updates_volume = false;
	bool updates_high_low = false;
	bool updates_open_close = false;
	if (!conds.is_array() || conds.empty())
	{
		if (REGULAR_TRADE_CONDITION.updates_volume)
			updates_volume = true;
		if (REGULAR_TRADE_CONDITION.updates_high_low)
			updates_high_low = true;
		if (REGULAR_TRADE_CONDITION.updates_open_close)
			updates_open_close = true;
	}
	else
	{
		for (const json& cond : conds)
		{
			const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : 0);
			if (c < CONDITIONS_LEN)
			{
				const trade_condition_t& tradeCond = CONDITIONS.at(c);
				if (tradeCond.updates_volume)
					updates_volume = true;
				//else
				//	std::cout << "tradeCond " << tradeCond.name << " skip vol update " << trade["s"].get<uint32_t>() << std::endl;
				if (tradeCond.updates_consolidated_high_low)
					updates_high_low = true;
				if (tradeCond.updates_consolidated_open_close)
					updates_open_close = true;
			}
		}
	}
	if (!updates_high_low && !updates_open_close && !updates_volume)
		return;  // no more processing to do (for aggregate generation)

	BarFullRef& bar_1min = latest_1min_[pos];
	BarFullRef& bar_5min = latest_5min_[pos];
	BarFullRef& bar_15min = latest_15min_[pos];
	BarRef& bar_1hr = latest_1hr_[pos];
	BarRef& bar_4hr = latest_4hr_[pos];
	BarRef& bar_1d = latest_1d_[pos];
	BarRef& bar_1w = latest_1w_[pos];
	
	const timestamp_us_t next_ts = ts_step + interval_us;
	if (ts >= next_ts)
	{
		const real_t prev_bid_1min = bar_1min.bid;
		const real_t prev_ask_1min = bar_1min.ask;
		const real_t prev_bid_size_1min = bar_1min.bid_size;
		const real_t prev_ask_size_1min = bar_1min.ask_size;
		const real_t prev_bid_5min = bar_5min.bid;
		const real_t prev_ask_5min = bar_5min.ask;
		const real_t prev_bid_size_5min = bar_5min.bid_size;
		const real_t prev_ask_size_5min = bar_5min.ask_size;
		const real_t prev_bid_15min = bar_15min.bid;
		const real_t prev_ask_15min = bar_15min.ask;
		const real_t prev_bid_size_15min = bar_15min.bid_size;
		const real_t prev_ask_size_15min = bar_15min.ask_size;
		//const std::array<real_t, 3> prev_alts = { 0, 0, 0 };  // { bar_1min.alt1, bar_1min.alt2, bar_1min.alt3 };

		// shift bars
		// Note: this introduces delay to the flush routine (by waiting for next timestamp before processing previous)
		//   This should be changed after moving to a faster network, then preempt end-of-interval
		const ShiftTriggers triggers(next_ts);
		if (next_ts > interval_seconds) {  // only shift if this is not the first timestep
			// call update before shifting
			do_update(triggers, pos);

			shift(triggers, pos);
		}

		// TODO consider updates_* flags in these cases?

		if (triggers.flush1min)
		{
			populate_full_bar(bar_1min, interval_seconds, next_ts, ts, price, vol, prev_bid_1min, prev_ask_1min, prev_bid_size_1min, prev_ask_size_1min/*, prev_alts*/);
		}
		if (triggers.flush5min)
		{
			populate_full_bar(bar_5min, interval_seconds * 5, next_ts, ts, price, vol, prev_bid_5min, prev_ask_5min, prev_bid_size_5min, prev_ask_size_5min);
		}
		if (triggers.flush15min)
		{
			populate_full_bar(bar_15min, interval_seconds * 15, next_ts, ts, price, vol, prev_bid_15min, prev_ask_15min, prev_bid_size_15min, prev_ask_size_15min);
		}
		if (triggers.flush1hr)
		{
			populate_bar(bar_1hr, interval_seconds * 60, next_ts, ts, price, vol);
		}
		if (triggers.flush4hr)
		{
			populate_bar(bar_4hr, interval_seconds * 240, next_ts, ts, price, vol);
		}
		// Note: never flush 1d/1w (TODO flush 1d if after hours starts? or 1w if after hours starts on friday?)
		/*if (triggers.flush1d)
			populate_bar(bar_1d, interval_seconds * 5, next_ts, ts, price, vol);
		if (triggers.flush1w)
			populate_bar(bar_1w, interval_seconds * 5, next_ts, ts, price, vol);*/

		if (triggers.flush10sec && next_ts > interval_us) {  //triggers.flush1min // if result was shifted, must update alts now...
			// Must run_alts now, that-way alt1-3 is available for snapshofts
			run_alts((*symbols_1min_)[pos], symbols_pos_rev_[pos], 1, (*symbols_ts_1min_)[pos]);
		}

		ts_step = bar_1min.timestamp;  // next_ts;
	}
	else
	{
		// append to existing bar!
		if (updates_high_low)
		{
			update_high_low(bar_1min, price);
			update_high_low(bar_5min, price);
			update_high_low(bar_15min, price);
			update_high_low(bar_1hr, price);
			update_high_low(bar_4hr, price);
			update_high_low(bar_1d, price);
			update_high_low(bar_1w, price);
		}
		if (updates_open_close)
		{
			update_open_close(bar_1min, price);
			update_open_close(bar_5min, price);
			update_open_close(bar_15min, price);
			update_open_close(bar_1hr, price);
			update_open_close(bar_4hr, price);
			update_open_close(bar_1d, price);
			update_open_close(bar_1w, price);
		}
		if (updates_volume)
		{
			bar_1min.volume += vol;
			bar_5min.volume += vol;
			bar_15min.volume += vol;
			bar_1hr.volume += vol;
			bar_4hr.volume += vol;
			bar_1d.volume += vol;
			bar_1w.volume += vol;
		}
	}
}


void DataController::process_quote(const size_t& pos, const json& quote)
{
	// Note: assuming quote["T"] has been handled/verified

	const timestamp_us_t ts = quote["t"].get<uint64_t>() / US_TO_NS;  //ns->us
	const real_t askPrice = quote["P"].get<double>();
	const auto& askSize = quote["S"].get<uint32_t>();
	const real_t bidPrice = quote["p"].get<double>();
	const auto& bidSize = quote["s"].get<uint32_t>();

	process_quote_finish(pos, ts, askPrice, askSize, bidPrice, bidSize);

	/*
	quote["T"]; // symbol
	quote["x"]; // Bid Exchange ID
	quote["p"]; // Bid Price
	quote["s"]; // Bid Size
	quote["X"]; // Ask Exchange ID
	quote["P"]; // Ask Price
	quote["S"]; // Ask Size
	quote["c"]; // The conditions
	quote["t"]; // timestamp (ms)
	quote["y"]; // Participant/Exchange Unix Timestamp (ns)
	quote["z"]; // tape (1 = NYSE, 2 = AMEX, 3 = Nasdaq)
	*/
}

void DataController::process_quote_rt(const size_t& pos, const json& quote)
{
	// Note: assuming quote["ev"]/quote["sym"] has been handled/verified

	const timestamp_us_t ts = quote["t"].get<uint64_t>() / US_TO_NS;
	const real_t askPrice = quote["ap"].get<double>();
	const auto& askSize = quote["as"].get<uint32_t>();
	const real_t bidPrice = quote["bp"].get<double>();
	const auto& bidSize = quote["bs"].get<uint32_t>();

	process_quote_finish(pos, ts, askPrice, askSize, bidPrice, bidSize);
	
	/*
	quote["ev"]; // Event Type (Q for quote)

	quote["sym"]; // symbol
	quote["bx"]; // Bid Exchange ID
	quote["bp"]; // Bid Price
	quote["bs"]; // Bid Size
	quote["ax"]; // Ask Exchange ID
	quote["ap"]; // Ask Price
	quote["as"]; // Ask Size
	quote["c"]; // The condition
	quote["t"]; // The Timestamp in Unix MS
	quote["z"]; // tape (1 = NYSE, 2 = AMEX, 3 = Nasdaq)
	*/
}

void DataController::process_quote_data(const size_t& pos, const QuoteData& quote)
{
	process_quote_finish(pos, quote.quote.timestamp, quote.quote.ask, quote.quote.ask_size, quote.quote.bid, quote.quote.bid_size);
}


inline void update_bid_ask(BarFullRef& bar, const real_t& bid, const real_t& ask, const uint32_t bid_size, const uint32_t ask_size)
{
	// append to existing bar!
	if (ask > bar.ask_high)
		bar.ask_high = ask;
	if (bar.ask_low == 0 || ask < bar.ask_low)
		bar.ask_low = ask;
	if (bid > bar.bid_high)
		bar.bid_high = bid;
	if (bar.bid_low == 0 || bid < bar.bid_low)
		bar.bid_low = bid;
	bar.ask = ask;
	bar.bid = bid;
	bar.ask_size = ask_size;
	bar.bid_size = bid_size;
}

inline void populate_next_full_bar(BarFullRef& bar, const timestamp_us_t& interval_us, const timestamp_us_t& next_ts, const timestamp_us_t& ts, const real_t& price, const real_t& bid, const real_t& ask, const uint32_t bid_size, const uint32_t ask_size/*, const std::array<real_t, 3>& prev_alts*/)
{
	// reset this bar
	bar.timestamp = static_cast<timestamp_us_t>(ts / interval_us) * interval_us;  // TODO verify
	bar.ts_real = static_cast<real_t>(bar.timestamp / SEC_TO_US);
	//bar.timestamp = next_ts;
	if (bar.timestamp != next_ts && next_ts > interval_us) {
		std::cout << "populate_next_full_bar() Skip timestamp " << next_ts << " (to " << bar.timestamp << ")" << std::endl;
	}
	bar.volume = 0;  // so far, 0 volume in this bar
	bar.low = bar.high = bar.open = bar.close = price;  // TODO copy last close price ?

	// set bid/ask and reset the high/lows
	bar.ask = bar.ask_high = bar.ask_low = ask;
	bar.bid = bar.bid_high = bar.bid_low = bid;
	bar.ask_size = ask_size;
	bar.bid_size = bid_size;

	/*// copy alts
	bar.alt1 = prev_alts[0];
	bar.alt2 = prev_alts[1];
	bar.alt3 = prev_alts[2];*/
}

inline void populate_next_bar(BarRef& bar, const timestamp_us_t& interval_us, const timestamp_us_t& next_ts, const timestamp_us_t& ts, const real_t& price)
{
	// reset this bar
	bar.timestamp = static_cast<timestamp_us_t>(ts / interval_us) * interval_us;  // TODO verify
	bar.ts_real = static_cast<real_t>(bar.timestamp / SEC_TO_US);
	//bar.timestamp = next_ts;
	if (bar.timestamp != next_ts && next_ts > interval_us) {
		std::cout << "populate_next_bar() Skip timestamp " << next_ts << " (to " << bar.timestamp << ")" << std::endl;
	}
	bar.volume = 0;  // so far, 0 volume in this bar
	bar.low = bar.high = bar.open = bar.close = price;  // TODO copy last close price ?
}

void DataController::process_quote_finish(const size_t& pos, const timestamp_us_t& ts, const real_t& askPrice, const uint32_t& askSize, const real_t& bidPrice, const uint32_t& bidSize)
{
	// flush into 1min bar
	constexpr int interval_seconds = 60;
	constexpr timestamp_us_t interval_us = static_cast<timestamp_us_t>(interval_seconds) * SEC_TO_US;

	timestamp_us_t& ts_step = cur_timesteps_[pos];

	/*
	// TODO consider condition?
	bool updates_volume = false;
	bool updates_high_low = false;
	bool updates_open_close = false;
	if (!quote.contains("c") || !quote["c"].is_array() || quote["c"].empty())
	{
		if (REGULAR_TRADE_CONDITION.updates_volume)
			updates_volume = true;
		if (REGULAR_TRADE_CONDITION.updates_high_low)
			updates_high_low = true;
		if (REGULAR_TRADE_CONDITION.updates_open_close)
			updates_open_close = true;
	}
	else
	{
		for (const json& cond : quote["c"])
		{
			const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : 0);
			if (c < CONDITIONS_LEN)
			{
				const trade_condition_t& tradeCond = CONDITIONS.at(c);
				if (tradeCond.updates_volume)
					updates_volume = true;
				//else
				//	std::cout << "tradeCond " << tradeCond.name << " skip vol update " << quote["s"].get<uint32_t>() << std::endl;
				if (tradeCond.updates_consolidated_high_low)
					updates_high_low = true;
				if (tradeCond.updates_consolidated_open_close)
					updates_open_close = true;
			}
		}
	}
	if (!updates_high_low && !updates_open_close && !updates_volume)
		return;  // no more processing to do (for aggregate generation)
	*/

	// update NBBO
	snapshots_[pos].nbbo = { ts, bidPrice, askPrice, bidSize, askSize };

	// always notify AccountController first (or other listener) when bid/ask changes
	if (on_snapshot_)
		on_snapshot_(symbols_pos_rev_[pos], snapshots_[pos]);

	BarFullRef& bar_1min = latest_1min_[pos];
	BarFullRef& bar_5min = latest_5min_[pos];
	BarFullRef& bar_15min = latest_15min_[pos];
	BarRef& bar_1hr = latest_1hr_[pos];
	BarRef& bar_4hr = latest_4hr_[pos];
	//BarRef& bar_1d = latest_1d_[pos];
	//BarRef& bar_1w = latest_1w_[pos];

	const timestamp_us_t next_ts = ts_step + interval_us;
	if (ts >= next_ts)
	{
		// copy last close price into new bars
		const real_t price_1min = bar_1min.close;
		const real_t price_5min = bar_5min.close;
		const real_t price_15min = bar_15min.close;
		//const std::array<real_t, 3> prev_alts = {0, 0, 0};  // { bar_1min.alt1, bar_1min.alt2, bar_1min.alt3 };

		const ShiftTriggers triggers(next_ts);

		// shift bars
		// Note: this introduces delay to the flush routine (by waiting for next timestamp before processing previous)
		//   This should be changed after moving to a faster network, then preempt end-of-interval
		if (next_ts > interval_us) {  // only shift if this is not the first timestep
			// call update before shifting
			do_update(triggers, pos);
			
			shift(triggers, pos);
		}

		if (triggers.flush1min)
		{
			populate_next_full_bar(bar_1min, interval_us, next_ts, ts, price_1min, bidPrice, askPrice, bidSize, askSize/*, prev_alts*/);
		}
		if (triggers.flush5min)
		{
			populate_next_full_bar(bar_5min, interval_us * 5, next_ts, ts, price_5min, bidPrice, askPrice, bidSize, askSize);
		}
		if (triggers.flush15min)
		{
			populate_next_full_bar(bar_15min, interval_us * 15, next_ts, ts, price_15min, bidPrice, askPrice, bidSize, askSize);
		}
		if (triggers.flush1hr)
		{
			populate_next_bar(bar_1hr, interval_us * 60, next_ts, ts, price_1min);
		}
		if (triggers.flush4hr)
		{
			populate_next_bar(bar_4hr, interval_us * 240, next_ts, ts, price_1min);
		}
		// Note: never flush 1d/1w (TODO flush 1d if after hours starts? or 1w if after hours starts on friday?)
		/*if (triggers.flush1d)
			populate_next_bar(bar_1d, interval_us, next_ts, ts, price);
		if (triggers.flush1w)
			populate_next_bar(bar_1w, interval_us, next_ts, ts, price);*/

		if (triggers.flush10sec && next_ts > interval_us) {  //triggers.flush1min // if result was shifted, must update alts now...
			// Must run_alts now, that-way alt1-3 is available for snapshofts
			// Note: since this functions at this point, maybe do not copy prev_alts?
			run_alts((*symbols_1min_)[pos], symbols_pos_rev_[pos], 1, (*symbols_ts_1min_)[pos]);
		}

		ts_step = bar_1min.timestamp;  // next_ts;
	}
	else
	{
		// append to existing bar!
		update_bid_ask(bar_1min, bidPrice, askPrice, bidSize, askSize);
		update_bid_ask(bar_5min, bidPrice, askPrice, bidSize, askSize);
		update_bid_ask(bar_15min, bidPrice, askPrice, bidSize, askSize);
	}
}

const Snapshot& DataController::getSnapshot(const Symbol& symbol) const
{
	const auto& pos = symbols_pos_.at(symbol);
	return snapshots_[pos];
}

void DataController::onPayloads(const json& payloads)
{
	if (!payloads.is_array() || payloads.empty())
		return; // payload must be a non-empty array

	//std::cout << "Number of payloads: " << payloads.size() << std::endl;

	for (const auto& payload : payloads)
	{
		const auto& event = payload["ev"].get<std::string>();
		if (strcmp(event.c_str(), "T") == 0)  // TODO optimize? (compare character)
			onTradePayload(payload);
		else if (strcmp(event.c_str(), "Q") == 0)  // TODO optimize? (compare character)
			onNbboPayload(payload);
	}
}

// TODO use data adapter in main_, and pass a common object here
void DataController::onTradePayload(const json& trade)
{
	//std::cout << "onTradePayload() " << trade << std::endl;
	
	const auto& ticker = trade["sym"].get<std::string>();
	const Symbol& symbol = Symbol::get_symbol(ticker);  // TODO
	const auto& pos = symbols_pos_[symbol];

	// TODO this is redundant, ideally use trade_data to make a process_ call:
	TradeData trade_data;
	try
	{
		adapter::payload_rt_to_trade_data(trade_data, trade);
	}
	catch (std::exception const& e)
	{
		std::cout << "onTradePayload() parse error: " << e.what() << std::endl;
		return;
	}

	// Note: we must insert the trade into the latest_trades_ queue, before calling process_
	//       (because process_ flushes the bar, and the queue is expected to be up to date at this time...)
	// TODO insert in place?
	(*latest_trades_)[pos].emplace(trade_data);

	// TODO could use process_trade_data, but it needs improvements?

	process_trade_rt(pos, trade);

}

// TODO use data adapter in main_, and pass a common object here
void DataController::onNbboPayload(const json& quote)
{
	//std::cout << "onNbboPayload() " << quote << std::endl;

	const auto& ticker = quote["sym"].get<std::string>();
	const Symbol& symbol = Symbol::get_symbol(ticker);  // TODO
	const auto& pos = symbols_pos_[symbol];

	// parse into QuoteData for processing
	QuoteData quote_data;
	try
	{
		adapter::payload_rt_to_quote_data(quote_data, quote);
	}
	catch (std::exception const& e)
	{
		std::cout << "onNbboPayload() parse error: " << e.what() << std::endl;
		return;
	}

	// Note: we must insert the trade into the latest_trades_ queue, before calling process_
	//       (because process_ flushes the bar, and the queue is expected to be up to date at this time...)
	// TODO insert in place?
	(*latest_quotes_)[pos].emplace(quote_data);  // TODO emplace, or is that a move?

	process_quote_data(pos, quote_data);
	
}

void DataController::startSimulation(std::chrono::seconds start_ts)
{
	// default to 3 hours
	startSimulation(start_ts, std::chrono::minutes(180));
}

void DataController::startSimulation(std::chrono::seconds start_ts, std::chrono::minutes num_minutes)
{
	// assuming initSymbol was called with this same timestamp:
	//  - data should have been downloaded up to start_ts
	//  - this method will begin downloading data after start_ts, and call the SimAccountController? callbacks
	if (mode_ != AGMode::BACK_TEST)
	{
		throw std::logic_error("mode_ must be BACK_TEST");
	}

	// now download the most recent quotes, and most recent trades
	timestamp_us_t req_start_ts = static_cast<timestamp_us_t>(start_ts.count()) * SEC_TO_US;
	timestamp_us_t max_end_ts = req_start_ts + (MIN_TO_US * num_minutes.count());
	timestamp_us_t end_ts = 0;

	// TODO handle multiple symbols
	if (symbols_pos_.size() > 1)
		throw std::logic_error("Only a single symbol is supported");
	
	// download trades/quotes into these staging arrays, then pull and push into the DataController symbol queues for processing
	std::queue<QuoteData> tmp_quotes;
	std::queue<TradeData> tmp_trades;
	
	do
	{
		// TODO verify:
		// in each iteration, restart end_ts, that-way we use date range from first 50k trades of first symbol
		end_ts = 0;

		// loop symbols, back-test each symbol
		for (const auto& pair : symbols_pos_)
		{
			const Symbol& symbol = pair.first;
			const size_t& pos = pair.second;

			// make sure staging arrays are clear after each iteration
			if (!tmp_quotes.empty())
				throw std::logic_error("tmp_quotes staging queue is not empty");
			if (!tmp_trades.empty())
				throw std::logic_error("tmp_trades staging queue is not empty");

			if (!end_ts)
			{
				// grab the last 50k quotes, then grab the trades up to the final timestamp...
				PolygonIoAdapter::getQuoteHistoryAfter(tmp_quotes, symbol.symbol, req_start_ts, 50000);
				if (DEBUG_PRINT_REQUESTS)
					std::cout << "tmp_quotes[start] size " << tmp_quotes.size() << std::endl;

				// TODO continue if quotes empty?
				// last ts
				if (!tmp_quotes.empty())
					end_ts = tmp_quotes.back().quote.timestamp;
			}
			else
			{
				// the next symbol will use the range identified in first symbol...
				PolygonIoAdapter::getQuoteHistoryBetween(tmp_quotes, symbol.symbol, req_start_ts, end_ts);
				if (DEBUG_PRINT_REQUESTS)
					std::cout << "tmp_quotes[range] size " << tmp_quotes.size() << std::endl;
			}

			// fetch trades
			PolygonIoAdapter::getTradeHistoryBetween(tmp_trades, symbol.symbol, req_start_ts, end_ts);
			if (DEBUG_PRINT_REQUESTS)
				std::cout << "tmp_trades[range] size " << tmp_trades.size() << std::endl;
			
			// flush the results now...
			while (!tmp_quotes.empty())
			{
				// pop first quote
				const QuoteData quote = tmp_quotes.front();
				tmp_quotes.pop();

				// pop from trades, and flush those until next trade is later than this quote
				while (!tmp_trades.empty() && tmp_trades.front().trade.timestamp < quote.quote.timestamp)
				{
					if (tmp_trades.front().trade.timestamp > max_end_ts) {
						tmp_trades.pop();
						continue;
					}

					// first push this trade into ctrl queue, that-way it's available for the callbacks
					(*latest_trades_)[pos].emplace(tmp_trades.front());

					process_trade_data(pos, tmp_trades.front());
					tmp_trades.pop();
				}

				if (quote.quote.timestamp > max_end_ts) {
					continue;
				}

				// first push this quote into ctrl queue, that-way it's available for the callbacks
				(*latest_quotes_)[pos].emplace(quote);

				// flush the quote that was popped
				process_quote_data(pos, quote);
			}
			// flush remaining trades (they have timestamp > than the last handled quote timestamp)
			while (!tmp_trades.empty())
			{
				if (tmp_trades.front().trade.timestamp > max_end_ts) {
					tmp_trades.pop();
					continue;
				}

				// first push this trade into ctrl queues, that-way it's available for the callbacks
				(*latest_trades_)[pos].emplace(tmp_trades.front());

				process_trade_data(pos, tmp_trades.front());
				tmp_trades.pop();
			}
		}
		
		// move req_start_ts up
		req_start_ts = end_ts;
		end_ts = 0;

		if (req_start_ts >= max_end_ts)
			break;  // TODO more ideal way of doing this?

	} while (true);

}

void DataController::initSymbol(const Symbol& symbol)
{
	// TODO
	// TODO shift to previous minute (fe. see ::floor)
	// const auto now = std::chrono::system_clock::now();
	const auto now = std::chrono::system_clock::now();  // -std::chrono::seconds(3 * 86400 + 5 * 3600);
	initSymbol(symbol, std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()));
}

void DataController::initSymbol(const Symbol& symbol, std::chrono::seconds ts)
{
    if (symbols_pos_.find(symbol) == symbols_pos_.end()) {
        // must have another slot
        assert(symbols_pos_.size() < MAX_ACTIVE_SYMBOLS);

        int pos = -1;
        for (unsigned int i = 0; i < MAX_ACTIVE_SYMBOLS; i++) {
            if (symbols_pos_rev_.find(i) == symbols_pos_rev_.end()) {
                pos = static_cast<int>(i);  // insert here...
                break;
            }
        }
        assert(pos >= 0);

        std::cout << "initSymbol(" << symbol.symbol << ") pos=" << pos << std::endl;

        symbols_pos_rev_.emplace(static_cast<size_t>(pos), symbol);
        symbols_pos_[symbol] = static_cast<size_t>(pos);
    }
    else {
        // symbol already exists, just zero it? throw?
        // TOOD throw?
        //return 1;  // return 1?
    }
	assert(symbols_pos_.size() == symbols_pos_rev_.size());

    const auto& pos = symbols_pos_[symbol];

    // zero
	zero_pos(pos);

	// load initial data
    const timestamp_us_t req_end_ts = static_cast<timestamp_us_t>(ts.count() / 60) * 60 * SEC_TO_US;
    //const timestamp_us_t req_end_ts = norm_1min_s_to_us(ts.count());
    timestamp_us_t req_start_ts_1min = 0;

    for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS)
    {
		switch (std::get<0>(tpl))
		{
		case 1:
			{
				req_start_ts_1min = req_end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_1min_)[pos];
				xtensor_raw_interval& dest = (*symbols_1min_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), req_start_ts_1min, req_end_ts, true, std::get<2>(tpl));
				break;
			}
		case 5:
			{
				const timestamp_us_t end_ts = static_cast<timestamp_us_t>(req_end_ts / (300 * SEC_TO_US)) * 300 * SEC_TO_US;
				const timestamp_us_t start_ts = end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_5min_)[pos];
				xtensor_raw_interval& dest = (*symbols_5min_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
				break;
			}
		case 15:
			{
				const timestamp_us_t end_ts = static_cast<timestamp_us_t>(req_end_ts / (900 * SEC_TO_US)) * 900 * SEC_TO_US;
				const timestamp_us_t start_ts = end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_15min_)[pos];
				xtensor_raw_interval& dest = (*symbols_15min_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
				break;
			}
		case 60:
			{
				const timestamp_us_t end_ts = static_cast<timestamp_us_t>(req_end_ts / (3600 * SEC_TO_US)) * 3600 * SEC_TO_US;
				const timestamp_us_t start_ts = end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_1hr_)[pos];
				xtensor_raw_interval& dest = (*symbols_1hr_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
				break;
			}
		case 240:
			{
				const timestamp_us_t end_ts = static_cast<timestamp_us_t>(req_end_ts / (3600 * SEC_TO_US)) * 3600 * SEC_TO_US;
				const timestamp_us_t start_ts = end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_4hr_)[pos];
				xtensor_raw_interval& dest = (*symbols_4hr_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
				break;
			}
		case 1440:
			{
				const timestamp_us_t end_ts = static_cast<timestamp_us_t>(req_end_ts / (3600 * SEC_TO_US)) * 3600 * SEC_TO_US;
				const timestamp_us_t start_ts = end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_1d_)[pos];
				xtensor_raw_interval& dest = (*symbols_1d_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
				break;
			}
		case 10080:
			{
				const timestamp_us_t end_ts = static_cast<timestamp_us_t>(req_end_ts / (3600 * SEC_TO_US)) * 3600 * SEC_TO_US;
				const timestamp_us_t start_ts = end_ts - std::get<1>(tpl) * MIN_TO_US;
				xtensor_ts_interval& dest_ts = (*symbols_ts_1w_)[pos];
				xtensor_raw_interval& dest = (*symbols_1w_)[pos];
				PolygonIoAdapter::getAggregateHistory(dest_ts, dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
				break;
			}
		default:
			break;
		}
    }

	// now download quotes/NBBO up to this timestamp, and merge the bid/ask prices... (and load latest NUM_QUOTES quotes)
    {
		// TODO yar, do not create temporary to perform this o.0
		xtensor_ts_255 data_ts = xt::stack(xt::xtuple(
			xt::dynamic_view((*symbols_ts_1min_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_ts_5min_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_ts_15min_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_ts_1hr_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_ts_4hr_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_ts_1d_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_ts_1w_)[pos], { xt::all(), xt::all() })
		));

		xtensor_raw_255 data = xt::stack(xt::xtuple(
			xt::dynamic_view((*symbols_1min_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_5min_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_15min_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_1hr_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_4hr_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_1d_)[pos], { xt::all(), xt::all() }),
			xt::dynamic_view((*symbols_1w_)[pos], { xt::all(), xt::all() })
		));
		if (PolygonIoAdapter::mergeQuotesAggregates(data, symbol.symbol, data_ts, req_end_ts))
		{
			// TODO improve this o.0
			// copy results back...

			/* // Note timestep were constant, so no need to copy these back...
			(*symbols_ts_1min_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_1min, xt::all(), xt::all() });
			(*symbols_ts_5min_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_5min, xt::all(), xt::all() });
			(*symbols_ts_15min_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_15min, xt::all(), xt::all() });
			(*symbols_ts_1hr_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_1hr, xt::all(), xt::all() });
			(*symbols_ts_4hr_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_4hr, xt::all(), xt::all() });
			(*symbols_ts_1d_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_1day, xt::all(), xt::all() });
			(*symbols_ts_1w_)[pos] = xt::dynamic_view(data_ts, { Timeframes::_1wk, xt::all(), xt::all() });*/

			(*symbols_1min_)[pos] = xt::dynamic_view(data, { Timeframes::_1min, xt::all(), xt::all() });
			(*symbols_5min_)[pos] = xt::dynamic_view(data, { Timeframes::_5min, xt::all(), xt::all() });
			(*symbols_15min_)[pos] = xt::dynamic_view(data, { Timeframes::_15min, xt::all(), xt::all() });
			(*symbols_1hr_)[pos] = xt::dynamic_view(data, { Timeframes::_1hr, xt::all(), xt::all() });
			(*symbols_4hr_)[pos] = xt::dynamic_view(data, { Timeframes::_4hr, xt::all(), xt::all() });
			(*symbols_1d_)[pos] = xt::dynamic_view(data, { Timeframes::_1day, xt::all(), xt::all() });
			(*symbols_1w_)[pos] = xt::dynamic_view(data, { Timeframes::_1wk, xt::all(), xt::all() });
		}
    }

	// TODO run preprocess now? (and later only process last ts)
    {
		// must transpose raw data to (features, timesteps) for preprocess...
		auto& symbol_outputs = (*symbols_outputs_)[pos];
		run_preprocess((*proc_symbols_1min_)[pos], symbol_outputs, symbol, 1, (*symbols_ts_1min_)[pos], (*symbols_1min_)[pos]);
		run_preprocess((*proc_symbols_5min_)[pos], symbol_outputs, symbol, 5, (*symbols_ts_5min_)[pos], (*symbols_5min_)[pos]);
		run_preprocess((*proc_symbols_15min_)[pos], symbol_outputs, symbol, 15, (*symbols_ts_15min_)[pos], (*symbols_15min_)[pos]);
		run_preprocess((*proc_symbols_1hr_)[pos], symbol_outputs, symbol, 60, (*symbols_ts_1hr_)[pos], (*symbols_1hr_)[pos]);
		run_preprocess((*proc_symbols_4hr_)[pos], symbol_outputs, symbol, 240, (*symbols_ts_4hr_)[pos], (*symbols_4hr_)[pos]);
		run_preprocess((*proc_symbols_1d_)[pos], symbol_outputs, symbol, 1440, (*symbols_ts_1d_)[pos], (*symbols_1d_)[pos]);
		run_preprocess((*proc_symbols_1w_)[pos], symbol_outputs, symbol, 10080, (*symbols_ts_1w_)[pos], (*symbols_1w_)[pos]);
    }

    {
		// now download the most recent quotes, and most recent trades
		std::queue<TradeData> trades, trades2;
		std::queue<QuoteData> quotes, quotes2;
		PolygonIoAdapter::getQuoteHistoryBefore(quotes, symbol.symbol, req_end_ts, NUM_QUOTES);
		PolygonIoAdapter::getTradeHistoryBefore(trades, symbol.symbol, req_end_ts, NUM_TRADES);
		// TODO improve this (reverse iterator?) (reverse using tmp queues)
		while (!quotes.empty())
		{
			quotes2.push(quotes.front());
			quotes.pop();
		}
		while (!trades.empty())
		{
			trades2.push(trades.front());
			trades.pop();
		}
		auto& latest_quotes = (*latest_quotes_)[pos];
		auto& latest_trades = (*latest_trades_)[pos];
		while (!quotes2.empty())
		{
			latest_quotes.push(quotes2.front());
			quotes2.pop();
		}
		while (!trades2.empty())
		{
			latest_trades.push(trades2.front());
			trades2.pop();
		}
    }
}

void DataController::destroySymbol(const Symbol& symbol)
{
    if (symbols_pos_.find(symbol) != symbols_pos_.end()) {
        const auto& pos = symbols_pos_[symbol];

		// zero
		zero_pos(pos);

		// remove symbol from symbols_pos_rev_ and symbols_pos_
        symbols_pos_rev_.erase(pos);
        symbols_pos_.erase(symbol);
    }
    else {
        // symbol does not exists
        /*
        // TODO skip this?!
        // TODO remove using iterator?
        int rem_pos = -1;
        for (auto& yar : symbols_pos_rev_) {
            if (yar.second == symbol) {
                rem_pos = static_cast<int>(yar.first);
                break;
            }
        }
        if (rem_pos >= 0) {
            std::cout << "destroySymbol() removing invalid symbol " << symbol.symbol << std::endl;
            symbols_pos_rev_.erase(rem_pos);
        }
        */
        // TOOD throw?
    }
	assert(symbols_pos_.size() == symbols_pos_rev_.size());

	
}
