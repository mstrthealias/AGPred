#ifndef LONG_LOW_H
#define LONG_LOW_H


#include <tensorflow/cc/saved_model/loader.h>

#include "../core/strategy.h"
#include "algo_common.h"


namespace agpred {

	const tensorflow::SignatureDef setup_bundle(tensorflow::SavedModelBundle& bundle);


	class LongLowAlgo final : public AlgoBase
	{
	public:
		constexpr static bool DUAL_OUTPUT = false;
		constexpr static bool SINGLE_BEAR_OUTPUT = false;

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

		static void initStatics();
	private:

		int calc_prediction_signal(const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const;


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

			const auto& atr = snapshot.last1min.alt3;
			//std::cout << "ATR: " << atr << std::endl;  //(17)

			// TODO an actual entry ?
			//return EntryData{ PositionType::LONG, 100, snapshot.last1min.close, snapshot.last1min.close - static_cast<real_t>(0.71) };  //symbol,
			return EntryData{ PositionType::LONG, 100, snapshot.last1min.close, snapshot.last1min.close - atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };  //symbol,
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



	class ShortHighEntry final : public EntryBase {
	public:
		ShortHighEntry(const std::string& name, const unsigned frequency, const AlgoBase& algo)
			: EntryBase(name, frequency, algo)
		{
		}
		~ShortHighEntry() override = default;

		EntryData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "ShortHighEntry: EntryData CALL()" << std::endl;
			// TODO an actual entry ?
			const auto& atr = snapshot.last1min.alt3;
			return EntryData{ PositionType::SHORT, 100, snapshot.last1min.close, snapshot.last1min.close + atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };
			//return EntryData{ PositionType::SHORT, 100, snapshot.last1min.close, snapshot.last1min.close + static_cast<real_t>(0.71) };
		}
	};


	class ShortHighExit : public AlgoExitBase {
	public:
		ShortHighExit(const std::string& name, const AlgoBase& algo)
			: AlgoExitBase(name, algo)
		{
		}
		~ShortHighExit() override = default;

		ExitData operator()(const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "ShortHighExit: ExitData CALL()" << std::endl;

			// PositionType::SHORT to exit SHORT positions...
			return ExitData{ PositionType::SHORT, 0.0 };  //symbol, 
		}
	};


}

#endif // LONG_LOW_H

