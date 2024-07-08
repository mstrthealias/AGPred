#ifndef MA3_EMA9_H
#define MA3_EMA9_H


#include "../core/strategy.h"
#include "algo_common.h"


namespace agpred {


	int calc_raw_signal(const xtensor_raw& raw);



	class MA3EMA9Algo final : public AlgoBase2<1>
	{
	public:
		MA3EMA9Algo(const std::string& name, bool inverse)
			: AlgoBase2<signal_size>(name), inverse_(inverse)
		{
		}
		~MA3EMA9Algo() override = default;

		std::array<bool, signal_size> operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const override
		{
			//std::cout << "MA3EMA9Algo: CALL()" << std::endl;

			if (inverse_)
				return { calc_raw_signal(raw) < 0 };
			else
				return { calc_raw_signal(raw) > 0 };
		}
	private:
		const bool inverse_;
	};


	class MA3EMA9Entry final : public EntryBase<1> {
	public:
		MA3EMA9Entry(const std::string& name, const unsigned frequency, const AlgoBase2<signal_size>& algo)
			: EntryBase<signal_size>(name, frequency, algo, 0)
		{
		}
		~MA3EMA9Entry() override = default;
		
		EntryData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "MA3EMA9Entry: EntryData CALL()" << std::endl;

			const auto& atr = snapshot.last1min.alt3;

			// TODO an actual entry ?
			return EntryData{ PositionType::LONG, 100, snapshot.nbbo.bid, 0.0 };
			//return EntryData{ PositionType::LONG, 100, snapshot.nbbo.bid, snapshot.nbbo.bid - atr * STOPLOSS_MULTIPLIER * MULTIPLIER_ADJ };
		}
	};


	class MA3EMA9Exit : public AlgoExitBase<1> {
	public:
		MA3EMA9Exit(const std::string& name, const AlgoBase2<signal_size>& algo)
			: AlgoExitBase<signal_size>(name, algo, 0)
		{
		}
		~MA3EMA9Exit() override = default;

		ExitData operator()(const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "MA3EMA9Exit: ExitData CALL()" << std::endl;

			// PositionType::LONG to exit LONG positions...
			return ExitData{ position.type(), 0.0 };  //symbol, 
		}
	};




}

#endif // MA3_EMA9_H

