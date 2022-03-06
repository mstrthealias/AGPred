#include "consolidate.h"

#include <iostream>

#include <websocketpp/client.hpp>



int test_fn()
{
	
    return 0;
}

json trade_rt = {
	{"ev", "T"},
	{"sym", "MSFT"},
	{"x", 4},
	{"i", "12345"},
	{"z", 3},
	{"p", 114.125},
	{"s", 100},
	{
		"c", json({
			0,
			12
		})
	},
	{"t", 1536036818784}
};

json trade = {
	{"c", json({12, 41})},
	{"i", "1"},
	{"p", 171.55},
	{"q", 1063},
	{"s", 100},
	{"t", 1517562000016036600},
	{"x", 11},
	{"y", 1517562000015577000},
	{"z", 3},
};


json agg_1min = {
	{"c", 75.0875},
	{"h", 75.15},
	{"l", 73.7975},
	{"n", 1},
	{"o", 74.06},
	{"t", 1577941200000},
	{"v", 135647456},
	{"vw", 74.6099},
};

json agg_1min_rt = {
	{"ev", "AM"},
	{"sym", "GTE"},
	{"v", 4110},
	{"av", 9470157},
	{"op", 0.4372},
	{"vw", 0.4488},
	{"o", 0.4488},
	{"c", 0.4486},
	{"h", 0.4489},
	{"l", 0.4486},
	{"a", 0.4352},
	{"z", 685},
	{"s", 1610144640000},
	{"e", 1610144700000},
};


void process_agg_rt(const json& agg)
{
	agg["ev"]; // Event Type (AM for realtime minute aggregate)

	agg["sym"]; // symbol
	agg["v"]; //  volume
	agg["av"]; // Today's accumulated volume
	agg["op"]; // Today's official opening price
	agg["vw"]; // vwap
	agg["o"]; // open
	agg["c"]; // close
	agg["h"]; // high
	agg["l"]; // low
	agg["a"]; // Today's volume weighted average price
	agg["z"]; // average trade size for this aggregate window
	agg["s"]; // timestamp of the starting tick
	agg["e"]; // timestamp of the ending tick
}


void process_trade(const json& trade, const int interval, timestamp_us_t& tsStep, Bar& bar, std::vector<Bar>& bars)
{
	const timestamp_us_t interval_us = interval / 1e6;  // us
	bool updates_volume = false;
	bool updates_high_low = false;
	bool updates_open_close = false;
	if (!trade.contains("c") || !trade["c"].is_array() || trade["c"].empty())
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
		for (const json& cond : trade["c"])
		{
			// TODO skip if !cond.is_number_integer() ??
			//if (!cond.is_number_integer())
			//	continue;
			const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : 0);
			if (c < CONDITIONS_LEN)
			{
				const trade_condition_t& tradeCond = CONDITIONS.at(c);
				if (tradeCond.updates_volume)
					updates_volume = true;
				else
					std::cout << "tradeCond " << tradeCond.name << " skip vol update " << trade["s"].get<uint32_t>() << std::endl;
				if (tradeCond.updates_consolidated_high_low)
					updates_high_low = true;
				if (tradeCond.updates_consolidated_open_close)
					updates_open_close = true;
				/*if (tradeCond.updates_high_low)
					updates_high_low = true;
				if (tradeCond.updates_open_close)
					updates_open_close = true;*/
			}
		}
	}
	if (!updates_high_low && !updates_open_close && !updates_volume)
		return;  // no more processing to do (for aggregate generation)

	const timestamp_us_t ts = static_cast<timestamp_us_t>(trade["t"].get<uint64_t>() / US_TO_NS);  // ns->us
	const real_t price = trade["p"].get<double>();
	const auto& vol = trade["s"].get<uint32_t>();

	const timestamp_us_t nextTs = tsStep + interval_us;
	if (ts >= nextTs)
	{
		// create a new bar!
		if (bar.volume)
			bars.push_back(bar);  // copy previous bar into bars

		// reset this bar
		bar.timestamp = static_cast<timestamp_us_t>(ts / interval_us) * interval_us;  // TODO verify
		//bar.timestamp = nextTs;
		if (bar.timestamp != nextTs) {  // !dbl_equal(bar.timestamp, nextTs)
			std::cout << "Skip timestamp " << nextTs << " (to " << bar.timestamp << ")" << std::endl;
		}
		bar.volume = vol;
		bar.low = bar.high = bar.open = bar.close = price;

		tsStep = bar.timestamp;  // nextTs;
	}
	else
	{
		// append to existing bar!
		if (updates_high_low)
		{
			if (price > bar.high)
				bar.high = price;
			if (price < bar.low)
				bar.low = price;
		}
		if (updates_open_close)
			bar.close = price;
		if (updates_volume)
			bar.volume += vol;
	}
	/*
	trade["c"]; // conditions
	trade["i"]; // trade ID
	trade["p"]; // price
	trade["q"]; // sequence number
	trade["s"]; // size
	trade["t"]; // The nanosecond accuracy SIP Unix Timestamp. This is the timestamp of when the SIP received this message from the exchange which produced it.
	trade["x"]; // Exchange ID
	trade["y"]; // The nanosecond accuracy Participant/Exchange Unix Timestamp. This is the timestamp of when the quote was actually generated at the exchange.
	trade["z"]; // tape (1 = NYSE, 2 = AMEX, 3 = Nasdaq)
	*/
}

/*int process_trades_rt(const json& trades, const int interval, std::vector<Bar>& bars)
{
	if (!trades.is_array() || trades.empty())
		return -1; // trades must be a non-empty array of 'trades'

	std::cout << "Number of inputs/trades: " << trades.size() << std::endl;

	real_t tsStep = static_cast<int>(static_cast<real_t>(trades[0]["t"].get<uint64_t>()) / 1.0e9 / 60) * 60.0;
	Bar bar;

	for (const auto& trade : trades)
	{
		// std::cout << "trade: " << trade << std::endl;
		process_trade_rt(trade, interval, tsStep, bar, bars);
	}

	std::cout << "Number of aggregates: " << bars.size() << std::endl;
	return 0;
}*/


int process_trades_history(const json& trades, const int interval, const timestamp_us_t& startTs, std::vector<Bar>& bars)
{
	if (!trades.is_array())
		return -1; // trades must be an array of 'trades'

	std::cout << "Number of inputs/trades: " << trades.size() << std::endl;

	timestamp_us_t tsStep = static_cast<timestamp_us_t>((trades[0]["t"].get<uint64_t>() / US_TO_NS) / (60 * SEC_TO_US)) * (60 * SEC_TO_US);  // startTs;
	//timestamp_us_t tsStep = static_cast<int>(trades[0]["t"].get<uint64_t>() / 1.0e9 / 60) * 60.0;  // startTs;
	//timestamp_us_t tsStep = startTs;
	Bar bar;

	for (const auto& trade : trades)
	{
		// std::cout << "trade: " << trade << std::endl;
		process_trade(trade, interval, tsStep, bar, bars);
	}

	std::cout << "Number of aggregates: " << bars.size() << std::endl;
	return 0;
}

xt::xarray<real_t> bars_to_xt(const std::vector<Bar>& bars)
{
	const int n_bars = static_cast<int>(bars.size());
	xt::xarray<real_t> results = xt::zeros<real_t>({6, n_bars });
	int pos = 0;
	for (const auto& bar : bars)
	{
		results(0, pos) = static_cast<real_t>(bar.timestamp);  // TODO subtract 37 years ?
		results(1, pos) = bar.open;
		results(2, pos) = bar.high;
		results(3, pos) = bar.low;
		results(4, pos) = bar.close;
		results(5, pos) = static_cast<real_t>(bar.volume);
		pos++;
	}
	return results;
}

xt::xarray<real_t> process_trades_json(const char* json_str, const int interval, const timestamp_us_t& start_ts)
{
	const json& trades = json::parse(json_str);

	std::cout << "trades.type_name: " << trades.type_name() << std::endl;

	std::cout << "trades.is_array: " << trades.is_array() << std::endl;
	
	std::vector<Bar> bars(0);  // TODO allocate # of bars?

	if (process_trades_history(trades, interval, start_ts, bars) != 0)
	{
		std::cout << "ERROR process_trades_history() != 0: " << std::endl;
	}

	return bars_to_xt(bars);
}
