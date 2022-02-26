#include "data_controller.h"

#include <array>
#include <iostream>
#include <tuple>
#include <algorithm>

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


/*void zero_symbols_data(std::array<xtensor_raw, MAX_ACTIVE_SYMBOLS>& data)
{
	for (auto& symbolData : data)
		symbolData = xt::zeros<double>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
}

void zero_processed_data(std::array<xt::xtensor_fixed<double, shape_processed_t>, MAX_ACTIVE_SYMBOLS>& data)
{
	for (auto& symbolData : data)
		symbolData = xt::zeros<double>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
}*/


// Note: data_outputs is only populated if interval==TIMEFRAME
inline void run_preprocess(xtensor_processed_interval& data_processed, xtensor_outputs_interval& data_outputs, const Symbol& symbol, const int interval, const xtensor_raw_interval& data_raw)
{
	//std::cout << interval << "min raw:" << std::endl << data_raw << std::endl;
	//std::cout << interval << "min raw shape:" << std::endl << data_raw.shape() << std::endl;

	// must transpose raw data to (features, timesteps) for preprocess...
	const xt::xarray<double> data_raw_transposed = xt::transpose(data_raw, { 1, 0 });

	// process interval data (returned in ASC order, input is in DESC order):
	// TODO TIMEFRAME, training
	const bool training = TIMEFRAME == interval;

	auto a_step1 = process_step1_single(symbol.symbol.c_str(), data_raw_transposed, training, TIMEFRAME, interval, false);
	auto processed = process_step2_single(symbol.symbol.c_str(), a_step1, training, TIMEFRAME, interval, false);

	// TODO skip creating outputs (elsewhere) if !training?
	xt::xarray<double> o_outputs;  // TODO include timestamp and/or close in outputs?
	if (training)
		o_outputs = xt::zeros<double>({ static_cast<int>(ColPos::_OUTPUT_NUM_COLS), static_cast<int>(a_step1.shape().at(1)) });
	process_step3_single(processed, o_outputs, symbol.symbol.c_str(), a_step1, training, TIMEFRAME, interval, false);
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
		xt::xarray<double> processed_transposed_heap = xt::transpose(data_processed, { 1, 0 });
		xt::xarray<double> tss2 = xt::row(processed_transposed_heap, ColPos::In::timestamp);
		std::cout << interval << "min processed_transposed tss:" << std::endl << tss2 << std::endl;
		std::cout << interval << "min processed_transposed tss shape: " << tss2.shape() << std::endl;
		*/

		if (training) {
			std::cout << interval << "min outputs: " << data_outputs << std::endl;
			std::cout << interval << "min outputs shape: " << data_outputs.shape() << std::endl;
		}
	}
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
			{ (*symbols_1min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[0](ROW_POS, ColPos::In::open), (*symbols_1min_)[0](ROW_POS, ColPos::In::high), (*symbols_1min_)[0](ROW_POS, ColPos::In::low), (*symbols_1min_)[0](ROW_POS, ColPos::In::close), (*symbols_1min_)[0](ROW_POS, ColPos::In::volume) },
			(*symbols_1min_)[0](ROW_POS, ColPos::In::bid), (*symbols_1min_)[0](ROW_POS, ColPos::In::bid_high), (*symbols_1min_)[0](ROW_POS, ColPos::In::bid_low),
			(*symbols_1min_)[0](ROW_POS, ColPos::In::ask), (*symbols_1min_)[0](ROW_POS, ColPos::In::ask_high), (*symbols_1min_)[0](ROW_POS, ColPos::In::ask_low),
			(*symbols_1min_)[0](ROW_POS, ColPos::In::bid_size), (*symbols_1min_)[0](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{
			{ (*symbols_1min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[1](ROW_POS, ColPos::In::open), (*symbols_1min_)[1](ROW_POS, ColPos::In::high), (*symbols_1min_)[1](ROW_POS, ColPos::In::low), (*symbols_1min_)[1](ROW_POS, ColPos::In::close), (*symbols_1min_)[1](ROW_POS, ColPos::In::volume) },
			(*symbols_1min_)[1](ROW_POS, ColPos::In::bid), (*symbols_1min_)[1](ROW_POS, ColPos::In::bid_high), (*symbols_1min_)[1](ROW_POS, ColPos::In::bid_low),
			(*symbols_1min_)[1](ROW_POS, ColPos::In::ask), (*symbols_1min_)[1](ROW_POS, ColPos::In::ask_high), (*symbols_1min_)[1](ROW_POS, ColPos::In::ask_low),
			(*symbols_1min_)[1](ROW_POS, ColPos::In::bid_size), (*symbols_1min_)[1](ROW_POS, ColPos::In::ask_size) },
		BarFullRef{
			{ (*symbols_1min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1min_)[2](ROW_POS, ColPos::In::open), (*symbols_1min_)[2](ROW_POS, ColPos::In::high), (*symbols_1min_)[2](ROW_POS, ColPos::In::low), (*symbols_1min_)[2](ROW_POS, ColPos::In::close), (*symbols_1min_)[2](ROW_POS, ColPos::In::volume) },
			(*symbols_1min_)[2](ROW_POS, ColPos::In::bid), (*symbols_1min_)[2](ROW_POS, ColPos::In::bid_high), (*symbols_1min_)[2](ROW_POS, ColPos::In::bid_low),
			(*symbols_1min_)[2](ROW_POS, ColPos::In::ask), (*symbols_1min_)[2](ROW_POS, ColPos::In::ask_high), (*symbols_1min_)[2](ROW_POS, ColPos::In::ask_low),
			(*symbols_1min_)[2](ROW_POS, ColPos::In::bid_size), (*symbols_1min_)[2](ROW_POS, ColPos::In::ask_size) } }),
	latest_5min_({
		BarRef{ (*symbols_5min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[0](ROW_POS, ColPos::In::open), (*symbols_5min_)[0](ROW_POS, ColPos::In::high), (*symbols_5min_)[0](ROW_POS, ColPos::In::low), (*symbols_5min_)[0](ROW_POS, ColPos::In::close), (*symbols_5min_)[0](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_5min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[1](ROW_POS, ColPos::In::open), (*symbols_5min_)[1](ROW_POS, ColPos::In::high), (*symbols_5min_)[1](ROW_POS, ColPos::In::low), (*symbols_5min_)[1](ROW_POS, ColPos::In::close), (*symbols_5min_)[1](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_5min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_5min_)[2](ROW_POS, ColPos::In::open), (*symbols_5min_)[2](ROW_POS, ColPos::In::high), (*symbols_5min_)[2](ROW_POS, ColPos::In::low), (*symbols_5min_)[2](ROW_POS, ColPos::In::close), (*symbols_5min_)[2](ROW_POS, ColPos::In::volume) } }),
	latest_15min_({
		BarRef{ (*symbols_15min_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[0](ROW_POS, ColPos::In::open), (*symbols_15min_)[0](ROW_POS, ColPos::In::high), (*symbols_15min_)[0](ROW_POS, ColPos::In::low), (*symbols_15min_)[0](ROW_POS, ColPos::In::close), (*symbols_15min_)[0](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_15min_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[1](ROW_POS, ColPos::In::open), (*symbols_15min_)[1](ROW_POS, ColPos::In::high), (*symbols_15min_)[1](ROW_POS, ColPos::In::low), (*symbols_15min_)[1](ROW_POS, ColPos::In::close), (*symbols_15min_)[1](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_15min_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_15min_)[2](ROW_POS, ColPos::In::open), (*symbols_15min_)[2](ROW_POS, ColPos::In::high), (*symbols_15min_)[2](ROW_POS, ColPos::In::low), (*symbols_15min_)[2](ROW_POS, ColPos::In::close), (*symbols_15min_)[2](ROW_POS, ColPos::In::volume) } }),
	latest_1hr_({
		BarRef{ (*symbols_1hr_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[0](ROW_POS, ColPos::In::open), (*symbols_1hr_)[0](ROW_POS, ColPos::In::high), (*symbols_1hr_)[0](ROW_POS, ColPos::In::low), (*symbols_1hr_)[0](ROW_POS, ColPos::In::close), (*symbols_1hr_)[0](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_1hr_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[1](ROW_POS, ColPos::In::open), (*symbols_1hr_)[1](ROW_POS, ColPos::In::high), (*symbols_1hr_)[1](ROW_POS, ColPos::In::low), (*symbols_1hr_)[1](ROW_POS, ColPos::In::close), (*symbols_1hr_)[1](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_1hr_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1hr_)[2](ROW_POS, ColPos::In::open), (*symbols_1hr_)[2](ROW_POS, ColPos::In::high), (*symbols_1hr_)[2](ROW_POS, ColPos::In::low), (*symbols_1hr_)[2](ROW_POS, ColPos::In::close), (*symbols_1hr_)[2](ROW_POS, ColPos::In::volume) } }),
	latest_4hr_({
		BarRef{ (*symbols_4hr_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[0](ROW_POS, ColPos::In::open), (*symbols_4hr_)[0](ROW_POS, ColPos::In::high), (*symbols_4hr_)[0](ROW_POS, ColPos::In::low), (*symbols_4hr_)[0](ROW_POS, ColPos::In::close), (*symbols_4hr_)[0](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_4hr_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[1](ROW_POS, ColPos::In::open), (*symbols_4hr_)[1](ROW_POS, ColPos::In::high), (*symbols_4hr_)[1](ROW_POS, ColPos::In::low), (*symbols_4hr_)[1](ROW_POS, ColPos::In::close), (*symbols_4hr_)[1](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_4hr_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_4hr_)[2](ROW_POS, ColPos::In::open), (*symbols_4hr_)[2](ROW_POS, ColPos::In::high), (*symbols_4hr_)[2](ROW_POS, ColPos::In::low), (*symbols_4hr_)[2](ROW_POS, ColPos::In::close), (*symbols_4hr_)[2](ROW_POS, ColPos::In::volume) } }),
	latest_1d_({
		BarRef{ (*symbols_1d_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[0](ROW_POS, ColPos::In::open), (*symbols_1d_)[0](ROW_POS, ColPos::In::high), (*symbols_1d_)[0](ROW_POS, ColPos::In::low), (*symbols_1d_)[0](ROW_POS, ColPos::In::close), (*symbols_1d_)[0](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_1d_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[1](ROW_POS, ColPos::In::open), (*symbols_1d_)[1](ROW_POS, ColPos::In::high), (*symbols_1d_)[1](ROW_POS, ColPos::In::low), (*symbols_1d_)[1](ROW_POS, ColPos::In::close), (*symbols_1d_)[1](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_1d_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1d_)[2](ROW_POS, ColPos::In::open), (*symbols_1d_)[2](ROW_POS, ColPos::In::high), (*symbols_1d_)[2](ROW_POS, ColPos::In::low), (*symbols_1d_)[2](ROW_POS, ColPos::In::close), (*symbols_1d_)[2](ROW_POS, ColPos::In::volume) } }),
	latest_1w_({
		BarRef{ (*symbols_1w_)[0](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[0](ROW_POS, ColPos::In::open), (*symbols_1w_)[0](ROW_POS, ColPos::In::high), (*symbols_1w_)[0](ROW_POS, ColPos::In::low), (*symbols_1w_)[0](ROW_POS, ColPos::In::close), (*symbols_1w_)[0](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_1w_)[1](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[1](ROW_POS, ColPos::In::open), (*symbols_1w_)[1](ROW_POS, ColPos::In::high), (*symbols_1w_)[1](ROW_POS, ColPos::In::low), (*symbols_1w_)[1](ROW_POS, ColPos::In::close), (*symbols_1w_)[1](ROW_POS, ColPos::In::volume) },
		BarRef{ (*symbols_1w_)[2](ROW_POS, ColPos::In::timestamp), (*symbols_1w_)[2](ROW_POS, ColPos::In::open), (*symbols_1w_)[2](ROW_POS, ColPos::In::high), (*symbols_1w_)[2](ROW_POS, ColPos::In::low), (*symbols_1w_)[2](ROW_POS, ColPos::In::close), (*symbols_1w_)[2](ROW_POS, ColPos::In::volume) } }),

	// TODO some form of snapshots_ initialization that dynamically handles different MAX_ACTIVE_SYMBOLS sizes?
	snapshots_({
		Snapshot{
				{ 0.0, 0.0, 0.0, 0, 0 },
				(*symbols_1min_)[0](ROW_POS, ColPos::In::close),
				latest_1min_[0],
				latest_5min_[0],
				latest_15min_[0],
				latest_1hr_[0] },
		Snapshot{
				{ 0.0, 0.0, 0.0, 0, 0 },
				(*symbols_1min_)[1](ROW_POS, ColPos::In::close),
				latest_1min_[1],
				latest_5min_[1],
				latest_15min_[1],
				latest_1hr_[1] },
		Snapshot{
				{ 0.0, 0.0, 0.0, 0, 0 },
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
		}
		if (triggers.flush5min)  // 5min
		{
			// shift down
			auto& data_5min = (*symbols_5min_)[pos];
			const auto data_copy = xt::roll(data_5min, shift_pos, 0);
			data_5min = data_copy;
			
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

		// preprocess all timeframes now...
		// process data (returned in ASC order, opposite of input):
		run_preprocess((*proc_symbols_1min_)[pos], symbol_outputs, symbol, 1, (*symbols_1min_)[pos]);
		run_preprocess((*proc_symbols_5min_)[pos], symbol_outputs, symbol, 5, (*symbols_5min_)[pos]);
		run_preprocess((*proc_symbols_15min_)[pos], symbol_outputs, symbol, 15, (*symbols_15min_)[pos]);
		run_preprocess((*proc_symbols_1hr_)[pos], symbol_outputs, symbol, 60, (*symbols_1hr_)[pos]);
		run_preprocess((*proc_symbols_4hr_)[pos], symbol_outputs, symbol, 240, (*symbols_4hr_)[pos]);

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
				on_update_outputs_(symbol, snapshots_[pos], data, data_processed, (*latest_quotes_)[pos], (*latest_trades_)[pos], symbol_outputs);
			else
				on_update_(symbol, snapshots_[pos], data, data_processed, (*latest_quotes_)[pos], (*latest_trades_)[pos]);
		}

		// TODO remove save for verification
		if (triggers.flush15min)
		{
			xt::dump_npy("pyfiles/_out.AAPL.1min.npy", (*symbols_1min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.5min.npy", (*symbols_5min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.15min.npy", (*symbols_15min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.1hr.npy", (*symbols_1hr_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.4hr.npy", (*symbols_4hr_)[pos]);
		}
	}
}


void DataController::process_trade(const size_t& pos, const json& trade)
{
	// Note: assuming trade["T"] has been handled/verified
	//

	const double ts = static_cast<double>(trade["t"].get<uint64_t>()) / 1.0e9;
	const auto& price = trade["p"].get<double>();
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
	
	const double ts = static_cast<double>(trade["t"].get<uint64_t>()) / 1.0e3;
	const auto& price = trade["p"].get<double>();
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
	const auto ts = static_cast<double>(trade.trade.timestamp / 1000);

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


inline void update_high_low(BarRef& bar, const double& price)
{
	if (price > bar.high)
		bar.high = price;
	if (bar.low == 0 || price < bar.low)
		bar.low = price;
}

inline void update_open_close(BarRef& bar, const double& price)
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

inline void populate_full_bar(BarFullRef& bar, const int interval_seconds, const timestamp_t& next_ts, const double& ts, const double& price, const uint32_t vol, const double& prev_bid, const double& prev_ask, const double& prev_bid_size, const double& prev_ask_size)
{
	// reset this bar
	bar.timestamp = static_cast<double>(static_cast<timestamp_t>(ts / interval_seconds) * interval_seconds);  // TODO verify
	//bar.timestamp = next_ts;
	if (static_cast<timestamp_t>(bar.timestamp) != next_ts && next_ts > interval_seconds) {
		std::cout << "populate_full_bar() Skip timestamp " << next_ts << " (to " << static_cast<timestamp_t>(bar.timestamp) << ")" << std::endl;
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
}

inline void populate_bar(BarRef& bar, const int interval_seconds, const timestamp_t& next_ts, const double& ts, const double& price, const uint32_t vol)
{
	// reset this bar
	bar.timestamp = static_cast<double>(static_cast<uint64_t>(ts / interval_seconds) * interval_seconds);  // TODO verify
	//bar.timestamp = next_ts;
	if (static_cast<timestamp_t>(bar.timestamp) != next_ts && next_ts > interval_seconds) {
		std::cout << "populate_bar() Skip timestamp " << next_ts << " (to " << static_cast<timestamp_t>(bar.timestamp) << ")" << std::endl;
	}
	bar.volume = vol;
	bar.low = bar.high = bar.open = bar.close = price;
}

void DataController::process_trade_finish(const size_t& pos, const json& conds, const double& ts, const double& price, const uint32_t vol)
{
	// flush into 1min bar
	const int interval_seconds = 60;
	timestamp_t& ts_step = cur_timesteps_[pos];

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
	BarRef& bar_5min = latest_5min_[pos];
	BarRef& bar_15min = latest_15min_[pos];
	BarRef& bar_1hr = latest_1hr_[pos];
	BarRef& bar_4hr = latest_4hr_[pos];
	BarRef& bar_1d = latest_1d_[pos];
	BarRef& bar_1w = latest_1w_[pos];
	
	const timestamp_t next_ts = ts_step + interval_seconds;
	if (ts >= static_cast<double>(next_ts))
	{
		const double prev_bid = bar_1min.bid;
		const double prev_ask = bar_1min.ask;
		const double prev_bid_size = bar_1min.bid_size;
		const double prev_ask_size = bar_1min.ask_size;

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
			populate_full_bar(bar_1min, interval_seconds, next_ts, ts, price, vol, prev_bid, prev_ask, prev_bid_size, prev_ask_size);
		}
		if (triggers.flush5min)
		{
			populate_bar(bar_5min, interval_seconds * 5, next_ts, ts, price, vol);
		}
		if (triggers.flush15min)
		{
			populate_bar(bar_15min, interval_seconds * 15, next_ts, ts, price, vol);
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

		ts_step = static_cast<timestamp_t>(bar_1min.timestamp);  // next_ts;
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

	const double ts = static_cast<double>(quote["t"].get<uint64_t>()) / 1.0e9;
	const auto& askPrice = quote["P"].get<double>();
	const auto& askSize = quote["S"].get<uint32_t>();
	const auto& bidPrice = quote["p"].get<double>();
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

	const double ts = static_cast<double>(quote["t"].get<uint64_t>()) / 1.0e3;
	const auto& askPrice = quote["ap"].get<double>();
	const auto& askSize = quote["as"].get<uint32_t>();
	const auto& bidPrice = quote["bp"].get<double>();
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
	const auto ts = static_cast<double>(quote.quote.timestamp / 1000);

	process_quote_finish(pos, ts, quote.quote.ask, quote.quote.ask_size, quote.quote.bid, quote.quote.bid_size);
}


inline void update_bid_ask(BarFullRef& bar, const double& bid, const double& ask, const double bid_size, const double ask_size)
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

inline void populate_next_full_bar(BarFullRef& bar, const int interval_seconds, const timestamp_t& next_ts, const double& ts, const double& price, const double& bid, const double& ask, const double bid_size, const double ask_size)
{
	// reset this bar
	bar.timestamp = static_cast<Bar::time_type>(static_cast<uint64_t>(ts / interval_seconds) * interval_seconds);  // TODO verify
	//bar.timestamp = next_ts;
	if (static_cast<timestamp_t>(bar.timestamp) != next_ts && next_ts > interval_seconds) {
		std::cout << "populate_next_full_bar() Skip timestamp " << next_ts << " (to " << static_cast<timestamp_t>(bar.timestamp) << ")" << std::endl;
	}
	bar.volume = 0;  // so far, 0 volume in this bar
	bar.low = bar.high = bar.open = bar.close = price;  // TODO copy last close price ?

	// set bid/ask and reset the high/lows
	bar.ask = bar.ask_high = bar.ask_low = ask;
	bar.bid = bar.bid_high = bar.bid_low = bid;
	bar.ask_size = ask_size;
	bar.bid_size = bid_size;
}

inline void populate_next_bar(BarRef& bar, const int interval_seconds, const timestamp_t& next_ts, const double& ts, const double& price)
{
	// reset this bar
	bar.timestamp = static_cast<Bar::time_type>(static_cast<uint64_t>(ts / interval_seconds) * interval_seconds);  // TODO verify
	//bar.timestamp = next_ts;
	if (static_cast<timestamp_t>(bar.timestamp) != next_ts && next_ts > interval_seconds) {
		std::cout << "populate_next_bar() Skip timestamp " << next_ts << " (to " << static_cast<timestamp_t>(bar.timestamp) << ")" << std::endl;
	}
	bar.volume = 0;  // so far, 0 volume in this bar
	bar.low = bar.high = bar.open = bar.close = price;  // TODO copy last close price ?
}

void DataController::process_quote_finish(const size_t& pos, const double& ts, const double& askPrice, const uint32_t& askSize, const double& bidPrice, const uint32_t& bidSize)
{
	// flush into 1min bar
	const int interval_seconds = 60;
	timestamp_t& ts_step = cur_timesteps_[pos];

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
	BarRef& bar_5min = latest_5min_[pos];
	BarRef& bar_15min = latest_15min_[pos];
	BarRef& bar_1hr = latest_1hr_[pos];
	BarRef& bar_4hr = latest_4hr_[pos];
	//BarRef& bar_1d = latest_1d_[pos];
	//BarRef& bar_1w = latest_1w_[pos];

	const timestamp_t next_ts = ts_step + interval_seconds;
	if (ts >= static_cast<double>(next_ts))
	{
		// copy last close price into new bars
		const double price = bar_1min.close;

		const ShiftTriggers triggers(next_ts);

		// shift bars
		// Note: this introduces delay to the flush routine (by waiting for next timestamp before processing previous)
		//   This should be changed after moving to a faster network, then preempt end-of-interval
		if (next_ts > interval_seconds) {  // only shift if this is not the first timestep
			// call update before shifting
			do_update(triggers, pos);
			
			shift(triggers, pos);
		}

		if (triggers.flush1min)
		{
			populate_next_full_bar(bar_1min, interval_seconds, next_ts, ts, price, bidPrice, askPrice, bidSize, askSize);
		}
		if (triggers.flush5min)
		{
			populate_next_bar(bar_5min, interval_seconds * 5, next_ts, ts, price);
		}
		if (triggers.flush15min)
		{
			populate_next_bar(bar_15min, interval_seconds * 15, next_ts, ts, price);
		}
		if (triggers.flush1hr)
		{
			populate_next_bar(bar_1hr, interval_seconds * 60, next_ts, ts, price);
		}
		if (triggers.flush4hr)
		{
			populate_next_bar(bar_4hr, interval_seconds * 240, next_ts, ts, price);
		}
		// Note: never flush 1d/1w (TODO flush 1d if after hours starts? or 1w if after hours starts on friday?)
		/*if (triggers.flush1d)
			populate_next_bar(bar_1d, interval_seconds, next_ts, ts, price);
		if (triggers.flush1w)
			populate_next_bar(bar_1w, interval_seconds, next_ts, ts, price);*/
		
		ts_step = static_cast<timestamp_t>(bar_1min.timestamp);  // next_ts;
	}
	else
	{
		// append to existing bar!
		update_bid_ask(bar_1min, bidPrice, askPrice, bidSize, askSize);
	}
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
	timestamp_t req_start_ts = static_cast<timestamp_t>(start_ts.count());
	timestamp_t max_end_ts = req_start_ts + (static_cast<timestamp_t>(60) * num_minutes.count());
	timestamp_t end_ts = 0;

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
					end_ts = tmp_quotes.back().quote.timestamp / 1000;
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
					if (tmp_trades.front().trade.timestamp / 1000.0 > max_end_ts) {
						tmp_trades.pop();
						continue;
					}

					// first push this trade into ctrl queue, that-way it's available for the callbacks
					(*latest_trades_)[pos].emplace(tmp_trades.front());

					process_trade_data(pos, tmp_trades.front());
					tmp_trades.pop();
				}

				if (quote.quote.timestamp / 1000.0 > max_end_ts) {
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
				if (tmp_trades.front().trade.timestamp / 1000.0 > max_end_ts) {
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
	const timestamp_t req_end_ts = (static_cast<timestamp_t>(ts.count()) / 60) * 60;
	timestamp_t req_start_ts_1min = 0;

    for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS)
    {
	    switch (std::get<0>(tpl))
	    {
	    case 1:
		    {
				req_start_ts_1min = req_end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_1min_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), req_start_ts_1min, req_end_ts, true, std::get<2>(tpl));
			    break;
		    }
	    case 5:
		    {
			    const timestamp_t end_ts = (req_end_ts / 300) * 300;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_5min_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true,std::get<2>(tpl));
			    break;
		    }
	    case 15:
		    {
			    const timestamp_t end_ts = (req_end_ts / 900) * 900;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_15min_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    break;
		    }
	    case 60:
		    {
			    const timestamp_t end_ts = (req_end_ts / 3600) * 3600;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_1hr_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    break;
		    }
	    case 240:
		    {
			    const timestamp_t end_ts = (req_end_ts / 3600) * 3600;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_4hr_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    break;
		    }
	    case 1440:
		    {
			    const timestamp_t end_ts = (req_end_ts / 3600) * 3600;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_1d_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    break;
		    }
	    case 10080:
		    {
			    const timestamp_t end_ts = (req_end_ts / 3600) * 3600;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_1w_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    break;
		    }
	    default:
		    break;
	    }
    }

	// now download quotes/NBBO up to this timestamp, and merge the bid/ask prices... (and load latest NUM_QUOTES quotes)
    {
		// TODO yar, do not create temporary to perform this o.0
		xtensor_raw_255 data = xt::stack(xt::xtuple(
			xt::xarray<double>((*symbols_1min_)[pos]),
			xt::xarray<double>((*symbols_5min_)[pos]),
			xt::xarray<double>((*symbols_15min_)[pos]),
			xt::xarray<double>((*symbols_1hr_)[pos]),
			xt::xarray<double>((*symbols_4hr_)[pos]),
			xt::xarray<double>((*symbols_1d_)[pos]),
			xt::xarray<double>((*symbols_1w_)[pos])
		));
		if (PolygonIoAdapter::mergeQuotesAggregates(data, symbol.symbol, req_end_ts))
		{
			// TODO improve this o.0
			//std::cout << "1min row shape: " << xt::dynamic_view(data, { Timeframes::_1min, xt::all(), xt::all() }).shape() << std::endl;
			// copy results back...
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
		run_preprocess((*proc_symbols_1min_)[pos], symbol_outputs, symbol, 1, (*symbols_1min_)[pos]);
		run_preprocess((*proc_symbols_5min_)[pos], symbol_outputs, symbol, 5, (*symbols_5min_)[pos]);
		run_preprocess((*proc_symbols_15min_)[pos], symbol_outputs, symbol, 15, (*symbols_15min_)[pos]);
		run_preprocess((*proc_symbols_1hr_)[pos], symbol_outputs, symbol, 60, (*symbols_1hr_)[pos]);
		run_preprocess((*proc_symbols_4hr_)[pos], symbol_outputs, symbol, 240, (*symbols_4hr_)[pos]);
		run_preprocess((*proc_symbols_1d_)[pos], symbol_outputs, symbol, 1440, (*symbols_1d_)[pos]);
		run_preprocess((*proc_symbols_1w_)[pos], symbol_outputs, symbol, 10080, (*symbols_1w_)[pos]);
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
		while (!quotes2.empty())
		{
			(*latest_quotes_)[pos].push(quotes2.front());
			quotes2.pop();
		}
		while (!trades2.empty())
		{
			(*latest_trades_)[pos].push(trades2.front());
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
