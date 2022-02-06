#ifndef DATA_CONTROLLER_H
#define DATA_CONTROLLER_H

#include <chrono>
#include <queue>

#include "core.h"
#include "ring_buffers.h"


namespace agpred {
	// fixed maximum number of symbols that may be tracked at any time:
	constexpr unsigned int MAX_ACTIVE_SYMBOLS = 3;
	
	using fn_update = std::function<void(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)>;
	using fn_snapshot = std::function<void(const Symbol& symbol, const Snapshot& snapshot)>;

	class DataController
	{
	public:
		DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update on_update);
		DataController(const AGMode mode);
		~DataController();

		// delete copy/move constructors and assignment operators
		DataController(const DataController&) = delete;
		DataController(const DataController&&) = delete;
		DataController& operator=(const DataController&) = delete;
		DataController& operator=(const DataController&&) = delete;

		void startSimulation(std::chrono::seconds start_ts);
		void startSimulation(std::chrono::seconds start_ts, std::chrono::minutes num_minutes);

		void initSymbol(const Symbol& symbol);
		void initSymbol(const Symbol& symbol, std::chrono::seconds ts);
		void destroySymbol(const Symbol& symbol);

		void onPayloads(const json& payloads);

	private:
		const AGMode mode_;
		const fn_snapshot on_snapshot_;
		const fn_update on_update_;

		// maintain a map of the active symbol positions in symbols_data
		std::map<Symbol, size_t, _compare_symbol> symbols_pos_; // TODO index by char*?
		std::map<size_t, Symbol> symbols_pos_rev_;

		// the remaining are tracked per-symbol, using a fixed array, indexed by the symbol position saved above...
		// (pre-allocate memory for all symbols)

		std::array<timestamp_t, MAX_ACTIVE_SYMBOLS> cur_timesteps_;
		
		// for each symbol, track enough bars for each interval, to role up into the next interval
		//   fe. track 5 1min bars, that-way we can role the 1min bars up into a 5min bar
		std::array<std::array<Bar, 5>, MAX_ACTIVE_SYMBOLS> bars_1min_;
		std::array<std::array<Bar, 3>, MAX_ACTIVE_SYMBOLS> bars_5min_;
		std::array<std::array<Bar, 4>, MAX_ACTIVE_SYMBOLS> bars_15min_;
		std::array<std::array<Bar, 4>, MAX_ACTIVE_SYMBOLS> bars_1hr_;
		// for roll-up to ~4hr, note: we never roll-up into daily/weekly bars

		// TODO may be more ideal to have these on the stack, not the heap o.0
		std::array<trades_queue, MAX_ACTIVE_SYMBOLS>* latest_trades_;
		std::array<quotes_queue, MAX_ACTIVE_SYMBOLS>* latest_quotes_;

		// track the raw candle data in xtensor arrays in ascending order
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_5min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_15min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1hr_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_4hr_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1d_ = nullptr;  // loaded in initSymbol/not kept up-to-date
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1w_ = nullptr;  // loaded in initSymbol/not kept up-to-date

		const std::array<const Snapshot, MAX_ACTIVE_SYMBOLS> snapshots_;

		// for each symbol, track processed data for each interval

		//std::array<xt::xtensor_fixed<double, shape_processed_t>, MAX_ACTIVE_SYMBOLS> processed_data_;

		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1min_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_5min_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_15min_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1hr_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_4hr_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1d_ = nullptr;  // loaded in initSymbol/not kept up-to-date
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1w_ = nullptr;  // loaded in initSymbol/not kept up-to-date


		void onTradePayload(const json& trade);
		void onNbboPayload(const json& quote);

		// TODO use a data adapter for all of these:

		void process_trade(const size_t& pos, const json& trade);
		void process_trade_rt(const size_t& pos, const json& trade);
		void process_trade_data(const size_t& pos, const TradeData& trade);
		// TODO replace `const json& cond` with TradeCondition[] ?
		void process_trade_finish(const size_t& pos, const json& cond, const double& ts, const double& price, const uint32_t vol);

		void process_quote(const size_t& pos, const json& quote);
		void process_quote_rt(const size_t& pos, const json& quote);
		void process_quote_data(const size_t& pos, const QuoteData& quote);
		void process_quote_finish(const size_t& pos, const double& ts, const double& askPrice, const uint32_t& askSize, const double& bidPrice, const uint32_t& bidSize);

		void flush(const timestamp_t& ts_step);
		void flush(const timestamp_t& next_ts, const size_t& pos);

		inline void zero_pos(const size_t& pos)
		{
			//processed_data_[pos] = xt::zeros<double>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
			//symbols_data_[pos] = xt::zeros<double>({ NUM_INTERVALS, 255, 6 });
			cur_timesteps_[pos] = 0;

			bars_1min_[pos][0].zero();

			// TODO zero bars_*
		}
	};
}

#endif // DATA_CONTROLLER_H
