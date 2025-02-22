#ifndef ALGO_COMMON_H
#define ALGO_COMMON_H


#include "../core/strategy.h"


namespace agpred {


	constexpr real_t PROFIT_MULTIPLIER = 1.777;
	constexpr real_t STOPLOSS_MULTIPLIER = 1.0;
	constexpr real_t MULTIPLIER_ADJ = 1.07;


	class TakeProfitExit final : public SnapshotExitBase {
	public:
		explicit TakeProfitExit(const std::string& name) : SnapshotExitBase(name)
		{
		}
		~TakeProfitExit() = default;

		bool call(const Position& position, const Snapshot& snapshot) const override
		{
			//position.entry_data

			const auto& atr = snapshot.last1min.alt3;
			if (!atr) {
				// TODO handle empty ATR?
				std::cout << "snapshot.last1min: " << snapshot.last1min << std::endl;  //(17)
				return false;
			}

			if (position.type() == PositionType::LONG) {
				// TODO
				return snapshot.nbbo.bid >= position.avg_price() + (PROFIT_MULTIPLIER * atr * MULTIPLIER_ADJ);
				//return snapshot.nbbo.bid >= position.avg_price() + 1.337;
			}
			else {
				// TODO
				return snapshot.nbbo.ask <= position.avg_price() - (PROFIT_MULTIPLIER * atr * MULTIPLIER_ADJ);
				//return snapshot.nbbo.bid >= position.avg_price() + 1.337;
			}
		}

		ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			std::cout << "TakeProfitExit: ExitData CALL()" << std::endl;
			return ExitData{ position.type(), 0.0 };
		}
	};


	class TimeExit final : public SnapshotExitBase {
	public:
		explicit TimeExit(const std::string& name, const timestamp_us_t& max_time) : SnapshotExitBase(name), max_time_(max_time)
		{
		}
		~TimeExit() = default;

		bool call(const Position& position, const Snapshot& snapshot) const override
		{
			return snapshot.nbbo.timestamp >= position.ts_created() + max_time_;
		}

		ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot& snapshot) const override
		{
			std::cout << "TimeExit: ExitData CALL()" << std::endl;
			return ExitData{ position.type(), 0.0 };
		}

	private:
		const timestamp_us_t max_time_;
	};


}


#endif // ALGO_COMMON_H


