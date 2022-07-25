#ifndef AGPRED_CORE_H
#define AGPRED_CORE_H

#include <cstdint>
#include <cstring>

#include <nlohmann/json.hpp>
#include <xtensor/xfixed.hpp>

#include "../src/defs.h"
#include "../src/common.h"


namespace agpred {
	using json = nlohmann::json;

	using id_t = unsigned int;  // TODO size_t?
	
	constexpr int NUM_RAW_COLUMNS = 17;
	constexpr int NUM_TRADE_COLUMNS = 6;  // ts, price, size, cond1, cond2, cond3
	constexpr int NUM_QUOTE_COLUMNS = 8;  // ts, bid, ask, bid_size, ask_size, cond1, cond2, cond3
	constexpr int RT_MAX_TIMESTEPS = 255;
	constexpr int RT_REPORT_TIMESTEPS = NUM_TIMESTEMPS;  // 11;  // TODO NUM_TIMESTEMPS ?

	// this is how much raw data is tracked for each symbol; note that not all timestamps fill 255 rows
	using shape_ts_interval_255_t = xt::xshape<RT_MAX_TIMESTEPS, 1>;  // (timesteps, 1)
	using shape_ts_255_t = xt::xshape<NUM_INTERVALS, RT_MAX_TIMESTEPS, 1>;  // (timeframe, timesteps, 1)
	using shape_raw_interval_255_t = xt::xshape<RT_MAX_TIMESTEPS, NUM_RAW_COLUMNS>;  // (timesteps, cols)
	using shape_raw_255_t = xt::xshape<NUM_INTERVALS, RT_MAX_TIMESTEPS, NUM_RAW_COLUMNS>;  // (timeframe, timesteps, cols)
	using shape_raw_t = xt::xshape<NUM_INTERVALS, RT_REPORT_TIMESTEPS, NUM_RAW_COLUMNS>;  // (timeframe, timesteps, cols)
	using shape_trades_t = xt::xshape<NUM_TRADES, NUM_TRADE_COLUMNS>;  // (timesteps, cols)
	using shape_quotes_t = xt::xshape<NUM_QUOTES, NUM_QUOTE_COLUMNS>;  // (timesteps, cols)

	using xtensor_ts_interval = xt::xtensor_fixed<timestamp_us_t , shape_ts_interval_255_t>;
	using xtensor_ts_255 = xt::xtensor_fixed<timestamp_us_t, shape_ts_255_t>;
	using xtensor_raw_interval = xt::xtensor_fixed<real_t, shape_raw_interval_255_t>;
	using xtensor_raw_255 = xt::xtensor_fixed<real_t, shape_raw_255_t>;
	using xtensor_raw = xt::xtensor_fixed<real_t, shape_raw_t>;
	using xtensor_trades = xt::xtensor_fixed<real_t, shape_trades_t>;
	using xtensor_quotes = xt::xtensor_fixed<real_t, shape_quotes_t>;

	// TODO processed
	// this is how much processed data is tracked for each symbol
	// shape: (timesteps, columns/features, timeframes)
	using shape_processed_interval_t = xt::xshape<RT_MAX_TIMESTEPS, NUM_COLUMNS>;
	using shape_processed_t = xt::xshape<NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS>;  // TODO move NUM_INTERVALS to end?
	using shape_outputs_interval_t = xt::xshape<RT_MAX_TIMESTEPS, ColPos::_OUTPUT_NUM_COLS>;  // (timesteps) for single column
	
	using xtensor_processed_interval = xt::xtensor_fixed<real_t, shape_processed_interval_t>;
	using xtensor_processed = xt::xtensor_fixed<real_t, shape_processed_t>;
	using xtensor_outputs_interval = xt::xtensor_fixed<double, shape_outputs_interval_t>;

	enum class AGMode
	{
		BACK_TEST,
		LIVE_TEST,
		LIVE_TRADE,
		DOWNLOAD_ONLY,
	};

	enum class Market
	{
		NYSE,
		AMEX,
		NASDAQ,
	};

	enum class OrderType : uint8_t
	{
		BUY = 1,
		SELL = 2,
	};

	enum class OrderStatus : uint8_t
	{
		FILLED = 1,
		PARTIAL = 2,
		REJECT = 3,  // TODO ERROR and REJECT?
		ERR = 4,  // TODO ERROR and REJECT?
	};

	enum class PositionType : uint8_t
	{
		LONG = 1,
		SHORT = 2,
	};


	struct AccountStatus {
		const real_t account_balance;
		const real_t max_trade_loss;
		const real_t max_daily_loss;
	};


	struct AccountStatusRequest {
		const id_t request_id = 0;
		json data;

		static AccountStatusRequest nextRequest()
		{
			return {
					next_request_id++,
					{}
			};
		}

	private:
		AccountStatusRequest(const id_t request_id, const json& data) 
			: request_id(request_id), data(data)
		{
		}

		/*inline*/ static id_t next_request_id/* = 1*/;

	};


	struct Symbol {
		const std::string symbol;
		const Market market = Market::NASDAQ;  // TODO
		
		bool operator== (const Symbol& b) const;
		bool operator!= (const Symbol& b) const;

		// TODO
		static const Symbol& get_symbol(const std::string& ticker);
	private:
		/*inline*/ static std::map<std::string, Symbol> symbol_cache;
	};

	struct _compare_symbol {
		bool operator() (const Symbol& lhs, const Symbol& rhs) const
		{
			return lhs.symbol < rhs.symbol;
		}
	};


	struct EntryData {
		//const Symbol& symbol;

		const PositionType type;

		const size_t size;
		const real_t limit_price;
		const real_t stoploss;  // if non-zero, the system will manage the exit at this stop-loss price

		/*
            'target': self.target,
            'loss': self.loss,
            'min_time': self.min_time,
            'max_time': self.max_time,
            'size': self.size,
            'limit_price': self.limit_price,
            */
	};
		
	struct ExitData {
		//const Symbol& symbol;

		const PositionType type;

		const real_t limit_price;
	};


	struct PendingOrder {
		const id_t order_id = 0;
		json data;

	protected:
		/*inline*/ static id_t next_order_id/* = 1*/;
	};

	struct PendingEntry : PendingOrder {
		const EntryData entry_data = { PositionType::LONG, 0, 0.0 };

		PendingEntry(const PendingOrder& pending_order, const EntryData& entry_data)
			: PendingOrder(pending_order), entry_data(entry_data)
		{
		}

		static PendingEntry fromEntryData(const EntryData& entry_data)
		{
			return {
				{
					next_order_id++,
					{}
				},
				entry_data
			};
		}
	};

	struct PendingExit : PendingOrder {
		const id_t position_id = 0;
		const ExitData exit_data = {PositionType::LONG, 0.0};

		PendingExit(const PendingOrder &pending_order, const id_t& position_id, const ExitData& exit_data)
			: PendingOrder(pending_order), position_id(position_id), exit_data(exit_data)
		{
		}

		static PendingExit fromExitData(const id_t& position_id, const ExitData& exit_data)
		{
			return {
				{
					next_order_id++,
					{}
				},
				position_id,
				exit_data
			};
		}
	};


	struct Snapshot {
		NBBO nbbo;  // TODO NBBO or just pass latest 1min Bar?

		const real_t& price;  // last price

		//Bar partial1min;  // TODO

		// the last *complete* bars, by interval  // TODO map index by interval?!
		const BarFullRef& last1min;
		const BarFullRef& last5min;
		const BarFullRef& last15min;
		const BarRef& hourly;
		//Bar daily;  // TODO ? prev_daily?
		//Bar weekly;  // TODO ?
	};


	// TODO
	struct QuoteData {
		//const Symbol symbol;  // TODO symbol for DataAdapter?

		/*const */Quote quote = { 0, 0.0, 0.0, 0, 0 };

		// TODO array?
		/*const */QuoteCondition cond[3] = { QuoteCondition::Invalid, QuoteCondition::Invalid, QuoteCondition::Invalid };
	};

	struct TradeData {
		//const Symbol symbol;  // TODO symbol for DataAdapter?

		/*const */Trade trade = {0, 0.0, 0};

		/*const */TradeCondition cond[3] = {TradeCondition::PLACEHOLDER, TradeCondition::PLACEHOLDER, TradeCondition::PLACEHOLDER };
	};

	



	//std::ostream& operator<< (std::ostream& out, const QuoteData& quote);
	//std::ostream& operator<< (std::ostream& out, const TradeData& trade);

}

#endif // AGPRED_CORE_H
