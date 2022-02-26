#ifndef MA3_EMA9_H
#define MA3_EMA9_H


#include "../core/strategy.h"


namespace agpred {


	int calc_raw_signal(const xtensor_raw& raw);


	class TakeProfitExit final : public SnapshotExitBase {
	public:
		explicit TakeProfitExit(const std::string& name) : SnapshotExitBase(name)
		{
		}
		~TakeProfitExit() = default;

		bool call(const Position& position, const Snapshot& snapshot) const override
		{
			// TODO
			return snapshot.nbbo.bid >= position.avg_price() + 1.337;
		}

		ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			std::cout << "TakeProfitExit: ExitData CALL()" << std::endl;
			return ExitData{ position.type(), 0.0};
		}
	};



	class MA3EMA9Algo final : public AlgoBase
	{
	public:
		MA3EMA9Algo(const std::string& name, bool inverse)
			: AlgoBase(name), inverse_(inverse)
		{
		}
		~MA3EMA9Algo() override = default;

		bool operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const override
		{
			//std::cout << "MA3EMA9Algo: CALL()" << std::endl;

			if (inverse_)
				return calc_raw_signal(raw) < 0;
			else
				return calc_raw_signal(raw) > 0;
		}
	private:
		const bool inverse_;
	};


	class MA3EMA9Entry final : public EntryBase {
	public:
		MA3EMA9Entry(const std::string& name, const unsigned frequency, const AlgoBase& algo)
			: EntryBase(name, frequency, algo)
		{
		}
		~MA3EMA9Entry() override = default;
		
		EntryData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "MA3EMA9Entry: EntryData CALL()" << std::endl;

			// TODO an actual entry ?
			return EntryData{ PositionType::LONG, 100, snapshot.last1min.close, snapshot.last1min.close - 35 };
		}
	};


	class MA3EMA9Exit : public AlgoExitBase {
	public:
		MA3EMA9Exit(const std::string& name, const AlgoBase& algo)
			: AlgoExitBase(name, algo)
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

