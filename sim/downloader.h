#ifndef DOWNLOADER_H
#define DOWNLOADER_H


#include "../src/common.h"
#include "../core/core.h"
#include "../core/ring_buffers.h"


namespace agpred {


	void onUpdateDl(const Symbol& symbol, const Snapshot& snapshot, const xtensor_ts_interval& data_ts, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades, const xtensor_outputs_interval& outputs);


	class Downloader final {
	public:
		Downloader(const std::string& file_prefix) 
			: file_prefix_(file_prefix)
		{
		}

		void onSnapshot(const Symbol& symbol, const Snapshot& snapshot)
		{
			//SymbolSimMarket& market = symbols_[symbol.symbol];
			
		}

		void onUpdate(const Symbol& symbol, const Snapshot& snapshot, const xtensor_ts_interval& data_ts, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades, const xtensor_outputs_interval& outputs);
		void onSimComplete(const Symbol& symbol);

	private:

		void do_flush(const Symbol& symbol);


		std::string file_prefix_;

		size_t cur_pos_ = 0;
		size_t cur_stage_ = 0;
	};


}


#endif // DOWNLOADER_H
