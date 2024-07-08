#define NOMINMAX  // do not define min/max macro

#include <ta_libc.h>

#include "core/account_controller.h"
#include "core/data_controller.h"
#include "src/util.h"

#include "algos/ma3_ema9.h"
#include "algos/long_low.h"

#include "adapters/polygon_io.h"  // TODO not directly include this?
#include "sim/simulator.h"  // TODO not directly include this?


using json = nlohmann::json;

using namespace agpred;


const real_t INITIAL_BALANCE = 100000.0f;
const real_t MAX_LOSS_PER_TRADE = 175.0;
const real_t MAX_LOSS_DAILY = 350.0f;  // 1200.0f;  //350.0;
static real_t account_balance = INITIAL_BALANCE;


TakeProfitExit exit_take_profit("take_profit exit");

TFModelAlgo algo_tf_model("tf_model");
TFModelLongEntry entry_long0("profit0 entry", 1, algo_tf_model, 0);
TFModelShortEntry entry_short0("short0 entry", 1, algo_tf_model, 1);
TFModelLongExit exit_long0("profit0 exit", algo_tf_model, 3);
TFModelShortExit exit_short0("short0 exit", algo_tf_model, 2);
TimeExit exit_both("time exit", 9 * MIN_TO_US);


const std::array<AlgoBase2<MAX_ALGOS>* const, 1> algos({ &algo_tf_model });
//const std::array<EntryBase<MAX_ALGOS>* const, 1> entries({ &entry_long0 });
const std::array<EntryBase<MAX_ALGOS>* const, 2> entries({ &entry_long0, &entry_short0 });
//const std::array<ExitBase* const, 1> exits({ &exit_take_profit });
const std::array<ExitBase* const, 4> exits({ &exit_take_profit, &exit_long0, &exit_short0, &exit_both });

using Ctrl = AccountController<algos.size(), entries.size(), exits.size(), MAX_ALGOS>;


//MA3EMA9Algo algo_ma_above("ma3_ema9", false);
//MA3EMA9Algo algo_ma_below("ma3_ema9", true);
//MA3EMA9Entry entry_ma("ma3_ema9 entry", 1, algo_ma_above);
//MA3EMA9Exit exit_ma("ma3_ema9 exit", algo_ma_below);
//
//const std::array<AlgoBase2<1>* const, 2> algos({ &algo_ma_above, &algo_ma_below });
//const std::array<EntryBase<1>* const, 1> entries({ &entry_ma });
//// const std::array<ExitBase* const, 2> exits({ &exit_ma, &exit_take_profit });
//const std::array<ExitBase* const, 1> exits({ &exit_ma });
//
//using Ctrl = AccountController<algos.size(), entries.size(), exits.size(), 1>;


void run_range(const std::string& symbol_str, int start_yr, int start_mon, int start_day, int end_yr, int end_mon, int end_day)
{
    int day_of_year = (start_mon - 1) * 30 + start_day;
    auto is_dst = (day_of_year > 71 && day_of_year < 311 ? 1 : 0);
    auto start_tp = to_time_point(start_yr, start_mon, start_day, 7, 50, is_dst);
    auto trading_start = std::chrono::duration_cast<std::chrono::seconds>(start_tp.time_since_epoch());

    day_of_year = (end_mon - 1) * 30 + end_day;
    is_dst = (day_of_year > 71 && day_of_year < 311 ? 1 : 0);
    auto end_tp = to_time_point(end_yr, end_mon, end_day, 7, 50, is_dst);
    auto last_trading_start = std::chrono::duration_cast<std::chrono::seconds>(end_tp.time_since_epoch());

    float totalProfitLoss = 0.0;
    size_t totalTrades = 0;
    size_t day_no = 0;

    for (auto trading_day = start_tp; trading_day <= end_tp; trading_day += std::chrono::days(1))
    {
        if (!is_trading_day(trading_day))
            continue;

        Simulator simulator({
            .account_balance = account_balance,
            .max_trade_loss = MAX_LOSS_PER_TRADE,
            .max_daily_loss = MAX_LOSS_DAILY
        });

        const AGMode mode = AGMode::BACK_TEST;

        Ctrl account(simulator, mode, algos, entries, exits);
        DataController ctrl(
            mode,
            [AccountPtr = &account, SimPtr = &simulator](const Symbol& symbol, const Snapshot& snapshot)
            {
                SimPtr->onSnapshot(symbol, snapshot);
                AccountPtr->onSnapshot(symbol, snapshot);
            },
            [AccountPtr = &account](const Symbol& symbol, const Snapshot& snapshot, const xtensor_ts_interval& data_ts, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
            {
                AccountPtr->onUpdate(
                    symbol, 
                    snapshot, 
                    data_ts,
                    data,  // TODO // vec_cleanup_float_errs(data),  //remove float / double conversion issues
                    data_processed, 
                    quotes,
                    trades);
            }
            );

        // fetch symbol
        const agpred::Symbol& symbol = agpred::Symbol::get_symbol(symbol_str);

        // make sure to shift to previous minute (that-way we get history up through this minute, and download trades/quotes from beyond this minute...)
        auto start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::floor<std::chrono::minutes>(trading_day.time_since_epoch()));

        std::chrono::minutes back_test_duration(5 * 60);

        // TODO initialize symbol in account ?
        simulator.initSymbol(symbol);  // TODO call from elsewhere?

        ctrl.initSymbol(symbol, start);

        ctrl.startSimulation(start, back_test_duration);

        account.closePositions(symbol, std::nullopt, [AccountPtr = &account, TotalProfitLoss = &totalProfitLoss, TotalTrades = &totalTrades, DayNo = &day_no, AccountBalance = &account_balance](const Symbol& symbol) {

            //using std::date::operator<<;
            //std::cout << sys_time<std::chrono::minutes>(trading_day) << ":" << std::endl;
            //std::cout << trading_day.time_since_epoch() << ":" << std::endl;
            std::cout << "Day #" << (AccountPtr->getProfitLoss() < 0 ? st_red_s : st_green_s)(std::to_string(++(*DayNo))) << ":" << std::endl;
            std::cout << "  Daily Profit Loss: $" << st_real_clr(AccountPtr->getProfitLoss() - 2 * AccountPtr->getNumTrades()) << std::endl;
            std::cout << "  Daily Number of Trades: " << AccountPtr->getNumTrades() << std::endl;

            *TotalProfitLoss += AccountPtr->getProfitLoss();
            *TotalTrades += AccountPtr->getNumTrades();
            *AccountBalance += AccountPtr->getProfitLoss();

        });  // TODO call from elsewhere?

        // call simulator snapshot once to close MARKET orders
        simulator.onSnapshot(symbol, ctrl.getSnapshot(symbol));

    }

    // TODO handle async callback condition?

    std::cout << (totalProfitLoss < 0 ? st_red_s : st_green_s)("Back-Test date range complete:") << std::endl;
    std::cout << "  Profit Loss: $" << st_real_clr(totalProfitLoss) << std::endl;
    std::cout << "  Number of Trades: " << totalTrades << std::endl;
    std::cout << "  Commissions: $" << (2 * totalTrades) << std::endl;
    std::cout << "  Account Balance: $" << st_real_clr(account_balance - 2 * totalTrades, INITIAL_BALANCE) << std::endl;

}

void run_day(const std::string& symbol_str, int yr, int mon, int day) {

    int day_of_year = (mon - 1) * 30 + day;
    auto is_dst = (day_of_year > 71 && day_of_year < 311 ? 1 : 0);
    auto start_tp = to_time_point(yr, mon, day, 7, 50, is_dst);
    auto trading_start = std::chrono::duration_cast<std::chrono::seconds>(start_tp.time_since_epoch());

    Simulator simulator({
        .account_balance = account_balance,
        .max_trade_loss = MAX_LOSS_PER_TRADE,
        .max_daily_loss = MAX_LOSS_DAILY
    });

    const AGMode mode = AGMode::BACK_TEST;

    Ctrl account(simulator, mode, algos, entries, exits);
    DataController ctrl(
        mode,
        [AccountPtr = &account, SimPtr = &simulator](const Symbol& symbol, const Snapshot& snapshot)
        {
            SimPtr->onSnapshot(symbol, snapshot);
            AccountPtr->onSnapshot(symbol, snapshot);
        },
        [AccountPtr = &account](const Symbol& symbol, const Snapshot& snapshot, const xtensor_ts_interval& data_ts, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
        {
            AccountPtr->onUpdate(symbol, snapshot, data_ts, data, data_processed, quotes, trades);
        }
    );

    // fetch symbol
    const agpred::Symbol& symbol = agpred::Symbol::get_symbol(symbol_str);

    // make sure to shift to previous minute (that-way we get history up through this minute, and download trades/quotes from beyond this minute...)
    auto start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::floor<std::chrono::minutes>(trading_start));

    std::chrono::minutes back_test_duration(5 * 60);

    // TODO initialize symbol in account ?
    simulator.initSymbol(symbol);  // TODO call from elsewhere?

    ctrl.initSymbol(symbol, start);

    ctrl.startSimulation(start, back_test_duration);

    // TODO call from elsewhere?
    account.closePositions(symbol, std::nullopt, [AccountPtr = &account, AccountBalance = &account_balance](const Symbol& symbol) {
        *AccountBalance += AccountPtr->getProfitLoss();

        std::cout << "Back-Test complete." << std::endl;
        std::cout << "  Profit Loss: $" << st_real_clr(AccountPtr->getProfitLoss()) << std::endl;
        std::cout << "  Number of Trades: " << AccountPtr->getNumTrades() << std::endl;
        std::cout << "  Commissions: " << (2 * AccountPtr->getNumTrades()) << std::endl;
        std::cout << "  Account Balance: $" << st_real_clr(*AccountBalance - 2 * AccountPtr->getNumTrades(), INITIAL_BALANCE) << std::endl;
    });

    // call simulator snapshot once to close MARKET orders
    simulator.onSnapshot(symbol, ctrl.getSnapshot(symbol));

}


int main(int argc, char* argv[])
{
    TA_RetCode retCode = TA_Initialize();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib initialize error: " << retCode << std::endl;
        return retCode;
    }
    std::cout << "TA-Lib initialized.\n";

    // initialize tensorflow
    //LongLowAlgo::initStatics();
    TFModelAlgo::initStatics();
    std::cout << "Tensorflow initialized.\n";

    /*if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
        std::cout << "Signal Handler registered..." << std::endl;
    else
        std::cout << "ERROR Unable to register Signal Handler." << std::endl;*/

    //// print numbers with 9 digit precision
    //std::cout.precision(9);

    //run_day("MSFT", 2022, 4, 7);
    //run_day("MSFT", 2022, 4, 8);
    //run_day("MSFT", 2022, 4, 21);
    //run_day("PDD");
    //run_day("SOFI");

    //run_range("MSFT", 2021, 1, 4, 2021, 5, 5);
    //run_range("MSFT", 2022, 1, 3, 2022, 5, 2);
    run_range("MSFT", 2021, 9, 1, 2021, 10, 1);

    //run_day("PLAN", 2022, 3, 21);

    //run_range("MSFT", 2022, 1, 17, 2022, 1, 28);
    //run_range("MSFT", 2022, 3, 1, 2022, 3, 4);
    //run_range("SPY", 2018, 3, 1, 2018, 3, 7);

    retCode = TA_Shutdown();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib shutdown error: " << retCode << std::endl;
        return retCode;
    }

    return 0;
}
