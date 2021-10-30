#ifndef STRATEGY_H
#define STRATEGY_H

#include <xtensor/xarray.hpp>

#include "core.h"


namespace agpred {
	class AlgoBase {
		const char* name;

		virtual bool operator () (const Snapshot& snapshot, const xt::xarray<double>& a_inputs) const = 0;  // TODO xt::xarray<> or specific class??
	};


	class EntryBase {
		const char* name;
		const unsigned int frequency;  // TODO ?
		//const bool allow_exit;  // allowExit // TODO

		const AlgoBase& algo;  // TODO reference into a algos table ??


		virtual agpred::EntryData operator () (Symbol symbol, const Snapshot& snapshot) const = 0;
	};


	class ExitBase {
		const char* name;

		const AlgoBase& algo;  // TODO reference into a algos table ??


		virtual agpred::ExitData operator () (Symbol symbol, const Snapshot& snapshot) const = 0;
	};

}

#endif // STRATEGY_H
