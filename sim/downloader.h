#ifndef DOWNLOADER_H
#define DOWNLOADER_H


#include "../src/common.h"
#include "../core/core.h"
#include "../core/ring_buffers.h"


namespace agpred {


	void onUpdateDl(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades);


	class Downloader final {
	public:
		Downloader() = default;

		void onSnapshot(const Symbol& symbol, const Snapshot& snapshot)
		{
			//SymbolSimMarket& market = symbols_[symbol.symbol];
			
		}

		void onUpdate(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
		{
			onUpdateDl(symbol, snapshot, data, data_processed, quotes, trades);


		}

	};


}


#endif // DOWNLOADER_H
