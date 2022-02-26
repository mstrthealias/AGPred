#ifndef LONG_LOW_H
#define LONG_LOW_H


#include <tensorflow/cc/saved_model/loader.h>

#include "../core/strategy.h"


namespace agpred {



	class LongLowAlgo final : public AlgoBase
	{
	public:
		LongLowAlgo(const std::string& name, bool inverse);
		~LongLowAlgo() override = default;

		bool operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const override
		{
			//std::cout << "LongLowAlgo: CALL()" << std::endl;

			if (inverse_)
				return calc_prediction_signal(raw, processed, quotes, trades) < 0;
			else
				return calc_prediction_signal(raw, processed, quotes, trades) > 0;
		}
	private:

		int calc_prediction_signal(const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const;


		tensorflow::SavedModelBundle bundle_;  // TODO SavedModelBundleLite?

		const tensorflow::SignatureDef sig_def_;
		const tensorflow::TensorInfo inputs_features_;
		const tensorflow::TensorInfo inputs_candles_;
		const tensorflow::TensorInfo inputs_trades_;
		const tensorflow::TensorInfo inputs_quotes_;
		const tensorflow::TensorInfo outputs_long_low_;

		const bool inverse_;

	};


	class LongLowEntry final : public EntryBase {
	public:
		LongLowEntry(const std::string& name, const unsigned frequency, const AlgoBase& algo)
			: EntryBase(name, frequency, algo)
		{
		}
		~LongLowEntry() override = default;

		EntryData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "LongLowEntry: EntryData CALL()" << std::endl;

			// TODO an actual entry ?
			return EntryData{ PositionType::LONG, 100, snapshot.last1min.close, snapshot.last1min.close - 0.71 };  //symbol, 
		}
	};


	class LongLowExit : public AlgoExitBase {
	public:
		LongLowExit(const std::string& name, const AlgoBase& algo)
			: AlgoExitBase(name, algo)
		{
		}
		~LongLowExit() override = default;

		ExitData operator()(const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "LongLowExit: ExitData CALL()" << std::endl;

			// PositionType::LONG to exit LONG positions...
			return ExitData{ PositionType::LONG, 0.0 };  //symbol, 
		}
	};




}

#endif // LONG_LOW_H

