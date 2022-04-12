#define NOMINMAX  // do not define min/max macro

#include <chrono>
#include <sstream>
#include <iostream>
#include <locale>
#include <filesystem>
#include <cpr/cpr.h>
#include <ta_libc.h>

#include "core/data_controller.h"
#include "src/util.h"

#include "adapters/polygon_io.h"  // TODO not directly include this?
#include "sim/downloader.h"  // TODO not directly include this?


using json = nlohmann::json;

using namespace agpred;


constexpr bool REVERSE = false;


int get_aggregate_history(const char* symbol, unsigned int interval, timestamp_t startTs, timestamp_t endTs, bool adjusted, unsigned int limit)
{
    std::string base_url = "https://api.polygon.io/v2/aggs/ticker/AAPL/range/";
    if (interval < 60)
        base_url = base_url + std::to_string(interval) + "/minute/";
    else if (interval <= 720)
        base_url = base_url + std::to_string(static_cast<unsigned int>(interval / 60)) + "/hour/";
    else if (interval == 1440)
        base_url = base_url + "1/day/";
    else if (interval == 10080)
        base_url = base_url + "1/week/";
    else
        assert(false);
    base_url = base_url + std::to_string(startTs * 1000) + "/" + std::to_string(endTs * 1000);

    cpr::Response r = cpr::Get(
        cpr::Url{ base_url },
        cpr::Parameters{ {"adjusted", (adjusted ? "true" : "false")}, {"limit", std::to_string(limit)}, {"apiKey", POLYGON_API_KEY} }
    );

    json rJson = json::parse(r.text);

    if (rJson["count"].is_null()) {
        std::cout << symbol << "[" << interval << "] ERROR:" << std::endl << "\t" << rJson << std::endl;
    }
    else {
        std::cout << symbol << "[" << interval << "] count=" << rJson["count"] << std::endl;
    }

    return 0;
}

int get_aggregate_history(const char* symbol, unsigned int interval, timestamp_t startTs, timestamp_t endTs)
{
    return get_aggregate_history(symbol, interval, startTs, endTs, true, 50000);
}


void run_sim(const std::string& symbol_str, int year, int mon, int day) 
{
    int day_of_year = (mon - 1) * 30 + day;
    auto is_dst = (day_of_year > 71 && day_of_year < 311 ? 1 : 0);
    auto start_tp = to_time_point(year, mon, day, 7, 50, is_dst);
    auto trading_start = std::chrono::duration_cast<std::chrono::seconds>(start_tp.time_since_epoch());
    std::chrono::minutes back_test_duration(5 * 60);

    Downloader downloader(symbol_str + "." + std::to_string(year) + "-" + std::to_string(mon) + "-" + std::to_string(day));

    const AGMode mode = AGMode::BACK_TEST;

    DataController ctrl(
        mode,
        [DlPtr = &downloader](const Symbol& symbol, const Snapshot& snapshot)
        {
            DlPtr->onSnapshot(symbol, snapshot);
        },
        [DlPtr = &downloader](const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades, const xtensor_outputs_interval& outputs)
        {
            DlPtr->onUpdate(symbol, snapshot, data, data_processed, quotes, trades, outputs);
        }
    );

    // fetch symbol
    const agpred::Symbol& symbol = agpred::Symbol::get_symbol(symbol_str);

    /* // choose trading_day
    constexpr unsigned int DAYS_BACK = 2;  //17;
    const auto now = std::chrono::system_clock::now() - std::chrono::seconds(DAYS_BACK * 86400);
    // start back-testing at 7:50 am ET
    const auto utc_day_start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::floor<std::chrono::duration<int, std::ratio<86400>>>(now.time_since_epoch()));
    const auto et_day_start = utc_day_start + std::chrono::hours(7);  // TODO 6 if DST?
    auto trading_start = et_day_start + std::chrono::seconds(28200);*/
    //std::cout << "utc_day_start " << utc_day_start.count() << std::endl;
    //std::cout << "et_day_start " << et_day_start.count() << std::endl;
    //std::cout << "trading_start " << trading_start.count() << std::endl;
    
    // make sure to shift to previous minute (that-way we get history up through this minute, and download trades/quotes from beyond this minute...)
    trading_start = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::floor<std::chrono::minutes>(trading_start));

    ctrl.initSymbol(symbol, trading_start);

    // download also starts simulation, as we only collect data and save it in the callbacks
    ctrl.startSimulation(trading_start, back_test_duration);

    downloader.onSimComplete(symbol);
}

void run_symbol_date(json& obj)
{
    auto symbol = obj["symbol"].get<std::string>();
    auto date = obj["date"].get<std::string>();

    /*if (std::strcmp(symbol.c_str(), "SPY") == 0)
        return;  // skip SPY for now
    else if (std::strcmp(symbol.c_str(), "QQQ") == 0)
        return;  // skip QQQ for now
    else if (std::strcmp(symbol.c_str(), "IWM") == 0)
        return;  // skip IWM for now
    //else if (std::strcmp(symbol.c_str(), "AAPL") == 0)
    //    return;
    else if (std::strcmp(symbol.c_str(), "BA") == 0)
        return;
    else if (std::strcmp(symbol.c_str(), "MSFT") == 0)
        return;*/

    auto yr = std::stoi(date.substr(0, 4));
    auto mon = std::stoi(date.substr(5, 2));
    auto day = std::stoi(date.substr(8, 2));

    namespace fs = std::filesystem;

    std::string file = "E:/_data/_v3/" + symbol + "." + std::to_string(yr) + "-" + std::to_string(mon) + "-" + std::to_string(day) + ".0.exports.npy";
    if (fs::exists(file))
        return;
    file = "E:/_data/_v3.e/" + symbol + "." + std::to_string(yr) + "-" + std::to_string(mon) + "-" + std::to_string(day) + ".0.exports.npy";
    if (fs::exists(file))
        return;

    run_sim(symbol, yr, mon, day);
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


    /*if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
        std::cout << "Signal Handler registered..." << std::endl;
    else
        std::cout << "ERROR Unable to register Signal Handler." << std::endl;*/

    //// print numbers with 9 digit precision
    //std::cout.precision(9);
    
    std::ifstream ifs("pyfiles/train_symbol_dates_main.json");
    json jf = json::parse(ifs);

    if constexpr (REVERSE)
    {
        for (json::reverse_iterator o = jf.rbegin(); o < jf.rend(); ++o) {
            run_symbol_date(*o);
        }
    }
    else {
        for (json obj : jf) {
            run_symbol_date(obj);
        }
    }

    /*
    timestamp_t endTs = 1632628800;
    timestamp_t startTs;
    for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS) {
        startTs = endTs - std::get<1>(tpl);
        get_aggregate_history("AAPL", std::get<0>(tpl), startTs, endTs, true, std::get<2>(tpl));
    }
    //std::cout << format_date_ET(std::chrono::system_clock::now()) << '\n';
    //std::cout << format_datetime_ET(std::chrono::system_clock::now()) << '\n';
    //get_aggregate_history("AAPL", 1, 1631545200, 1631561400, true, 50000);
    */

    retCode = TA_Shutdown();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib shutdown error: " << retCode << std::endl;
        return retCode;
    }

    return 0;
}
