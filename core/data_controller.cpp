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


using namespace agpred;



// TODO move this (stuff) into class?!  // TODO don't use a class?
// DataController should only be constructed once...
static bool dc_initialized = false;



/*void zero_symbols_data(std::array<xtensor_raw, MAX_ACTIVE_SYMBOLS>& data)
{
	for (auto& symbolData : data)
		symbolData = xt::zeros<double>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
}

void zero_processed_data(std::array<xt::xtensor_fixed<double, shape_processed_255_t>, MAX_ACTIVE_SYMBOLS>& data)
{
	for (auto& symbolData : data)
		symbolData = xt::zeros<double>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
}*/


DataController::DataController(const fn_snapshot on_snapshot, const fn_update on_update) :
	on_snapshot_(on_snapshot),
	on_update_(on_update),
	cur_timesteps_(),
	symbols_1min_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_5min_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_15min_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_1hr_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_4hr_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_1d_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	symbols_1w_(new std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>()),
	// TODO some form of snapshots_ initialization that dynamically handles different MAX_ACTIVE_SYMBOLS sizes?
	snapshots_({
		Snapshot{
				bars_1min_[0][0].nbbo(),
				bars_1min_[0][0].close,
				bars_1min_[0][0],
				bars_5min_[0][0],
				bars_15min_[0][0],
				bars_1hr_[0][0]
		},
		Snapshot{
				bars_1min_[1][0].nbbo(),
				bars_1min_[1][0].close,
				bars_1min_[1][0],
				bars_5min_[1][0],
				bars_15min_[1][0],
				bars_1hr_[1][0]
		},
		Snapshot{
				bars_1min_[2][0].nbbo(),
				bars_1min_[2][0].close,
				bars_1min_[2][0],
				bars_5min_[2][0],
				bars_15min_[2][0],
				bars_1hr_[2][0]
		} })
{
	assert(dc_initialized == false);
	dc_initialized = true;
	//zero_symbols_data(symbols_data_);
	//zero_processed_data(processed_data_);

	symbols_pos_.clear();
	symbols_pos_rev_.clear();

	assert(symbols_pos_.size() == symbols_pos_rev_.size());
}

DataController::DataController() :
	DataController(fn_snapshot(), fn_update())
{
}

DataController::~DataController()
{
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
}

void zero_bar(Bar& bar)
{
	bar = { 0, 0, 0, 0, 0, 0 };
}

void shift_bars_5(std::array<Bar, 5>& bars)
{
	std::swap(bars[3], bars[4]);
	std::swap(bars[2], bars[3]);
	std::swap(bars[1], bars[2]);
	std::swap(bars[0], bars[1]);
	// zero first
	zero_bar(bars[0]);
}

void shift_bars_4(std::array<Bar, 4>& bars)
{
	std::swap(bars[2], bars[3]);
	std::swap(bars[1], bars[2]);
	std::swap(bars[0], bars[1]);
	// zero first
	zero_bar(bars[0]);
}

void shift_bars_3(std::array<Bar, 3>& bars)
{
	std::swap(bars[1], bars[2]);
	std::swap(bars[0], bars[1]);
	// zero first
	zero_bar(bars[0]);
}

void copy_bar_2(Bar& dest, const Bar& b1, const Bar& b2)
{
	if (!b2.volume)
	{
		dest = b1;  // directly copy the single bar...
		return;
	}

	dest.timestamp = b1.timestamp;  // TODO set timestamp here?
	dest.close = b1.close;
	dest.ask = b1.ask;
	dest.ask_size = b1.ask_size;
	dest.bid = b1.bid;
	dest.bid_size = b1.bid_size;
	dest.open = b2.open;
	dest.volume = b1.volume + b2.volume;
	dest.high = std::max({ b1.high, b2.high });
	dest.low = std::min({ b1.low, b2.low });
	dest.ask_high = std::max({ b1.ask_high, b2.ask_high });
	dest.ask_low = std::min({ b1.ask_low, b2.ask_low });
	dest.bid_high = std::max({ b1.bid_high, b2.bid_high });
	dest.bid_low = std::min({ b1.bid_low, b2.bid_low });
}

void copy_bar_3(Bar& dest, const Bar& b1, const Bar& b2, const Bar& b3)
{
	if (!b3.volume)
		return copy_bar_2(dest, b1, b2);

	dest.timestamp = b1.timestamp;  // TODO set timestamp here?
	dest.close = b1.close;
	dest.ask = b1.ask;
	dest.ask_size = b1.ask_size;
	dest.bid = b1.bid;
	dest.bid_size = b1.bid_size;
	dest.open = b3.open;
	dest.volume = b1.volume + b2.volume + b3.volume;
	dest.high = std::max({ b1.high, b2.high, b3.high });
	dest.low = std::min({ b1.low, b2.low, b3.low });
	dest.ask_high = std::max({ b1.ask_high, b2.ask_high, b3.ask_high });
	dest.ask_low = std::min({ b1.ask_low, b2.ask_low, b3.ask_low });
	dest.bid_high = std::max({ b1.bid_high, b2.bid_high, b3.bid_high });
	dest.bid_low = std::min({ b1.bid_low, b2.bid_low, b3.bid_low });
}

void copy_bar_4(Bar& dest, const Bar& b1, const Bar& b2, const Bar& b3, const Bar& b4)
{
	if (!b4.volume)
		return copy_bar_3(dest, b1, b2, b3);

	dest.timestamp = b1.timestamp;  // TODO set timestamp here?
	dest.close = b1.close;
	dest.ask = b1.ask;
	dest.ask_size = b1.ask_size;
	dest.bid = b1.bid;
	dest.bid_size = b1.bid_size;
	dest.open = b4.open;
	dest.volume = b1.volume + b2.volume + b3.volume + b4.volume;
	dest.high = std::max({ b1.high, b2.high, b3.high, b4.high });
	dest.low = std::min({ b1.low, b2.low, b3.low, b4.low });
	dest.ask_high = std::max({ b1.ask_high, b2.ask_high, b3.ask_high, b4.ask_high });
	dest.ask_low = std::min({ b1.ask_low, b2.ask_low, b3.ask_low, b4.ask_low });
	dest.bid_high = std::max({ b1.bid_high, b2.bid_high, b3.bid_high, b4.bid_high });
	dest.bid_low = std::min({ b1.bid_low, b2.bid_low, b3.bid_low, b4.bid_low });
}

void copy_bar_5(Bar& dest, const Bar& b1, const Bar& b2, const Bar& b3, const Bar& b4, const Bar& b5)
{
	if (!b5.volume)
		return copy_bar_4(dest, b1, b2, b3, b4);

	dest.timestamp = b1.timestamp;  // TODO set timestamp here?
	dest.close = b1.close;
	dest.ask = b1.ask;
	dest.ask_size = b1.ask_size;
	dest.bid = b1.bid;
	dest.bid_size = b1.bid_size;
	dest.open = b5.open;
	dest.volume = b1.volume + b2.volume + b3.volume + b4.volume + b5.volume;
	dest.high = std::max({ b1.high, b2.high, b3.high, b4.high, b5.high });
	dest.low = std::min({ b1.low, b2.low, b3.low, b4.low, b5.low });
	dest.ask_high = std::max({ b1.ask_high, b2.ask_high, b3.ask_high, b4.ask_high, b5.ask_high });
	dest.ask_low = std::min({ b1.ask_low, b2.ask_low, b3.ask_low, b4.ask_low, b5.ask_low });
	dest.bid_high = std::max({ b1.bid_high, b2.bid_high, b3.bid_high, b4.bid_high, b5.bid_high });
	dest.bid_low = std::min({ b1.bid_low, b2.bid_low, b3.bid_low, b4.bid_low, b5.bid_low });
}

inline void insert_candle(xtensor_raw_interval& data_latest, const Bar& bar)
{
	// TODO better way to insert these?
	data_latest(0, ColPos::In::timestamp) = bar.timestamp;
	data_latest(0, ColPos::In::open) = bar.open;
	data_latest(0, ColPos::In::high) = bar.high;
	data_latest(0, ColPos::In::low) = bar.low;
	data_latest(0, ColPos::In::close) = bar.close;
	data_latest(0, ColPos::In::volume) = static_cast<double>(bar.volume);  // TODO divide by 100 to match IB data?

	// TODO this order?
	data_latest(0, ColPos::In::ask_size) = bar.ask_size;
	data_latest(0, ColPos::In::bid_size) = bar.bid_size;
	data_latest(0, ColPos::In::ask) = bar.ask;
	data_latest(0, ColPos::In::ask_high) = bar.ask_high;
	data_latest(0, ColPos::In::ask_low) = bar.ask_low;
	data_latest(0, ColPos::In::bid) = bar.bid;
	data_latest(0, ColPos::In::bid_high) = bar.bid_high;
	data_latest(0, ColPos::In::bid_low) = bar.bid_low;
}


template <size_t Size>
void copy_candles(std::array<Bar, Size>& bars, const xtensor_raw_interval& data)
{

	/*if (!bars.empty()) {
		const auto num_candles = bars.size();
		for (typename std::array<Bar, Size>::size_type i = 0; i < Size; ++i)
			std::cout << bars[i] << (i == num_candles - 1 ? "" : ", ");
	}*/
	for (typename std::array<Bar, Size>::size_type i = 0; i < Size; ++i)
	{
		Bar& bar = bars[i];

		bar.timestamp = data(i, ColPos::In::timestamp);
		bar.open = data(i, ColPos::In::open);
		bar.high = data(i, ColPos::In::high);
		bar.low = data(i, ColPos::In::low);
		bar.close = data(i, ColPos::In::close);
		bar.volume = static_cast<uint64_t>(data(i, ColPos::In::volume));

		bar.bid = data(i, ColPos::In::bid);
		bar.bid_high = data(i, ColPos::In::bid_high);
		bar.bid_low = data(i, ColPos::In::bid_low);

		bar.ask = data(i, ColPos::In::ask);
		bar.ask_high = data(i, ColPos::In::ask_high);
		bar.ask_low = data(i, ColPos::In::ask_low);

		bar.ask_size = static_cast<unsigned>(data(i, ColPos::In::ask_size));
		bar.bid_size = static_cast<unsigned>(data(i, ColPos::In::bid_size));
	}
}

void DataController::flush(const timestamp_t& ts_step)
{
}

void DataController::flush(const size_t& pos, const timestamp_t& next_ts)
{
	// (1min bar is filled with latest data)

	const auto ts_dbl = static_cast<double>(next_ts);
	const auto flush10sec = static_cast<timestamp_t>(ts_dbl / 10.0) * 10 == next_ts;
	if (flush10sec)
	{
		const auto flush1min = static_cast<timestamp_t>(ts_dbl / 60.0) * 60 == next_ts;
		const auto flush5min = static_cast<timestamp_t>(ts_dbl / 300.0) * 300 == next_ts;
		const auto flush15min = static_cast<timestamp_t>(ts_dbl / 900.0) * 900 == next_ts;
		const auto flush1hr = static_cast<timestamp_t>(ts_dbl / 3600.0) * 3600 == next_ts;
		const auto flush4hr = static_cast<timestamp_t>(ts_dbl / 14400.0) * 14400 == next_ts;
		if (flush5min)  // 5min
		{
			// flush 1min bars into 5min
			auto& bars_5min = bars_5min_[pos];
			const auto& bars_1min = bars_1min_[pos];
			// shift 5min bars
			shift_bars_3(bars_5min);
			// sum all 1min into 5min[0]
			copy_bar_5(bars_5min[0], bars_1min[0], bars_1min[1], bars_1min[2], bars_1min[3], bars_1min[4]);

			std::cout << "latest 5min bar:" << bars_5min[0] << std::endl;

			// copy into symbols_data_
			// shift down
			auto& data_5min = (*symbols_5min_)[pos];
			// TODO copy here?? (otherwise allocated on the stack?!)
			const auto data_copy = xt::roll(data_5min, 1, 0);
			data_5min = data_copy;

			// insert latest candle in [0]
			insert_candle(data_5min, bars_5min[0]);

			std::cout << "5min data:" << std::endl << data_5min << std::endl;
		}

		if (flush15min)  // 15min
		{
			// flush 5min bars into 15min
			auto& bars_15min = bars_15min_[pos];
			const auto& bars_5min = bars_5min_[pos];
			shift_bars_4(bars_15min);
			copy_bar_3(bars_15min[0], bars_5min[0], bars_5min[1], bars_5min[2]);

			// copy into symbols_data_
			// shift down
			auto& data_15min = (*symbols_15min_)[pos];
			data_15min = xt::roll(data_15min, 1, 0);

			// insert latest candle in [0]
			insert_candle(data_15min, bars_15min[0]);

			std::cout << "15min data:" << std::endl << data_15min << std::endl;
		}

		if (flush1hr)  // 1hr
		{
			// flush 15min bars into 1hr
			auto& bars_1hr = bars_1hr_[pos];
			const auto& bars_15min = bars_15min_[pos];
			shift_bars_4(bars_1hr);
			copy_bar_4(bars_1hr[0], bars_15min[0], bars_15min[1], bars_15min[2], bars_15min[3]);

			// copy into symbols_data_
			// shift down
			auto& data_1hr = (*symbols_1hr_)[pos];
			data_1hr = xt::roll(data_1hr, 1, 0);

			// insert latest candle in [0]
			insert_candle(data_1hr, bars_1hr[0]);

			std::cout << "1hr data:" << std::endl << data_1hr << std::endl;
		}
		if (flush4hr)  // 4hr
		{
			// flush 1hr bars into 4hr
			Bar bar4hr;
			const auto& bars_1hr = bars_1hr_[pos];
			copy_bar_4(bar4hr, bars_1hr[0], bars_1hr[1], bars_1hr[2], bars_1hr[3]);

			// copy into symbols_data_
			// shift down
			auto& data_4hr = (*symbols_4hr_)[pos];
			data_4hr = xt::roll(data_4hr, 1, 0);

			// insert latest candle in [0]
			insert_candle(data_4hr, bar4hr);

			std::cout << "4hr data:" << std::endl << data_4hr << std::endl;
		}
		if (flush1min)  // 1min
		{
			// copy into symbols_data_
			// shift down
			auto& data_1min = (*symbols_1min_)[pos];
			data_1min = xt::roll(data_1min, 1, 0);

			// insert latest candle in [0]
			insert_candle(data_1min, bars_1min_[pos][0]);

			std::cout << "1min data:" << std::endl << data_1min << std::endl;
		}

		// notify AccountController (or other listener)
		if (on_update_)
		{
			const xtensor_raw data = xt::stack(xt::xtuple(
				xt::xarray<double>((*symbols_1min_)[pos]),
				xt::xarray<double>((*symbols_5min_)[pos]),
				xt::xarray<double>((*symbols_15min_)[pos]),
				xt::xarray<double>((*symbols_1hr_)[pos]),
				xt::xarray<double>((*symbols_4hr_)[pos]),
				xt::xarray<double>((*symbols_1d_)[pos]),
				xt::xarray<double>((*symbols_1w_)[pos])
			));
			on_update_(symbols_pos_rev_[pos], snapshots_[pos], data);
		}

		// TODO remove save for verification
		if (flush15min)
		{
			xt::dump_npy("pyfiles/_out.AAPL.1min.npy", (*symbols_1min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.5min.npy", (*symbols_5min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.15min.npy", (*symbols_15min_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.1hr.npy", (*symbols_1hr_)[pos]);
			xt::dump_npy("pyfiles/_out.AAPL.4hr.npy", (*symbols_4hr_)[pos]);
		}
	}
}

void DataController::process_trade_rt(const size_t& pos, const json& trade)
{
	// Note: assuming trade["ev"]/trade["sym"] has been handled/verified
	//
	// flush into 1min bar
	const int interval_seconds = 60;
	Bar& bar = bars_1min_[pos][0];  
	timestamp_t& ts_step = cur_timesteps_[pos];

	bool updates_volume = false;
	bool updates_high_low = false;
	bool updates_open_close = false;
	if (!trade.contains("c") || !trade["c"].is_array() || trade["c"].empty())
	{
		if (regular_trade_condition.updates_volume)
			updates_volume = true;
		if (regular_trade_condition.updates_high_low)
			updates_high_low = true;
		if (regular_trade_condition.updates_open_close)
			updates_open_close = true;
	}
	else
	{
		for (const json& cond : trade["c"])
		{
			const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : 0);
			if (c < conditions_len)
			{
				const trade_condition_t& tradeCond = conditions.at(c);
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

	const double ts = static_cast<double>(trade["t"].get<uint64_t>()) / 1.0e3;
	const auto& price = trade["p"].get<double>();
	const auto& vol = trade["s"].get<uint32_t>();

	const timestamp_t next_ts = ts_step + interval_seconds;
	if (ts >= static_cast<double>(next_ts))
	{
		// flush last bar
		// Note: this introduces delay to the flush routine (by waiting for next timestamp before processing previous)
		//   This should be changed after moving to a faster network, then preempt end-of-interval
		if (next_ts > interval_seconds) {  // only flush if this is not the first payload
			flush(pos, next_ts);
		}

		// reset this bar
		bar.timestamp = static_cast<double>(static_cast<uint64_t>(ts / interval_seconds) * interval_seconds);  // TODO verify
		//bar.timestamp = next_ts;
		if (static_cast<timestamp_t>(bar.timestamp) != next_ts && next_ts > interval_seconds) {
			std::cout << "Skip timestamp " << next_ts << " (to " << bar.timestamp << ")" << std::endl;
		}
		bar.volume = vol;
		bar.low = bar.high = bar.open = bar.close = price;

		// reset ask/bid high/low to current bid/ask price
		bar.ask_high = bar.ask_low = bar.ask;
		bar.bid_high = bar.bid_low = bar.bid;
		// TODO Note: leave ask/bid and ask_size/bid_size the same ??

		ts_step = static_cast<timestamp_t>(bar.timestamp);  // next_ts;
	}
	else
	{
		// append to existing bar!
		if (updates_high_low)
		{
			if (price > bar.high)
				bar.high = price;
			if (bar.low == 0 || price < bar.low)
				bar.low = price;
		}
		if (updates_open_close)
		{
			bar.close = price;
			if (bar.open == 0)
			{
				// TODO should never get here?
				std::cout << "process_trade_rt() update open/close; open=0; ts=" << ts << "; next_ts=" << next_ts << std::endl;
				bar.open = price;
				if (bar.high == 0)
					bar.high = price;
				if (bar.low == 0)
					bar.low = price;
			}

		}
		if (updates_volume)
			bar.volume += vol;
	}
	/*
	trade["ev"]; // Event Type (T for trade)

	trade["sym"]; // symbol
	trade["x"]; // Exchange ID
	trade["i"]; // trade ID
	trade["z"]; // tape (1 = NYSE, 2 = AMEX, 3 = Nasdaq)
	trade["p"]; // price
	trade["s"]; // size
	trade["c"]; // conditions
	trade["t"]; // timestamp
	*/
}

void DataController::process_quote_rt(const size_t& pos, const json& quote)
{
	// Note: assuming quote["ev"]/quote["sym"] has been handled/verified
	
	// flush into 1min bar
	const int interval_seconds = 60;
	Bar& bar = bars_1min_[pos][0];
	timestamp_t& ts_step = cur_timesteps_[pos];

	/*
	// TODO consider condition?
	bool updates_volume = false;
	bool updates_high_low = false;
	bool updates_open_close = false;
	if (!quote.contains("c") || !quote["c"].is_array() || quote["c"].empty())
	{
		if (regular_trade_condition.updates_volume)
			updates_volume = true;
		if (regular_trade_condition.updates_high_low)
			updates_high_low = true;
		if (regular_trade_condition.updates_open_close)
			updates_open_close = true;
	}
	else
	{
		for (const json& cond : quote["c"])
		{
			const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : 0);
			if (c < conditions_len)
			{
				const trade_condition_t& tradeCond = conditions.at(c);
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
	const double ts = static_cast<double>(quote["t"].get<uint64_t>()) / 1.0e3;
	const auto& askPrice = quote["ap"].get<double>();
	const auto& askSize = quote["as"].get<uint32_t>();
	const auto& bidPrice = quote["bp"].get<double>();
	const auto& bidSize = quote["bs"].get<uint32_t>();

	const timestamp_t next_ts = ts_step + interval_seconds;
	if (ts >= static_cast<double>(next_ts))
	{
		// flush last bar
		// Note: this introduces delay to the flush routine (by waiting for next timestamp before processing previous)
		//   This should be changed after moving to a faster network, then preempt end-of-interval
		if (next_ts > interval_seconds) {  // only flush if this is not the first payload
			flush(pos, next_ts);
		}

		// TODO copy last close price into this bar?
		const auto price = bar.close;

		// reset this bar
		bar.timestamp = static_cast<double>(static_cast<uint64_t>(ts / interval_seconds) * interval_seconds);  // TODO verify
		//bar.timestamp = next_ts;
		if (static_cast<timestamp_t>(bar.timestamp) != next_ts && next_ts > interval_seconds) {
			std::cout << "Skip timestamp " << next_ts << " (to " << bar.timestamp << ")" << std::endl;
		}
		bar.volume = 0;  // so far, 0 volume in this bar
		bar.low = bar.high = bar.open = bar.close = price;  // TODO copy last close price ?

		// set bid/ask and reset the high/lows
		bar.ask = bar.ask_high = bar.ask_low = askPrice;
		bar.bid = bar.bid_high = bar.bid_low = bidPrice;
		bar.ask_size = askSize;
		bar.bid_size = bidSize;

		ts_step = static_cast<timestamp_t>(bar.timestamp);  // next_ts;
	}
	else
	{
		// append to existing bar!
		if (askPrice > bar.ask_high)
			bar.ask_high = askPrice;
		if (bar.ask_low == 0 || askPrice < bar.ask_low)
			bar.ask_low = askPrice;
		if (bidPrice > bar.bid_high)
			bar.bid_high = bidPrice;
		if (bar.bid_low == 0 || bidPrice < bar.bid_low)
			bar.bid_low = bidPrice;
		bar.ask = askPrice;
		bar.bid = bidPrice;
		bar.ask_size = askSize;
		bar.bid_size = bidSize;
	}

	// always notify AccountController (or other listener) when bid/ask changes
	if (on_snapshot_)
		on_snapshot_(symbols_pos_rev_[pos], snapshots_[pos]);

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

void DataController::onPayloads(const char* json_str)
{
	const json& payloads = json::parse(json_str);
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

void DataController::onTradePayload(const json& trade)
{
	//std::cout << "onTradePayload() " << trade << std::endl;
	
	const auto& ticker = trade["sym"].get<std::string>();
	const Symbol& symbol = Symbol::get_symbol(ticker);  // TODO
	const auto& pos = symbols_pos_[symbol];
	
	process_trade_rt(pos, trade);
}

void DataController::onNbboPayload(const json& quote)
{
	//std::cout << "onNbboPayload() " << quote << std::endl;

	const auto& ticker = quote["sym"].get<std::string>();
	const Symbol& symbol = Symbol::get_symbol(ticker);  // TODO
	const auto& pos = symbols_pos_[symbol];

	process_quote_rt(pos, quote);
}



void DataController::initSymbol(const Symbol& symbol)
{
	const auto now = std::chrono::system_clock::now();
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

    for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS)
    {
	    switch (std::get<0>(tpl))
	    {
	    case 1:
		    {
			    const timestamp_t start_ts = req_end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_1min_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, req_end_ts, true, std::get<2>(tpl));
			    //// now copy from dest into the staging bars array...
			    //copy_candles(bars_1min_[pos], dest);
			    break;
		    }
	    case 5:
		    {
			    const timestamp_t end_ts = (req_end_ts / 300) * 300;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_5min_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true,std::get<2>(tpl));
			    //copy_candles(bars_5min_[pos], dest);
			    break;
		    }
	    case 15:
		    {
			    const timestamp_t end_ts = (req_end_ts / 900) * 900;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_15min_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    //copy_candles(bars_15min_[pos], dest);
			    break;
		    }
	    case 60:
		    {
			    const timestamp_t end_ts = (req_end_ts / 3600) * 3600;
			    const timestamp_t start_ts = end_ts - std::get<1>(tpl);
			    xtensor_raw_interval& dest = (*symbols_1hr_)[pos];
			    PolygonIoAdapter::getAggregateHistory(dest, symbol.symbol, std::get<0>(tpl), start_ts, end_ts, true, std::get<2>(tpl));
			    //copy_candles(bars_1hr_[pos], dest);
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

	// now download quotes/NBBO up to this timestamp, and merge the bid/ask prices...

	// TODO yar, do not create temporary to perform this o.0
	xtensor_raw data = xt::stack(xt::xtuple(
		xt::xarray<double>((*symbols_1min_)[pos]),
		xt::xarray<double>((*symbols_5min_)[pos]),
		xt::xarray<double>((*symbols_15min_)[pos]),
		xt::xarray<double>((*symbols_1hr_)[pos]),
		xt::xarray<double>((*symbols_4hr_)[pos]),
		xt::xarray<double>((*symbols_1d_)[pos]),
		xt::xarray<double>((*symbols_1w_)[pos])
	));
	if (PolygonIoAdapter::getQuoteHistory(data, symbol.symbol, req_end_ts))
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

	// copy into staging bars
    for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS)
    {
	    switch (std::get<0>(tpl))
	    {
	    case 1:
		    {
			    xtensor_raw_interval& dest = (*symbols_1min_)[pos];
			    copy_candles(bars_1min_[pos], dest);
			    break;
		    }
	    case 5:
		    {
			    xtensor_raw_interval& dest = (*symbols_5min_)[pos];
			    copy_candles(bars_5min_[pos], dest);
			    break;
		    }
	    case 15:
		    {
			    xtensor_raw_interval& dest = (*symbols_15min_)[pos];
			    copy_candles(bars_15min_[pos], dest);
			    break;
		    }
	    case 60:
		    {
			    xtensor_raw_interval& dest = (*symbols_1hr_)[pos];
			    copy_candles(bars_1hr_[pos], dest);
			    break;
		    }
		default:
			break;
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
