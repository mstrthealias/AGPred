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

	using id_t = unsigned int;

	constexpr int RT_MAX_TIMESTEPS = 255;

	// this is how much raw data is tracked for each symbol; note that not all timestamps fill 255 rows
	using shape_raw_interval_255_t = xt::xshape<RT_MAX_TIMESTEPS, 14>;  // (timesteps, cols)
	using shape_raw_255_t = xt::xshape<NUM_INTERVALS, RT_MAX_TIMESTEPS, 14>;  // (timeframe, timesteps, cols)

	using xtensor_raw_interval = xt::xtensor_fixed<double, shape_raw_interval_255_t>;
	using xtensor_raw = xt::xtensor_fixed<double, shape_raw_255_t>;

	// TODO processed
	// this is how much processed data is tracked for each symbol; note that not all timestamps fill 255 rows
	// shape: (timesteps, columns/features, timeframes)
	using shape_processed_255_t = xt::xshape<RT_MAX_TIMESTEPS, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS>;
	

	enum class Market {
		NYSE,
		AMEX,
		NASDAQ,
	};

	enum class OrderType : uint8_t {
		BUY = 1,
		SELL = 2,
	};

	enum class PositionType : uint8_t {
		LONG = 1,
		SHORT = 2,
	};
	
	struct Symbol {
		const std::string symbol;
		const Market market = Market::NASDAQ;
		
		bool operator== (const Symbol& b) const
		{
			return symbol == b.symbol;
		}

		// TODO
		static const Symbol& get_symbol(const std::string& ticker)
		{
			if (symbol_cache.find(ticker) == symbol_cache.end())
			{
				// TODO statically implement list of all Symbol to consider ?
				// TODO use REST to download market/symbol data?
				symbol_cache.emplace(ticker, Symbol{ ticker.c_str(), Market::NASDAQ });
			}
			return symbol_cache[ticker];
		}
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
		const Symbol symbol;

	};

	struct ExitData {
		const Symbol symbol;

	};

	struct Snapshot {
		NBBO nbbo;  // TODO NBBO or just pass latest 1min Bar?

		const double& price;  // last price

		//Bar partial1min;  // TODO

		// the last *complete* bars, by interval  // TODO map index by interval?!
		const Bar& last1min;
		const Bar& last5min;
		const Bar& last15min;
		const Bar& hourly;
		//Bar daily;  // TODO ? prev_daily?
		//Bar weekly;  // TODO ?
	};

}

#endif // AGPRED_CORE_H
