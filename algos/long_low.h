#ifndef LONG_LOW_H
#define LONG_LOW_H


#include <tensorflow/cc/saved_model/loader.h>

#include "../core/strategy.h"
#include "algo_common.h"


namespace agpred {

	constexpr size_t MAX_ALGOS = 4;


	const tensorflow::SignatureDef setup_bundle(tensorflow::SavedModelBundle& bundle);


    class TFModelAlgo final : public AlgoBase2<MAX_ALGOS>
    {
    public:
		explicit TFModelAlgo(const std::string& name) : AlgoBase2(name)
		{
		}

        std::array<bool, signal_size> operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const override;

        static void initStatics();

		static timestamp_us_t last_ts;
		static timestamp_us_t block_long_til_ts;
		static timestamp_us_t block_short_til_ts;
		static timestamp_us_t boost_long_til_ts;
		static timestamp_us_t boost_short_til_ts;

    };


	/*class LongLowAlgo final : public AlgoBase2<1>
	{
	public:
		constexpr static bool DUAL_OUTPUT = false;
		constexpr static bool SINGLE_BEAR_OUTPUT = false;

		LongLowAlgo(const std::string& name, bool inverse);
		~LongLowAlgo() override = default;

		std::array<bool, signal_size> operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const override
		{
			//std::cout << "LongLowAlgo: CALL()" << std::endl;

			if (inverse_)
				return { calc_prediction_signal(raw, processed, quotes, trades) < 0 };
			else
				return { calc_prediction_signal(raw, processed, quotes, trades) > 0 };
		}

		static void initStatics();
	private:

		int calc_prediction_signal(const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const;


		const bool inverse_;

	};*/


	class TFModelLongEntry final : public EntryBase<MAX_ALGOS> {
	public:
		TFModelLongEntry(const std::string& name, const unsigned frequency, const AlgoBase2<signal_size>& algo, const size_t& algo_pos)
			: EntryBase<signal_size>(name, frequency, algo, algo_pos)
		{
		}
		~TFModelLongEntry() override = default;

		EntryData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "TFModelLongEntry: EntryData CALL()" << std::endl;

			const auto& atr = snapshot.last1min.alt3;
			//std::cout << "ATR: " << atr << std::endl;  //(17)

			// TODO an actual entry ?
			//return EntryData{ PositionType::LONG, 100, snapshot.last1min.close, snapshot.last1min.close - static_cast<real_t>(0.71) };  //symbol,
			//return EntryData{ PositionType::LONG, 100, snapshot.last1min.close, snapshot.last1min.close - atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };  //symbol,
			return EntryData{ PositionType::LONG, 100, snapshot.nbbo.bid, snapshot.nbbo.bid - atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };  //symbol,
		}
	};


	class TFModelLongExit : public AlgoExitBase<MAX_ALGOS> {
	public:
		TFModelLongExit(const std::string& name, const AlgoBase2<signal_size>& algo, const size_t& algo_pos)
			: AlgoExitBase<signal_size>(name, algo, algo_pos)
		{
		}
		~TFModelLongExit() override = default;

		ExitData operator()(const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "TFModelLongExit: ExitData CALL()" << std::endl;

			// PositionType::LONG to exit LONG positions...
			return ExitData{ PositionType::LONG, 0.0 };  //symbol, 
		}
	};



	class TFModelShortEntry final : public EntryBase<MAX_ALGOS> {
	public:
		TFModelShortEntry(const std::string& name, const unsigned frequency, const AlgoBase2<signal_size>& algo, const size_t& algo_pos)
			: EntryBase<signal_size>(name, frequency, algo, algo_pos)
		{
		}
		~TFModelShortEntry() override = default;

		EntryData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "TFModelShortEntry: EntryData CALL()" << std::endl;
			// TODO an actual entry ?
			const auto& atr = snapshot.last1min.alt3;
			//return EntryData{ PositionType::SHORT, 100, snapshot.last1min.close, snapshot.last1min.close + static_cast<real_t>(0.71) };
			// return EntryData{ PositionType::SHORT, 100, snapshot.last1min.close, snapshot.last1min.close + atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };
			return EntryData{ PositionType::SHORT, 100, snapshot.nbbo.ask, snapshot.nbbo.ask + atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };
		}
	};


	class TFModelShortExit : public AlgoExitBase<MAX_ALGOS> {
	public:
		TFModelShortExit(const std::string& name, const AlgoBase2<signal_size>& algo, const size_t& algo_pos)
			: AlgoExitBase<signal_size>(name, algo, algo_pos)
		{
		}
		~TFModelShortExit() override = default;

		ExitData operator()(const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "TFModelShortExit: ExitData CALL()" << std::endl;

			// PositionType::SHORT to exit SHORT positions...
			return ExitData{ PositionType::SHORT, 0.0 };  //symbol, 
		}
	};


}

#endif // LONG_LOW_H

