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

	constexpr int NUM_RAW_COLUMNS = 14;
	constexpr int RT_MAX_TIMESTEPS = 255;
	constexpr int RT_REPORT_TIMESTEPS = 11;  // TODO NUM_TIMESTEMPS ?

	// this is how much raw data is tracked for each symbol; note that not all timestamps fill 255 rows
	using shape_raw_interval_255_t = xt::xshape<RT_MAX_TIMESTEPS, NUM_RAW_COLUMNS>;  // (timesteps, cols)
	using shape_raw_255_t = xt::xshape<NUM_INTERVALS, RT_MAX_TIMESTEPS, NUM_RAW_COLUMNS>;  // (timeframe, timesteps, cols)
	using shape_raw_t = xt::xshape<NUM_INTERVALS, RT_REPORT_TIMESTEPS, NUM_RAW_COLUMNS>;  // (timeframe, timesteps, cols)

	using xtensor_raw_interval = xt::xtensor_fixed<double, shape_raw_interval_255_t>;
	using xtensor_raw_255 = xt::xtensor_fixed<double, shape_raw_255_t>;
	using xtensor_raw = xt::xtensor_fixed<double, shape_raw_t>;

	// TODO processed
	// this is how much processed data is tracked for each symbol
	// shape: (timesteps, columns/features, timeframes)
	using shape_processed_interval_t = xt::xshape<RT_MAX_TIMESTEPS, NUM_COLUMNS>;
	using shape_processed_t = xt::xshape<NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS>;  // TODO move NUM_INTERVALS to end?
	
	using xtensor_processed_interval = xt::xtensor_fixed<double, shape_processed_interval_t>;
	using xtensor_processed = xt::xtensor_fixed<double, shape_processed_t>;

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


	struct Symbol {
		const std::string symbol;
		const Market market = Market::NASDAQ;  // TODO
		
		bool operator== (const Symbol& b) const;

		// TODO
		static const Symbol& get_symbol(const std::string& ticker);
	private:
		inline static std::map<std::string, Symbol> symbol_cache;
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
		const double limit_price;

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

		const double limit_price;
	};


	struct PendingOrder {
		const id_t order_id = 0;
		json data;

	protected:
		inline static id_t next_order_id = 1;
	};

	struct PendingEntry : PendingOrder {
		const EntryData entry_data = { PositionType::LONG, 0, 0.0 };

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

		const double& price;  // last price

		//Bar partial1min;  // TODO

		// the last *complete* bars, by interval  // TODO map index by interval?!
		const BarFullRef& last1min;
		const BarRef& last5min;
		const BarRef& last15min;
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
