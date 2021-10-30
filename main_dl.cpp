#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "src/common.h"
#include "src/defs.h"
#include "adapters/polygon_io.h"  // TODO not directly include this?


using json = nlohmann::json;

using namespace agpred;


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


int main(int argc, char* argv[])
{

    timestamp_t endTs = 1632628800;
    timestamp_t startTs;
    for (const auto& tpl : INTERVAL_INITIAL_DOWNLOADS) {
        startTs = endTs - std::get<1>(tpl);
        get_aggregate_history("AAPL", std::get<0>(tpl), startTs, endTs, true, std::get<2>(tpl));
    }
    //std::cout << format_date_ET(std::chrono::system_clock::now()) << '\n';
    //std::cout << format_datetime_ET(std::chrono::system_clock::now()) << '\n';
    //get_aggregate_history("AAPL", 1, 1631545200, 1631561400, true, 50000);

    return 0;
    cpr::Response r = cpr::Get(cpr::Url{ "https://www.google.com" });
    /*cpr::Response r = cpr::Get(cpr::Url{"https://www.google.com"},
        cpr::Authentication{ "user", "pass" },
        cpr::Parameters{ {"anon", "true"}, {"key", "value"} });*/
        /*r.status_code;                  // 200
        r.header["content-type"];       // application/json; charset=utf-8
        r.text;                         // JSON text string*/

    std::cout << "Response: " << std::endl << r.text << std::endl;

    return 0;
}
