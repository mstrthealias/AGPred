#ifndef POLYGON_IO_H
#define POLYGON_IO_H

#include <string>

#include "../src/common.h"
#include "../core/adapter.h"
#include "../core/core.h"


namespace agpred {

	static const std::string POLYGON_API_KEY = "bEB2e26btlSKHLjRCw_k_HpH_0pglFyX";

	static const std::string POLYGON_BASE_URI = "https://api.polygon.io/";

	constexpr unsigned int MAX_HISTORY_QUOTE_REQUESTS = 25;


	class PolygonIoAdapter : Adapter {

	public:
		static size_t getAggregateHistory(xtensor_raw_interval& dest, const std::string& symbol, unsigned int interval, timestamp_t start_ts, timestamp_t end_ts, bool adjusted, unsigned int limit);
		static size_t getAggregateHistory(xtensor_raw_interval& dest, const std::string& symbol, unsigned int interval, timestamp_t start_ts, timestamp_t end_ts, bool adjusted);

		static size_t getQuoteHistory(xtensor_raw& dest, const std::string& symbol, timestamp_t end_ts);
		static size_t getQuoteHistory(xtensor_raw& dest, const std::string& symbol, timestamp_t end_ts, unsigned int limit);
	};
}

#endif // POLYGON_IO_H
