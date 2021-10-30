#include "wsclient.h"
#include "wsclient_ssl.h"

#include <iostream>
#include <nlohmann/json.hpp>

// TODO not directly include this?
#include "../adapters/polygon_io.h"

// #include <date/tz.h>



using json = nlohmann::json;

using namespace agpred;


/*
static auto const ET = date::locate_zone("America/New_York");

std::string
format_date_ET(std::chrono::system_clock::time_point tp)
{
    using namespace date;
    using namespace std::chrono;
    return format("%F", zoned_time<milliseconds>{ET, floor<milliseconds>(tp)});
}

std::string
format_datetime_ET(std::chrono::system_clock::time_point tp)
{
    using namespace date;
    using namespace std::chrono;
    return format("%FT%T%z", zoned_time<milliseconds>{ET, floor<milliseconds>(tp)});
}
*/



void on_message(client* c, DataController* ctrl, websocketpp::connection_hdl hdl, client::message_ptr msg)
{
    //std::cout << msg->get_payload() << std::endl;

    json payload = json::parse(msg->get_payload());

    websocketpp::lib::error_code ec;

    //std::cout << payload  << std::endl;

    if (payload.is_array())
    {
        const auto& first = payload[0];
        if (first.is_object())
        {
            if (!strcmp(first["ev"].get<std::string>().c_str(), "status"))
            {
                //std::cout << "status message: " << first["status"].get<std::string>() << std::endl;
                
                if (!strcmp(first["status"].get<std::string>().c_str(), "connected"))
                {
                    std::cout << "> CONNECTED" << std::endl;
                    // TODO could trigger auth from here??? (if status=="connected")
                }
                else if (!strcmp(first["status"].get<std::string>().c_str(), "auth_success"))
                {
                    // SUBSCRIBE!
                    std::cout << "> AUTHENTICATED" << std::endl;

                    c->send(hdl, "{\"action\":\"subscribe\", \"params\":\"T.AAPL\"}", websocketpp::frame::opcode::text, ec);
                    if (ec) {
                        std::cout << "> Error sending message: " << ec.message() << std::endl;
                        return;
                    }

                    c->send(hdl, "{\"action\":\"subscribe\", \"params\":\"Q.AAPL\"}", websocketpp::frame::opcode::text, ec);
                    if (ec) {
                        std::cout << "> Error sending message: " << ec.message() << std::endl;
                        return;
                    }
                }
            }
            else
            {
                ctrl->onPayloads(msg->get_payload().c_str());
            }
        }
    }


}

void on_conn_open(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "> ON CONN OPEN" << std::endl;

    std::string authPayload = "{\"action\":\"auth\",\"params\":\"" + POLYGON_API_KEY + "\"}";

    websocketpp::lib::error_code ec;
    c->send(hdl, authPayload, websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cout << "> Error sending message: " << ec.message() << std::endl;
        return;
    }

}

void on_conn_close(client* c, websocketpp::connection_hdl hdl) {
    std::cout << "> ON CONN CLOSED" << std::endl;

}

