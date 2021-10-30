#include "polygon_io.h"

#include <time.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <xtensor/xview.hpp>
#include <xtensor/xdynamic_view.hpp>
#include <xtensor/xindex_view.hpp>
#include <xtensor/xio.hpp>


using namespace agpred;


void process_agg(Bar& dest, const json& agg)
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

void process_agg2(xtensor_raw_interval& dest, const uint32_t i, const json& agg)
{
	dest(i, 0) = static_cast<double>(agg["t"].get<uint64_t>()) / 1.0e3;
	dest(i, 1) = agg["o"].get<double>();
	dest(i, 2) = agg["h"].get<double>();
	dest(i, 3) = agg["l"].get<double>();
	dest(i, 4) = agg["c"].get<double>();
	dest(i, 5) = agg["v"].get<uint32_t>();

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

size_t PolygonIoAdapter::getAggregateHistory(xtensor_raw_interval& dest, const std::string& symbol, unsigned int interval, timestamp_t start_ts, timestamp_t end_ts, bool adjusted)
{
	return getAggregateHistory(dest, symbol, interval, start_ts, end_ts, adjusted, 50000);
}



size_t PolygonIoAdapter::getQuoteHistory(xtensor_raw& dest, const std::string& symbol, timestamp_t end_ts, unsigned int limit)
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
	do
	{
		cpr::Response r = cpr::Get(
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
			//tmp_time = static_cast<time_t>(first_timestamp);
			//ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			//std::cout << "getQuoteHistory() first_timestamp: " << tmp_str;  //  << std::endl

			auto& last_entry = res["results"].back();
			last_timestamp = std::to_string(last_entry["t"].get<uint64_t>());  // nanosecond
			//tmp_time = static_cast<time_t>(last_entry["t"].get<uint64_t>() / static_cast<uint64_t>(1e9));
			//ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
			//std::cout << "getQuoteHistory() last_timestamp: " << tmp_str;  // << std::endl
			////std::cout << "getQuoteHistory() last_timestamp: " << last_timestamp << std::endl;
			
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
						//std::cout << "getQuoteHistory() [" << interval << "] !i_where.size()" << std::endl;
						continue;
					}
					auto& ii_where = i_where[0];
					if (ii_where.empty())
					{
						//std::cout << "getQuoteHistory() [" << interval << "] !ii_where.size()" << std::endl;
						continue;
					}
					size_t next_pos = ii_where[ii_where.size() - 1];
					//std::cout << "getQuoteHistory() [" << interval << "] next_pos=" << next_pos << std::endl;

					if (next_pos == NUM_TIMESTEMPS - 1)
					{
						std::cout << "getQuoteHistory() [" << interval << "] quotes more recent than oldest timestep" << std::endl;

						// TODO include times in message?
						auto next_pos_ts = i_timestamps(next_pos);
						tmp_time = static_cast<time_t>(next_pos_ts);
						ctime_s(tmp_str, sizeof tmp_str, &tmp_time);
						std::cout << "getQuoteHistory() [" << interval << "] next_pos TS: " << tmp_str;  // << std::endl

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
				//std::cout << "getQuoteHistory() [" << interval << "] in  dest_ts: " << tmp_str;  // << std::endl

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
						//std::cout << "getQuoteHistory() [" << interval << "] BREAK (dest_row==0)" << std::endl;
						break;
					}


					const timestamp_t next_ts = dest_ts + interval_seconds;
					if (cur_ts >= next_ts)
					{
						if (dest_row < 1)
						{
							//std::cout << "getQuoteHistory() [" << interval << "] BREAK (dest_row==0)" << std::endl;
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
				//std::cout << "getQuoteHistory() [" << interval << "] out dest_ts: " << tmp_str;  // << std::endl
			}
		}
	}
	while (++req_cnt < MAX_HISTORY_QUOTE_REQUESTS && res_cnt >= limit);
	
	return count;
}

size_t PolygonIoAdapter::getQuoteHistory(xtensor_raw& dest, const std::string& symbol, timestamp_t end_ts)
{
	return getQuoteHistory(dest, symbol, end_ts, 50000);
}
