#include "account_controller.h"

#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>


using namespace agpred;



/*AccountController::AccountController(const AGMode mode, const std::array<AlgoBase* const, 2>, const std::array<EntryBase* const, 2>, const std::array<ExitBase* const, 2>)
	: mode_(mode), algos_(algos)
{
}*/


/*void AccountController::onSnapshot(const Symbol& symbol, const Snapshot& snapshot)
{
}

void AccountController::onUpdate(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed)
{
}*/


void agpred::onUpdateLog(const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed)
{

	//(NUM_INTERVALS, NUM_TIMESTEMPS, NUM_COLUMNS)

	//if (DEBUG_PRINT_DATA)


	/*std::cout << "onUpdate(" << symbol.symbol << ") data_processed.shape:" << std::endl << data_processed.shape() << std::endl;

	xt::xarray<double> row0 = xt::view(data_processed, 0, xt::all(), xt::all());
	std::cout << "onUpdate(" << symbol.symbol << ") row[0].shape:" << std::endl << row0.shape() << std::endl;

	xt::xarray<double> row00 = xt::row(row0, 0);
	std::cout << "onUpdate(" << symbol.symbol << ") row[0][0].shape:" << std::endl << row00.shape() << std::endl;

	xt::xarray<double> row0col0 = xt::col(row0, ColPos::In::timestamp);
	std::cout << "onUpdate(" << symbol.symbol << ") row[0]col[10].shape:" << std::endl << row0col0.shape() << std::endl;
	std::cout << "onUpdate(" << symbol.symbol << ") row[0]col[10]:" << std::endl << row0col0 << std::endl;*/
}
