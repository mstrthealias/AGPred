#ifndef MA3_EMA9_H
#define MA3_EMA9_H


#include "../core/strategy.h"


namespace agpred {


	int calc_raw_signal(const xtensor_raw& raw);


	class MA3EMA9Algo final : public AlgoBase
	{
	public:
		MA3EMA9Algo(const std::string& name, bool inverse)
			: AlgoBase(name), inverse_(inverse)
		{
		}
		~MA3EMA9Algo() override = default;

		bool operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed) const override
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
			return EntryData{ PositionType::LONG, 100, snapshot.price};  //symbol, 
		}
	};


	class MA3EMA9Exit : public AlgoExitBase {
	public:
		MA3EMA9Exit(const std::string& name, const AlgoBase& algo)
			: AlgoExitBase(name, algo)
		{
		}
		~MA3EMA9Exit() override = default;

		ExitData operator()(const Symbol& symbol, const Snapshot& snapshot) const override
		{
			//std::cout << "MA3EMA9Exit: ExitData CALL()" << std::endl;

			// PositionType::LONG to exit LONG positions...
			return ExitData{PositionType::LONG, 0.0};  //symbol, 
		}
	};




}

#endif // MA3_EMA9_H

