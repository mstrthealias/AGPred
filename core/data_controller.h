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
	using fn_update_outputs = std::function<void(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades, const xtensor_outputs_interval& outputs)>;
	using fn_snapshot = std::function<void(const Symbol& symbol, const Snapshot& snapshot)>;


	struct ShiftTriggers
	{
		const timestamp_us_t next_ts;
		const double ts_dbl;
		const bool flush10sec;
		const bool flush1min;
		const bool flush5min;
		const bool flush15min;
		const bool flush1hr;
		const bool flush4hr;

		explicit ShiftTriggers(const timestamp_us_t& next_ts)
			: next_ts(next_ts),
			ts_dbl(static_cast<double>(next_ts) / static_cast<double>(SEC_TO_US)),
			flush10sec(static_cast<timestamp_s_t>(ts_dbl / 10.0) * 10 == next_ts / SEC_TO_US),
			flush1min(static_cast<timestamp_s_t>(ts_dbl / 60.0) * 60 == next_ts / SEC_TO_US),
			flush5min(static_cast<timestamp_s_t>(ts_dbl / 300.0) * 300 == next_ts / SEC_TO_US),
			flush15min(static_cast<timestamp_s_t>(ts_dbl / 900.0) * 900 == next_ts / SEC_TO_US),
			flush1hr(static_cast<timestamp_s_t>(ts_dbl / 3600.0) * 3600 == next_ts / SEC_TO_US),
			flush4hr(static_cast<timestamp_s_t>(ts_dbl / 14400.0) * 14400 == next_ts / SEC_TO_US)
		{
		}
	};


	class DataController
	{
	public:
		DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update on_update, const fn_update_outputs on_update_outputs);
		DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update on_update);
		DataController(const AGMode mode, const fn_snapshot on_snapshot, const fn_update_outputs on_update_outputs);
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
		const fn_update_outputs on_update_outputs_;

		// maintain a map of the active symbol positions in symbols_data
		std::map<Symbol, size_t, _compare_symbol> symbols_pos_; // TODO index by char*?
		std::map<size_t, Symbol> symbols_pos_rev_;

		// the remaining are tracked per-symbol, using a fixed array, indexed by the symbol position saved above...
		// (pre-allocate memory for up to MAX_ACTIVE_SYMBOLS symbols)

		std::array<timestamp_us_t, MAX_ACTIVE_SYMBOLS> cur_timesteps_;
		
		// TODO may be more ideal to have these on the stack, not the heap o.0
		std::array<trades_queue, MAX_ACTIVE_SYMBOLS>* latest_trades_ = nullptr;
		std::array<quotes_queue, MAX_ACTIVE_SYMBOLS>* latest_quotes_ = nullptr;
		
		// track outputs here, incase in download-data mode
		std::array<xtensor_outputs_interval, MAX_ACTIVE_SYMBOLS>* symbols_outputs_ = nullptr;

		// track the raw candle data timestamps separately for different datatype
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_1min_ = nullptr;
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_5min_ = nullptr;
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_15min_ = nullptr;
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_1hr_ = nullptr;
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_4hr_ = nullptr;
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_1d_ = nullptr;  // loaded in initSymbol/not kept up-to-date
		std::array<xtensor_ts_interval, MAX_ACTIVE_SYMBOLS>* symbols_ts_1w_ = nullptr;  // loaded in initSymbol/not kept up-to-date

		// track the raw candle data in xtensor arrays in ascending order
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_5min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_15min_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1hr_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_4hr_ = nullptr;
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1d_ = nullptr;  // loaded in initSymbol/not kept up-to-date
		std::array<xtensor_raw_interval, MAX_ACTIVE_SYMBOLS>* symbols_1w_ = nullptr;  // loaded in initSymbol/not kept up-to-date

		// for each symbol, track processed data for each interval
		//std::array<xt::xtensor_fixed<real_t, shape_processed_t>, MAX_ACTIVE_SYMBOLS> processed_data_;  // TODO save combined xtensor, and use views into it??
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1min_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_5min_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_15min_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1hr_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_4hr_ = nullptr;
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1d_ = nullptr;  // loaded in initSymbol/not kept up-to-date
		std::array<xtensor_processed_interval, MAX_ACTIVE_SYMBOLS>* proc_symbols_1w_ = nullptr;  // loaded in initSymbol/not kept up-to-date

		// for each symbol, track bars that reference to values of the latest timestamp
		std::array<BarFullRef, MAX_ACTIVE_SYMBOLS> latest_1min_;
		std::array<BarRef, MAX_ACTIVE_SYMBOLS> latest_5min_;
		std::array<BarRef, MAX_ACTIVE_SYMBOLS> latest_15min_;
		std::array<BarRef, MAX_ACTIVE_SYMBOLS> latest_1hr_;
		std::array<BarRef, MAX_ACTIVE_SYMBOLS> latest_4hr_;
		std::array<BarRef, MAX_ACTIVE_SYMBOLS> latest_1d_;
		std::array<BarRef, MAX_ACTIVE_SYMBOLS> latest_1w_;

		std::array<Snapshot, MAX_ACTIVE_SYMBOLS> snapshots_;


		void onTradePayload(const json& trade);
		void onNbboPayload(const json& quote);

		// TODO use a data adapter for all of these:

		void process_trade(const size_t& pos, const json& trade);
		void process_trade_rt(const size_t& pos, const json& trade);
		void process_trade_data(const size_t& pos, const TradeData& trade);
		// TODO replace `const json& cond` with TradeCondition[] ?
		void process_trade_finish(const size_t& pos, const json& cond, const timestamp_us_t& ts, const real_t& price, const uint32_t vol);

		void process_quote(const size_t& pos, const json& quote);
		void process_quote_rt(const size_t& pos, const json& quote);
		void process_quote_data(const size_t& pos, const QuoteData& quote);
		void process_quote_finish(const size_t& pos, const timestamp_us_t& ts, const real_t& askPrice, const uint32_t& askSize, const real_t& bidPrice, const uint32_t& bidSize);
		
		void shift(const ShiftTriggers& triggers, const size_t& pos);
		void do_update(const ShiftTriggers& triggers, const size_t& pos);

		inline void zero_pos(const size_t& pos)
		{
			//processed_data_[pos] = xt::zeros<real_t>({ 255, NUM_TIMESTEMPS, NUM_COLUMNS, NUM_INTERVALS });
			//symbols_data_[pos] = xt::zeros<real_t>({ NUM_INTERVALS, 255, 6 });
			cur_timesteps_[pos] = 0;

			// TODO
			//bars_1min_[pos][0].zero();

			// TODO zero bars_*
		}
	};
}

#endif // DATA_CONTROLLER_H
