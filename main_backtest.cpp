#define NOMINMAX  // do not define min/max macro

#include <ta_libc.h>

#include "core/account_controller.h"
#include "core/data_controller.h"

#include "algos/ma3_ema9.h"

#include "adapters/polygon_io.h"  // TODO not directly include this?
#include "sim/simulator.h"  // TODO not directly include this?


using json = nlohmann::json;

using namespace agpred;


MA3EMA9Algo algo_ma_above("ma3_ema9", false);
MA3EMA9Algo algo_ma_below("ma3_ema9", true);
//MA3EMA9Algo algo2("ma3_ma9");

MA3EMA9Entry entry_ma("ma3_ema9 entry", 1, algo_ma_above);

MA3EMA9Exit exit_ma("ma3_ema9 exit", algo_ma_below);

StopLossExit exit_stop_loss("stop_loss exit");


const std::array<AlgoBase* const, 2> algos({ &algo_ma_above, &algo_ma_below });

const std::array<EntryBase* const, 1> entries({ &entry_ma });
const std::array<ExitBase* const, 2> exits({ &exit_ma, &exit_stop_loss });


using Ctrl = AccountController<algos.size(), entries.size(), exits.size()>;



int main(int argc, char* argv[])
{
    TA_RetCode retCode = TA_Initialize();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib initialize error: " << retCode << std::endl;
        return retCode;
    }
    std::cout << "TA-Lib initialized.\n";


    /*if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
        std::cout << "Signal Handler registered..." << std::endl;
    else
        std::cout << "ERROR Unable to register Signal Handler." << std::endl;*/

    //// print numbers with 9 digit precision
    //std::cout.precision(9);

    std::string hostname = "*.polygon.io";
    std::string uri = "wss://socket.polygon.io/stocks";

    Simulator simulator;
    //SimulatedAccountAdapter account_adapter(simulator);

    const AGMode mode = AGMode::BACK_TEST;

    Ctrl account(simulator, mode, algos, entries, exits);
    DataController ctrl(
        mode,
        [AccountPtr = &account, SimPtr = &simulator](const Symbol& symbol, const Snapshot& snapshot)
        {
            SimPtr->onSnapshot(symbol, snapshot);
            AccountPtr->onSnapshot(symbol, snapshot);
        },
        [AccountPtr = &account](const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
        {
            AccountPtr->onUpdate(symbol, snapshot, data, data_processed, quotes, trades);
        }
    );

    // fetch symbol
    const agpred::Symbol& symbol = agpred::Symbol::get_symbol("AAPL");

    // choose trading_day
    //const auto now = std::chrono::system_clock::now();
    const auto now = std::chrono::system_clock::now() - std::chrono::seconds(1 * 86400);
    // start back-testing at 7:50 am ET
    const auto utc_day_start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::floor<std::chrono::duration<int, std::ratio<86400>>>(now.time_since_epoch()));
    const auto et_day_start = utc_day_start + std::chrono::hours(7);  // TODO 6 if DST?
    auto trading_start = et_day_start + std::chrono::seconds(28200);
    //std::cout << "utc_day_start " << utc_day_start.count() << std::endl;
    //std::cout << "et_day_start " << et_day_start.count() << std::endl;
    //std::cout << "trading_start " << trading_start.count() << std::endl;
    // make sure to shift to previous minute (that-way we get history up through this minute, and download trades/quotes from beyond this minute...)
    trading_start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::floor<std::chrono::minutes>(trading_start));

    std::chrono::minutes back_test_duration(5 * 60);

    ctrl.initSymbol(symbol, trading_start);

    ctrl.startSimulation(trading_start, back_test_duration);
    
    std::cout << "Back-Test complete." << std::endl;
    std::cout << "  Profit Loss: $" << account.getProfitLoss() << std::endl;
    std::cout << "  Number of Trades: " << account.getNumTrades() << std::endl;

    retCode = TA_Shutdown();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib shutdown error: " << retCode << std::endl;
        return retCode;
    }

    return 0;
}
