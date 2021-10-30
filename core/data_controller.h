#ifndef DATA_CONTROLLER_H
#define DATA_CONTROLLER_H

#include <chrono>

#include "core.h"


namespace agpred {
	// fixed maximum number of symbols that may be tracked at any time:
	constexpr unsigned int MAX_ACTIVE_SYMBOLS = 3;

	using fn_update = std::function<void(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data)>;
	using fn_snapshot = std::function<void(const Symbol& symbol, const Snapshot& snapshot)>;

	class DataController {
	public:
		DataController(const fn_snapshot on_snapshot, const fn_update on_update);
		DataController();
		~DataController();

		// delete copy/move constructors and assignment operators
		DataController(const DataController&) = delete;
		DataController(const DataController&&) = delete;
		DataController& operator= (const DataController&) = delete;
		DataController& operator= (const DataController&&) = delete;

		void initSymbol(const Symbol& symbol);
		void initSymbol(const Symbol& symbol, std::chrono::seconds ts);
		void destroySymbol(const Symbol& symbol);

		void onPayloads(const char* json_str);

	private:
		const fn_snapshot on_snapshot_;
		const fn_update on_update_;

		// maintain a map of the active symbol positions in symbols_data
		std::map<Symbol, size_t, _compare_symbol> symbols_pos_;  // TODO index by char*?
		std::map<size_t, Symbol> symbols_pos_rev_;

		// the remaining are tracked per-symbol, using a fixed array, indexed by the symbol position saved above...
		// (pre-allocate memory for all symbols)

		std::array<timestamp_t, MAX_ACTIVE_SYMBOLS> cur_timesteps_;

		//// each symbol aggregates into this Bar, before flushing into a 1min/10s bar
		//std::array<Bar, MAX_ACTIVE_SYMBOLS> cur_bar_;

		// for each symbol, track enough bars for each interval, to role up into the next interval
		//   fe. track 5 1min bars, that-way we can role the 1min bars up into a 5min bar
		std::array<std::array<Bar, 5>, MAX_ACTIVE_SYMBOLS> bars_1min_;
		std::array<std::array<Bar, 3>, MAX_ACTIVE_SYMBOLS> bars_5min_;
		std::array<std::array<Bar, 4>, MAX_ACTIVE_SYMBOLS> bars_15min_;
		std::array<std::array<Bar, 4>, MAX_ACTIVE_SYMBOLS> bars_1hr_;  // for roll-up to ~4hr, note: we never roll-up into daily/weekly bars

		//std::array<xt::xtensor_fixed<double, shape_processed_255_t>, MAX_ACTIVE_SYMBOLS> processed_data_;

		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_5min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_15min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1hr_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_4hr_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1d_ = nullptr;  // loaded in initSymbol/not kept up-to-date
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1w_ = nullptr;  // loaded in initSymbol/not kept up-to-date

		const std::array<const Snapshot, MAX_ACTIVE_SYMBOLS> snapshots_;

		void onTradePayload(const json& trade);
		void onNbboPayload(const json& quote);

		void process_trade_rt(const size_t& pos, const json& trade);
		void process_quote_rt(const size_t& pos, const json& quote);

		void flush(const timestamp_t& ts_step);
		void flush(const size_t& pos, const timestamp_t& ts_step);

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
