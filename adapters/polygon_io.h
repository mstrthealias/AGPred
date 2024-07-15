#ifndef POLYGON_IO_H
#define POLYGON_IO_H

#include <string>
#include <queue>

#include "../src/common.h"
#include "../core/data_adapter.h"
#include "../core/core.h"


// #define USE_PROXY_CACHE


namespace agpred {

	static const std::string POLYGON_API_KEY = "YOUR_POLYGON_API_KEY";

	static const std::string POLYGON_BASE_URI = "https://api.polygon.io/";

	constexpr unsigned int MAX_HISTORY_QUOTE_REQUESTS = 45;  // TODO remove

	constexpr unsigned int MAX_LIMIT = 50000;

	namespace adapter
	{
		// TODO adapter class instead of namespace?

		// TODO inline these?

		void payload_rt_to_quote_data(QuoteData& quote_data, const json& quote);

		void payload_rt_to_trade_data(TradeData& trade_data, const json& trade);

	}


	class PolygonIoAdapter : DataAdapter {

	public:
		/**
		 * getAggregateHistory:
		 */
		static size_t getAggregateHistory(xtensor_ts_interval& dest_ts, xtensor_raw_interval& dest, const std::string& symbol, unsigned int interval, timestamp_us_t start_ts, timestamp_us_t end_ts, bool adjusted, unsigned int limit = MAX_LIMIT);


		/**
		 * mergeQuotesAggregates:
		 *   1) Fetches up to 50000*MAX_HISTORY_QUOTE_REQUESTS of quotes history for today, used to aggregate bid(_high,_low)/ask(_high,_low) into 1min data {dest} xtensor array
		 */
		static size_t mergeQuotesAggregates(xtensor_raw_255& dest, const std::string& symbol, const xtensor_ts_255& dest_ts, timestamp_us_t end_ts, unsigned int limit = MAX_LIMIT);


		/**
		 * getQuoteHistoryBefore:
		 *	 1) Fetches NUM_QUOTES of quotes history into {quotes} array
		 */
		static size_t getQuoteHistoryBefore(std::queue<QuoteData>& quotes, const std::string& symbol, timestamp_us_t end_ts, const size_t limit);

		/**
		 * getQuoteHistoryAfter:
		 */
		static size_t getQuoteHistoryAfter(std::queue<QuoteData>& quotes, const std::string& symbol, const timestamp_us_t start_ts, const size_t limit = MAX_LIMIT);

		/**
		 * getQuoteHistoryBetween:
		 */
		static size_t getQuoteHistoryBetween(std::queue<QuoteData>& quotes, const std::string& symbol, const timestamp_us_t start_ts, timestamp_us_t end_ts);


		/**
		 * getTradeHistoryBefore:
		 *	 1) Fetches NUM_TRADES of trades history into {trades} array
		 */
		static size_t getTradeHistoryBefore(std::queue<TradeData>& trades, const std::string& symbol, timestamp_us_t end_ts, const size_t limit);

		/**
		 * getTradeHistoryBetween:
		 */
		static size_t getTradeHistoryBetween(std::queue<TradeData>& trades, const std::string& symbol, timestamp_us_t start_ts, timestamp_us_t end_ts);
	};
}

#endif // POLYGON_IO_H
