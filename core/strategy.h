#ifndef STRATEGY_H
#define STRATEGY_H

#include <string>

#include "core.h"
#include "position.h"
#include "ring_buffers.h"


namespace agpred {


	class AlgoBase {
	public:
		explicit AlgoBase(const std::string& name) : id_(next_algo_id_++), name_(name)
		{
		}
		virtual ~AlgoBase() = default;

		virtual bool operator() (const Snapshot& snapshot, const xtensor_raw& raw, const xtensor_processed& processed, const quotes_queue& quotes, const trades_queue& trades) const = 0;

		[[nodiscard]] const std::string& name() const
		{
			return name_;
		}

	public:
		const size_t id_;
		const std::string name_;

	private:
		/*inline*/ static size_t next_algo_id_/* = 1*/;
	};


	class EntryBase {
	public:
		explicit EntryBase(const std::string& name, const unsigned int frequency, const AlgoBase& algo) : name_(name), frequency_(frequency), algo_(algo)
		{
		}
		virtual ~EntryBase() = default;
		
		virtual agpred::EntryData operator() (const Symbol& symbol, const Snapshot& snapshot) const = 0;

		[[nodiscard]] const std::string& name() const
		{
			return name_;
		}

	public:
		const std::string name_;
		const unsigned int frequency_;  // TODO ?
		//const bool allow_exit;  // allowExit // TODO

		const AlgoBase& algo_;  // TODO reference into a algos table ??
	};


	class ExitBase {
	public:
		explicit ExitBase(const std::string& name) : name_(name)
		{
		}
		virtual ~ExitBase() = default;

		virtual agpred::ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot& snapshot) const = 0;

		[[nodiscard]] const std::string& name() const
		{
			return name_;
		}

	private:
		const std::string name_;
	};


	// TODO call these SnapshotExitBase and IntervalExitBase ?
	
	/**
	 * Calls an algo to determine if exit should occur.
	 * The algo may be shared with other entries/exits, and will be called once/in bulk.
	 */
	class AlgoExitBase : public ExitBase {
	public:
		explicit AlgoExitBase(const std::string& name, const AlgoBase& algo) : ExitBase(name), algo_(algo)
		{
		}
		virtual ~AlgoExitBase() = default;

	public:
		const AlgoBase& algo_;
	};


	/**
	 * Calls the exit directly to determine if exit should occur.
	 */
	class SnapshotExitBase : public ExitBase {
	public:
		explicit SnapshotExitBase(const std::string& name) : ExitBase(name)
		{
		}
		virtual ~SnapshotExitBase() = default;

		virtual bool call(const Position& position, const Snapshot& snapshot) const = 0;
	};


	/*class StopLossExit final : public SnapshotExitBase {
	public:
		explicit StopLossExit(const std::string& name) : SnapshotExitBase(name)
		{
		}
		~StopLossExit() = default;

		bool call(const Position& position, const Snapshot& snapshot) const override
		{
			// TODO
			return false;
		}

		ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot & snapshot) const override
		{
			std::cout << "StopLossExit: ExitData CALL()" << std::endl;
			return ExitData{ PositionType::LONG, 0.0 };  // symbol, 
		}
	};*/


}

#endif // STRATEGY_H
