#include "polygon_io.h"

#include <time.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <xtensor/xview.hpp>
#include <xtensor/xdynamic_view.hpp>
#include <xtensor/xindex_view.hpp>
#include <xtensor/xio.hpp>

#include "../src/consolidate.h"


using namespace agpred;


static const json JSON_NULL = json::parse("null");


inline void process_agg(Bar& dest, const json& agg)
{
	dest.timestamp = static_cast<double>(agg["t"].get<uint64_t>()) / 1.0e3;
	dest.open = agg["o"].get<double>();
	dest.high = agg["h"].get<double>();
	dest.low = agg["l"].get<double>();
	dest.close = agg["c"].get<double>();
	dest.volume = agg["v"].get<uint32_t>();

	/*
	agg["o"];  // open
	agg["c"];  // close
	agg["h"];  // high
	agg["l"];  // low
	agg["n"];  // number of transactions in the aggregate window
	agg["t"];  // timestamp for the start of the aggregate window
	agg["v"];  // volume
	agg["vw"]; // vwap
	*/
}

inline void process_agg2(xtensor_raw_interval& dest, const uint32_t i, const json& agg)
{
	dest(i, ColPos::In::timestamp) = static_cast<double>(agg["t"].get<uint64_t>()) / 1.0e3;
	dest(i, ColPos::In::open) = agg["o"].get<double>();
	dest(i, ColPos::In::high) = agg["h"].get<double>();
	dest(i, ColPos::In::low) = agg["l"].get<double>();
	dest(i, ColPos::In::close) = agg["c"].get<double>();
	dest(i, ColPos::In::volume) = static_cast<double>(agg["v"].get<uint32_t>());

	/*
	agg["o"];  // open
	agg["c"];  // close
	agg["h"];  // high
	agg["l"];  // low
	agg["n"];  // number of transactions in the aggregate window
	agg["t"];  // timestamp for the start of the aggregate window
	agg["v"];  // volume
	agg["vw"]; // vwap
	*/
}

inline void payload_v2_to_quote_data(QuoteData& quote_data, const json& quote)
{
	const timestamp_t cur_ts = quote["t"].get<uint64_t>() / static_cast<timestamp_t>(1e9);
	const auto& askPrice = quote["P"].get<double>();
	const auto& askSize = quote["S"].get<uint32_t>();
	const auto& bidPrice = quote["p"].get<double>();
	const auto& bidSize = quote["s"].get<uint32_t>();

	QuoteCondition conds[] = { QuoteCondition::Invalid, QuoteCondition::Invalid, QuoteCondition::Invalid };
	if (quote.contains("c"))
	{
		const auto& q_conds = quote["c"];
		if (q_conds.is_array() && !q_conds.empty())
		{
			unsigned int cond_pos = 0;
			for (const json& cond : q_conds)
			{
				const int32_t c = (cond.is_number_integer() ? cond.get<int32_t>() : QUOTE_PLACEHOLDER);
				if (cond_pos < 3 && c != QUOTE_PLACEHOLDER && c <= MAX_VALID_QUOTE_CONDITION)
					conds[cond_pos++] = static_cast<QuoteCondition>(c);
			}
		}
	}

	quote_data = {
		{
			cur_ts,
			bidPrice,
			askPrice,
			bidSize,
			askSize
		},
		{conds[0], conds[1], conds[2]}
	};
}

inline void payload_vx_to_quote_data(QuoteData& quote_data, const json& quote)
{
	const timestamp_t cur_ts = quote["sip_timestamp"].get<uint64_t>() / static_cast<timestamp_t>(1e9);
	const auto& askPrice = quote["ask_price"].get<double>();
	const auto& askSize = quote["ask_size"].get<uint32_t>();
	const auto& bidPrice = quote["bid_price"].get<double>();
	const auto& bidSize = quote["bid_size"].get<uint32_t>();

	QuoteCondition conds[] = { QuoteCondition::Invalid, QuoteCondition::Invalid, QuoteCondition::Invalid };
	if (quote.contains("conditions"))
	{
		const auto& q_conds = quote["conditions"];
		if (q_conds.is_array() && !q_conds.empty())
		{
			unsigned int cond_pos = 0;
			for (const json& cond : q_conds)
			{
				const int32_t c = (cond.is_number_integer() ? cond.get<int32_t>() : QUOTE_PLACEHOLDER);
				if (cond_pos < 3 && c != QUOTE_PLACEHOLDER && c <= MAX_VALID_QUOTE_CONDITION)
					conds[cond_pos++] = static_cast<QuoteCondition>(c);
			}
		}
	}

	quote_data = {
		{
			cur_ts,
			bidPrice,
			askPrice,
			bidSize,
			askSize
		},
		{conds[0], conds[1], conds[2]}
	};
}

void adapter::payload_rt_to_quote_data(QuoteData& quote_data, const json& quote)
{
	const timestamp_t cur_ts = quote["t"].get<uint64_t>() / static_cast<timestamp_t>(1e9);
	const auto& askPrice = quote["ap"].get<double>();
	const auto& askSize = quote["as"].get<uint32_t>();
	const auto& bidPrice = quote["bp"].get<double>();
	const auto& bidSize = quote["bs"].get<uint32_t>();

	QuoteCondition conds[] = { QuoteCondition::Invalid, QuoteCondition::Invalid, QuoteCondition::Invalid };
	if (quote.contains("c"))
	{
		const auto& q_conds = quote["c"];
		if (q_conds.is_array() && !q_conds.empty())
		{
			unsigned int cond_pos = 0;
			for (const json& cond : q_conds)
			{
				const int32_t c = (cond.is_number_integer() ? cond.get<int32_t>() : QUOTE_PLACEHOLDER);
				if (cond_pos < 3 && c != QUOTE_PLACEHOLDER && c <= MAX_VALID_QUOTE_CONDITION)
					conds[cond_pos++] = static_cast<QuoteCondition>(c);
			}
		}
	}

	quote_data = {
		{
			cur_ts,
			bidPrice,
			askPrice,
			bidSize,
			askSize
		},
		{conds[0], conds[1], conds[2]}
	};

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


inline void payload_vx_to_trade_data(TradeData& trade_data, const json& trade)
{
	const timestamp_t cur_ts = trade["sip_timestamp"].get<uint64_t>() / static_cast<timestamp_t>(1e9);
	const auto& price = trade["price"].get<double>();
	const auto& size = trade["size"].get<uint32_t>();

	TradeCondition conds[] = { TradeCondition::PLACEHOLDER, TradeCondition::PLACEHOLDER, TradeCondition::PLACEHOLDER };
	if (trade.contains("conditions"))
	{
		const auto& conditions = trade["conditions"];
		if (conditions.is_array() && !conditions.empty())
		{
			unsigned int cond_pos = 0;
			for (const json& cond : conditions)
			{
				const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : TRADE_PLACEHOLDER);
				if (c != TRADE_PLACEHOLDER && c < MAX_VALID_TRADE_CONDITION && cond_pos < 3)
					conds[cond_pos++] = static_cast<TradeCondition>(c);
			}
		}
	}

	trade_data = {
		{
			cur_ts,
			price,
			size
		},
		{conds[0], conds[1], conds[2]}
	};
}

void adapter::payload_rt_to_trade_data(TradeData& trade_data, const json& trade)
{
	const timestamp_t cur_ts = trade["t"].get<uint64_t>() / static_cast<timestamp_t>(1e9);
	const auto& price = trade["p"].get<double>();
	const auto& size = trade["s"].get<uint32_t>();

	TradeCondition conds[] = { TradeCondition::PLACEHOLDER, TradeCondition::PLACEHOLDER, TradeCondition::PLACEHOLDER };
	if (trade.contains("c"))
	{
		const auto& conditions = trade["c"];
		if (conditions.is_array() && !conditions.empty())
		{
			unsigned int cond_pos = 0;
			for (const json& cond : conditions)
			{
				const uint32_t c = (cond.is_number_integer() ? cond.get<uint32_t>() : TRADE_PLACEHOLDER);
				if (c != TRADE_PLACEHOLDER && c < MAX_VALID_TRADE_CONDITION && cond_pos < 3)
					conds[cond_pos++] = static_cast<TradeCondition>(c);
			}
		}
	}

	trade_data = {
		{
			cur_ts,
			price,
			size
		},
		{conds[0], conds[1], conds[2]}
	};

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



size_t PolygonIoAdapter::getAggregateHistory(xtensor_raw_interval& dest, const std::string& symbol, unsigned int interval, timestamp_t start_ts, timestamp_t end_ts, bool adjusted, unsigned int limit)
{
	std::string base_url = POLYGON_BASE_URI + "v2/aggs/ticker/" + std::string(symbol) + "/range/";
	if (interval < 60)
		base_url = base_url + std::to_string(interval) + "/minute/";
	else if (interval <= 720)
		base_url = base_url + std::to_string(static_cast<unsigned int>(interval / 60)) + "/hour/";
	else if (interval == 1440)
		base_url = base_url + "1/day/";
	else if (interval == 10080)
		base_url = base_url + "1/week/";
	else
		assert(false);
	base_url = base_url + std::to_string(start_ts * 1000) + "/" + std::to_string(end_ts * 1000);

	cpr::Response r = cpr::Get(
		cpr::Url{ base_url },
		cpr::Parameters{
			{"adjusted", (adjusted ? "true" : "false")},
			{"limit", std::to_string(limit)},
			{"sort", "desc"},
			{"apiKey", POLYGON_API_KEY}
		}
	);

	json res = json::parse(r.text);
	const size_t count = res["count"].is_null() || !res["results"].is_array() ? 0 : res["count"].get<size_t>();
	if (!count) {
		std::cout << symbol << "[" << interval << "] ERROR:" << std::endl << "\t" << res << std::endl;
	}
	else {
		std::cout << symbol << "[" << interval << "] count=" << count << std::endl;
		//std::vector<Bar> bars = std::vector<Bar>(count);
		uint32_t i = 0;
		for (const json& agg : res["results"])
		{
			if (i >= RT_MAX_TIMESTEPS)
				break;
			//// create view referencing the current row/timestep
			//xt::xview<double, uint32_t> row = xt::view(dest, xt::range(i, i + 1), xt::all());
			process_agg2(dest, i++, agg);
			//process_agg(bars[i++], agg);
		}
	}

	return count;
}


size_t PolygonIoAdapter::mergeQuotesAggregates(xtensor_raw_255& dest, const std::string& symbol, timestamp_t end_ts, unsigned int limit)
{
	const auto end_time = static_cast<time_t>(end_ts);
	auto end_tm = tm();
	localtime_s(&end_tm, &end_time);  // Note: formatting day only, so assuming string localtime will match ET tz

	const std::string url = POLYGON_BASE_URI + "v2/ticks/stocks/nbbo/" + std::string(symbol) + '/' + std::to_string(1900 + end_tm.tm_year) + '-' + (end_tm.tm_mon + 1 < 10 ? "0" : "") + std::to_string(end_tm.tm_mon + 1) + '-' + (end_tm.tm_mday < 10 ? "0" : "") + std::to_string(end_tm.tm_mday);

	char tmp_str[26];
	time_t tmp_time;

	std::array<size_t, NUM_INTERVALS> cur_rows = { 0, 0, 0, 0, 0, 0, 0 };  // fe. 1min, 5min, 15min, 1hr, 4hr, 1d, 1wk
	std::array<timestamp_t, NUM_INTERVALS> ts_steps = { 0, 0, 0, 0, 0, 0, 0 };
	
	size_t res_cnt;
	size_t count = 0;
	unsigned int req_cnt = 0;
	std::string last_timestamp = std::to_string((end_ts - 60 * 60 * 8) * static_cast<long long>(1e9));
	cpr::Response r;
	do
	{
		r = cpr::Get(
			cpr::Url{ url },
			cpr::Parameters{
				{"limit", std::to_string(limit)},
				{"reverse", "false"},
				{"timestamp", last_timestamp},  // Used for pagination (offset at which to start the results). Using the timestamp of the last result to retrieve next page. // nanosec
				{"timestampLimit", std::to_string(end_ts * static_cast<long long>(1e9))}, // nanosec, fe. sec*9
				{"apiKey", POLYGON_API_KEY}
			}
		);

		json res = json::parse(r.text);
		res_cnt = res["results_count"].is_null() || !res["results"].is_array() ? 0 : res["results_count"].get<size_t>();
		if (!res_cnt) {
			std::cout << symbol << " ERROR:" << std::endl << "\t" << res << std::endl;
		}
		else {
			count += res_cnt;
			std::cout << symbol << " count=" << res_cnt << std::endl;

			// update last_timestamp to the timestamp of the last result (to retrieve the next page, if res_cnt >= limit)
			auto& first_entry = res["results"].front();
			auto first_timestamp = first_entry["t"].get<uint64_t>() / static_cast<uint64_t>(1e9);
			tmp_time = static_cast<time_t>(first_timestamp);
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "mergeQuotesAggregates() first_timestamp: " << tmp_str;  //  << std::endl

			auto& last_entry = res["results"].back();
			last_timestamp = std::to_string(last_entry["t"].get<uint64_t>());  // nanosecond
			tmp_time = static_cast<time_t>(last_entry["t"].get<uint64_t>() / static_cast<uint64_t>(1e9));
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "mergeQuotesAggregates() last_timestamp: " << tmp_str;  // << std::endl
			////std::cout << "mergeQuotesAggregates() last_timestamp: " << last_timestamp << std::endl;
			
			for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS)
			{
				const unsigned int& interval = std::get<0>(tpl);
				const int i_loc = static_cast<int>(std::get<3>(tpl));
				const size_t interval_seconds = static_cast<size_t>(interval) * 60;
				auto i_dest = xt::dynamic_view(dest, { i_loc, xt::all(), xt::all() });  // shape: (timesteps, cols)

				if (ts_steps[i_loc] == 0)
				{
					// find the first non-zero timestamp
					auto i_timestamps = xt::view(i_dest, xt::all(), 0);  // shape: (timesteps,)
					auto i_where = xt::where(i_timestamps >= first_timestamp);
					if (i_where.empty())
					{
						//std::cout << "mergeQuotesAggregates() [" << interval << "] !i_where.size()" << std::endl;
						continue;
					}
					auto& ii_where = i_where[0];
					if (ii_where.empty())
					{
						//std::cout << "mergeQuotesAggregates() [" << interval << "] !ii_where.size()" << std::endl;
						continue;
					}
					size_t next_pos = ii_where[ii_where.size() - 1];
					//std::cout << "mergeQuotesAggregates() [" << interval << "] next_pos=" << next_pos << std::endl;

					if (next_pos == NUM_TIMESTEMPS - 1)
					{
						std::cout << "mergeQuotesAggregates() [" << interval << "] quotes more recent than oldest timestep" << std::endl;

						// TODO include times in message?
						auto next_pos_ts = i_timestamps(next_pos);
						tmp_time = static_cast<time_t>(next_pos_ts);
						ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
						std::cout << "mergeQuotesAggregates() [" << interval << "] next_pos TS: " << tmp_str;  // << std::endl

						continue;
					}
					
					// set the row pointer to the matched timestamp
					ts_steps[i_loc] = static_cast<timestamp_t>(i_timestamps(next_pos));
					cur_rows[i_loc] = next_pos;
				}

				auto& dest_ts = ts_steps[i_loc];
				auto& dest_row = cur_rows[i_loc];
				//tmp_time = static_cast<time_t>(dest_ts);
				//ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
				//std::cout << "mergeQuotesAggregates() [" << interval << "] in  dest_ts: " << tmp_str;  // << std::endl

				for (const json& quote : res["results"])
				{
					// seek dest_row to the position of the row in dest to update
					const timestamp_t cur_ts = quote["t"].get<uint64_t>() / static_cast<timestamp_t>(1e9);
					const auto& askPrice = quote["P"].get<double>();
					const auto& askSize = quote["S"].get<uint32_t>();
					const auto& bidPrice = quote["p"].get<double>();
					const auto& bidSize = quote["s"].get<uint32_t>();
					bool reset_bar = false;
					
					// seek to correct position
					while (cur_ts >= dest_ts)
					{
						if (dest_row < 1)
							break;
						dest_row--;
						dest_ts = static_cast<timestamp_t>(i_dest(dest_row, ColPos::In::timestamp));  // TODO subtract instead?
					}
					if (cur_ts >= dest_ts && dest_row < 1)
					{
						//std::cout << "mergeQuotesAggregates() [" << interval << "] BREAK (dest_row==0)" << std::endl;
						break;
					}


					const timestamp_t next_ts = dest_ts + interval_seconds;
					if (cur_ts >= next_ts)
					{
						if (dest_row < 1)
						{
							//std::cout << "mergeQuotesAggregates() [" << interval << "] BREAK (dest_row==0)" << std::endl;
							break;
						}
						dest_ts = next_ts;
						dest_row--;

						reset_bar = true;
					}

					// TODO view for row?
					auto& ask_size = i_dest(dest_row, ColPos::In::ask_size);
					auto& bid_size = i_dest(dest_row, ColPos::In::bid_size);
					auto& ask = i_dest(dest_row, ColPos::In::ask);
					auto& ask_high = i_dest(dest_row, ColPos::In::ask_high);
					auto& ask_low = i_dest(dest_row, ColPos::In::ask_low);
					auto& bid = i_dest(dest_row, ColPos::In::bid);
					auto& bid_high = i_dest(dest_row, ColPos::In::bid_high);
					auto& bid_low = i_dest(dest_row, ColPos::In::bid_low);
					if (reset_bar)
					{
						// set bid/ask and reset the high/lows
						ask = ask_high = ask_low = askPrice;
						bid = bid_high = bid_low = bidPrice;
						ask_size = askSize;
						bid_size = bidSize;
					}
					else
					{
						// append to existing bar!
						if (askPrice > ask_high)
							ask_high = askPrice;
						if (ask_low == 0 || askPrice < ask_low)
							ask_low = askPrice;
						if (bidPrice > bid_high)
							bid_high = bidPrice;
						if (bid_low == 0 || bidPrice < bid_low)
							bid_low = bidPrice;
						ask = askPrice;
						bid = bidPrice;
						ask_size = askSize;
						bid_size = bidSize;
					}
				}

				//tmp_time = static_cast<time_t>(dest_ts);
				//ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
				//std::cout << "mergeQuotesAggregates() [" << interval << "] out dest_ts: " << tmp_str;  // << std::endl
			}
		}
	}
	while (++req_cnt < MAX_HISTORY_QUOTE_REQUESTS && res_cnt >= limit);
	
	return count;
}


size_t PolygonIoAdapter::getQuoteHistoryBefore(std::queue<QuoteData>& quotes, const std::string& symbol, timestamp_t end_ts, const size_t limit)
{
	if (limit > MAX_LIMIT)
		throw std::logic_error("limit must be <= MAX_LIMIT");

	const auto end_time = static_cast<time_t>(end_ts);
	auto end_tm = tm();
	localtime_s(&end_tm, &end_time);  // Note: formatting day only, so assuming string localtime will match ET tz
	
	char tmp_str[26];
	time_t tmp_time;

	// download the latest quotes (in reverse order)
	tmp_time = static_cast<time_t>(end_ts);
	ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
	std::cout << "getQuoteHistoryBefore() end_ts: " << tmp_str;  //  << std::endl

	//last_timestamp = std::to_string(end_ts * static_cast<long long>(1e9));

	// Note: using the vX version of endpoint here, since it handles desc order with max timestamp:
	const std::string url = POLYGON_BASE_URI + "vX/quotes/" + std::string(symbol);
	cpr::Response r = cpr::Get(
		cpr::Url{ url },  // see note above
		cpr::Parameters{
			{"limit", std::to_string(limit)},
			{"order", "desc"},
			{"timestamp.lte", std::to_string(end_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9
			{"apiKey", POLYGON_API_KEY}
			// for v2 (original) version of endpoint:
			//{"limit", std::to_string(limit)},
			//{"reverse", "true"},
			////{"timestamp", last_timestamp},  // Used for pagination (offset at which to start the results). Using the timestamp of the last result to retrieve next page. // nanosec
			//{"timestampLimit", std::to_string(end_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9
		}
	);
	
	json res = json::parse(r.text);
	const size_t res_cnt = res["count"].is_null() || !res["results"].is_array() ? 0 : res["count"].get<size_t>();
	if (!res_cnt) {
		std::cout << symbol << " LATEST QUOTES ERROR:" << std::endl << "\t" << res << std::endl;
	}
	else {
		if (DEBUG_PRINT_REQUESTS)
			std::cout << symbol << " count=" << res_cnt << std::endl;
		
		QuoteData quote_data;
		for (json::const_reverse_iterator i = res["results"].crbegin(); i != std::prev(res["results"].crend()); ++i) {
			const json& quote = *i;

			//// TODO catch instead of checking fields?
			//if (!quote.contains("ask_size") || !quote.contains("bid_size"))
			//	continue;

			try
			{
				payload_vx_to_quote_data(quote_data, quote);
			}
			catch (std::exception const& e)
			{
				std::cout << "getQuoteHistoryBefore() parse error: " << e.what() << std::endl;
				continue;
			}

			// TODO construct in place?
			quotes.emplace(quote_data);
		}

		if (DEBUG_PRINT_REQUESTS)
		{
			tmp_time = static_cast<time_t>(quotes.front().quote.timestamp);
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "getQuoteHistoryBefore() front timestamp: " << tmp_str;  //  << std::endl

			tmp_time = static_cast<time_t>(quotes.back().quote.timestamp);
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "getQuoteHistoryBefore() back timestamp: " << tmp_str;  // << std::endl
		}
	}
	return res_cnt;
}


size_t PolygonIoAdapter::getTradeHistoryBefore(std::queue<TradeData>& trades, const std::string& symbol, timestamp_t end_ts, const size_t limit)
{
	if (limit > MAX_LIMIT)
		throw std::logic_error("limit must be <= MAX_LIMIT");

	const std::string url = POLYGON_BASE_URI + "vX/trades/" + std::string(symbol);

	char tmp_str[26];
	time_t tmp_time;

	// download the latest trades (in reverse order)

	// TODO
	tmp_time = static_cast<time_t>(end_ts);
	ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
	std::cout << "getTradeHistoryBefore() end_ts: " << tmp_str;  //  << std::endl

	//last_timestamp = std::to_string(end_ts * static_cast<long long>(1e9));
	
	cpr::Response r = cpr::Get(
		cpr::Url{ url },
		cpr::Parameters{
			{"limit", std::to_string(limit)},
			{"order", "desc"},
			{"timestamp.lte", std::to_string(end_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9
			{"apiKey", POLYGON_API_KEY}
		}
	);

	json res = json::parse(r.text);
	const size_t res_cnt = res["count"].is_null() || !res["results"].is_array() ? 0 : res["count"].get<size_t>();
	size_t count = 0;
	if (!res_cnt) {
		std::cout << symbol << " LATEST TRADES ERROR:" << std::endl << "\t" << res << std::endl;
	}
	else {
		count += res_cnt;
		if (DEBUG_PRINT_REQUESTS)
			std::cout << symbol << " count=" << res_cnt << std::endl;
		
		TradeData trade_data;
		for (json::const_reverse_iterator i = res["results"].crbegin(); i != res["results"].crend(); ++i) {
			const json& trade = *i;
			
			try
			{
				payload_vx_to_trade_data(trade_data, trade);
			}
			catch (std::exception const& e)
			{
				std::cout << "getTradeHistoryBefore() parse error: " << e.what() << std::endl;
				continue;
			}

			// TODO construct in place?
			trades.emplace(trade_data);
		}
		
		if (DEBUG_PRINT_REQUESTS)
		{
			tmp_time = static_cast<time_t>(trades.front().trade.timestamp);
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "getTradeHistoryBefore() front timestamp: " << tmp_str;  //  << std::endl

			tmp_time = static_cast<time_t>(trades.back().trade.timestamp);
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "getTradeHistoryBefore() back timestamp: " << tmp_str;  // << std::endl
		}
	}

	return count;
}



size_t PolygonIoAdapter::getQuoteHistoryAfter(std::queue<QuoteData>& quotes, const std::string& symbol, const timestamp_t start_ts, const size_t limit)
{
	if (limit > MAX_LIMIT)
		throw std::logic_error("limit must be <= MAX_LIMIT");

	char tmp_str[26];
	time_t tmp_time;

	// download the latest quotes (in reverse order)
	if (DEBUG_PRINT_REQUESTS)
	{
		tmp_time = static_cast<time_t>(start_ts);
		ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
		std::cout << "getQuoteHistoryAfter() start_ts: " << tmp_str;  //  << std::endl
	}

	//last_timestamp = std::to_string(end_ts * static_cast<long long>(1e9));

	// Note: using the vX version of endpoint here
	const std::string url = POLYGON_BASE_URI + "vX/quotes/" + std::string(symbol);
	cpr::Response r = cpr::Get(
		cpr::Url{ url },  // cpr::Url{ url },  // see note above
		cpr::Parameters{
			{"limit", std::to_string(limit)},
			{"order", "asc"},
			{"timestamp.gte", std::to_string(start_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9
			{"apiKey", POLYGON_API_KEY}
		}
	);

	json res = json::parse(r.text);
	const size_t res_cnt = res["count"].is_null() || !res["results"].is_array() ? 0 : res["count"].get<size_t>();
	if (!res_cnt) {
		std::cout << symbol << " LATEST QUOTES ERROR:" << std::endl << "\t" << res << std::endl;
	}
	else {
		std::cout << symbol << " count=" << res_cnt << std::endl;

		if (DEBUG_PRINT_REQUESTS)
		{
			auto& first_entry = res["results"].front();
			auto first_timestamp = first_entry["sip_timestamp"].get<uint64_t>() / static_cast<uint64_t>(1e9);
			tmp_time = static_cast<time_t>(first_timestamp);
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "getQuoteHistoryAfter() first_timestamp: " << tmp_str;  //  << std::endl

			auto& last_entry = res["results"].back();
			tmp_time = static_cast<time_t>(last_entry["sip_timestamp"].get<uint64_t>() / static_cast<uint64_t>(1e9));
			ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			std::cout << "getQuoteHistoryAfter() last_timestamp: " << tmp_str;  // << std::endl
		}

		QuoteData quote_data;
		for (const json& quote : res["results"])
		{
			//// TODO catch instead of checking fields?
			//if (!quote.contains("ask_size") || !quote.contains("bid_size"))
			//	continue;

			try
			{
				payload_vx_to_quote_data(quote_data, quote);
			}
			catch (std::exception const& e)
			{
				std::cout << "getQuoteHistoryAfter() parse error: " << e.what() << std::endl;
				continue;
			}
			
			// TODO construct in place?
			quotes.emplace(quote_data);
		}
	}
	return res_cnt;
}
size_t PolygonIoAdapter::getQuoteHistoryBetween(std::queue<QuoteData>& quotes, const std::string& symbol, const timestamp_t start_ts, timestamp_t end_ts)
{
	const size_t limit = 50000;  // TODO static constant?

	// Note: using the vX version of endpoint here
	const std::string url = POLYGON_BASE_URI + "vX/quotes/" + std::string(symbol);

	// download quotes in range start_ts -> end_ts

	/*
	char tmp_str[26];
	time_t tmp_time;
	// download the latest quotes (in reverse order)
	tmp_time = static_cast<time_t>(start_ts);
	ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
	std::cout << "getQuoteHistoryBetween() start_ts: " << tmp_str;  //  << std::endl
	//last_timestamp = std::to_string(end_ts * static_cast<long long>(1e9));
	*/
	
	size_t count = 0;
	json next_url = JSON_NULL;
	cpr::Response r;
	do
	{
		if (!next_url.empty() && next_url.is_string())
		{
			r = cpr::Get(
				cpr::Url{ next_url.get<std::string>() + "&apiKey=" + POLYGON_API_KEY }
			);
		}
		else
		{
			r = cpr::Get(
				cpr::Url{ url },  // see note above
				cpr::Parameters{
					{"limit", std::to_string(limit)},
					{"order", "asc"},
					{"timestamp.gte", std::to_string(start_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9
					{"timestamp.lt", std::to_string(end_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9  // TODO lt or lte?
					{"apiKey", POLYGON_API_KEY}
				}
			);
		}
		json res = json::parse(r.text);
		const size_t res_cnt = res["count"].is_null() || !res["results"].is_array() ? 0 : res["count"].get<size_t>();

		if (!res_cnt) {
			std::cout << symbol << " LATEST QUOTES ERROR:" << std::endl << "\t" << res << std::endl;
			next_url = JSON_NULL;
		}
		else {
			// fetch next_url from response...
			next_url = res["next_url"];

			count += res_cnt;
			if (DEBUG_PRINT_REQUESTS)
				std::cout << symbol << " getQuoteHistoryBetween() count=" << res_cnt << std::endl;

			QuoteData quote_data;
			for (const json& quote : res["results"])
			{
				//// TODO catch instead of checking fields?
				//if (!quote.contains("ask_size") || !quote.contains("bid_size"))
				//	continue;
				
				try
				{
					payload_vx_to_quote_data(quote_data, quote);
				}
				catch (std::exception const& e)
				{
					std::cout << "getQuoteHistoryBetween() parse error: " << e.what() << std::endl;
					continue;
				}

				// TODO construct in place?
				quotes.emplace(quote_data);
			}
		}
	} while (!next_url.empty() && next_url.is_string());

	return count;
}

size_t PolygonIoAdapter::getTradeHistoryBetween(std::queue<TradeData>& trades, const std::string& symbol, timestamp_t start_ts, timestamp_t end_ts)
{
	const size_t limit = 50000;  // TODO static constant?

	const std::string url = POLYGON_BASE_URI + "vX/trades/" + std::string(symbol);
	
	// download trades in range start_ts -> end_ts

	/*
	// TODO
	char tmp_str[26];
	time_t tmp_time;
	tmp_time = static_cast<time_t>(start_ts);
	ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
	std::cout << "getTradeHistoryBetween() start_ts: " << tmp_str;  //  << std::endl

	tmp_time = static_cast<time_t>(end_ts);
	ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
	std::cout << "getTradeHistoryBetween() end_ts: " << tmp_str;  //  << std::endl
	//last_timestamp = std::to_string(end_ts * static_cast<long long>(1e9));
	*/
	
	size_t count = 0;
	json next_url = JSON_NULL;
	cpr::Response r;
	do
	{
		if (!next_url.empty() && next_url.is_string())
		{
			r = cpr::Get(
				cpr::Url{ next_url.get<std::string>() + "&apiKey=" + POLYGON_API_KEY }
			);
		}
		else
		{
			r = cpr::Get(
				cpr::Url{ url },
				cpr::Parameters{
					{"limit", std::to_string(limit)},
					{"order", "asc"},
					{"timestamp.gte", std::to_string(start_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9
					{"timestamp.lt", std::to_string(end_ts * static_cast<long long>(1e9))},  // nanosec, fe. sec*9  // TODO lt or lte?
					{"apiKey", POLYGON_API_KEY}
				}
			);
		}
		json res = json::parse(r.text);
		const size_t res_cnt = res["count"].is_null() || !res["results"].is_array() ? 0 : res["count"].get<size_t>();

		if (!res_cnt) {
			std::cout << symbol << " LATEST TRADES ERROR:" << std::endl << "\t" << res << std::endl;
			next_url = JSON_NULL;
		}
		else {
			// fetch next_url from response...
			next_url = res["next_url"];

			count += res_cnt;
			if (DEBUG_PRINT_REQUESTS)
				std::cout << symbol << " getTradeHistoryBetween() count=" << res_cnt << std::endl;

			TradeData trade_data;
			for (const json& trade : res["results"])
			{

				try
				{
					payload_vx_to_trade_data(trade_data, trade);
				}
				catch (std::exception const& e)
				{
					std::cout << "getTradeHistoryBetween() parse error: " << e.what() << std::endl;
					continue;
				}

				// TODO construct in place?
				trades.emplace(trade_data);
			}
		}
	}
	while (!next_url.empty() && next_url.is_string());

	return count;
}
