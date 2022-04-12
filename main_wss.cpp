#define NOMINMAX  // do not define min/max macro


#include <ta_libc.h>

#include "src/consolidate.h"
#include "src/wsclient.h"
#include "src/wsclient_ssl.h"

#include "core/data_controller.h"
#include "core/account_controller.h"

#include "algos/ma3_ema9.h"
#include "sim/simulator.h"  // TODO not directly include this?


using namespace agpred;


// TODO
const real_t INITIAL_BALANCE = 100000.0;
const real_t MAX_LOSS_PER_TRADE = 175.0;
const real_t MAX_LOSS_DAILY = 350;


// TODO abstract this...
MA3EMA9Algo algo_ma_above("ma3_ema9", false);
MA3EMA9Algo algo_ma_below("ma3_ema9", true);

MA3EMA9Entry entry_ma("ma3_ema9 entry", 1, algo_ma_above);

MA3EMA9Exit exit_ma("ma3_ema9 exit", algo_ma_below);

//StopLossExit exit_stop_loss("stop_loss exit");


const std::array<AlgoBase* const, 2> algos({ &algo_ma_above, &algo_ma_below });

const std::array<EntryBase* const, 1> entries({ &entry_ma });
const std::array<ExitBase* const, 2> exits({ &exit_ma });


using Ctrl = AccountController<algos.size(), entries.size(), exits.size()>;



client c;



void on_timer(client* c, websocketpp::lib::error_code const& ec) {
    std::cout << "> ON TIMER " << ec << std::endl;
    
    // repeat ...
    c->set_timer(250, bind(&on_timer, c, ::_1));
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
        printf("Ctrl-C event\n\n");
        Beep(750, 300);
        c.stop();
        return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
        Beep(600, 200);
        printf("Ctrl-Close event\n\n");
        return TRUE;

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        Beep(900, 200);
        printf("Ctrl-Break event\n\n");
        c.stop();
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        Beep(1000, 200);
        printf("Ctrl-Logoff event\n\n");
        c.stop();
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        Beep(750, 500);
        printf("Ctrl-Shutdown event\n\n");
        c.stop();
        return FALSE;

    default:
        return FALSE;
    }
}

int main(int argc, char* argv[])
{

    if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
        std::cout << "Signal Handler registered..." << std::endl;
    else
        std::cout << "ERROR Unable to register Signal Handler." << std::endl;


    TA_RetCode retCode = TA_Initialize();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib initialize error: " << retCode << std::endl;
        return retCode;
    }
    std::cout << "TA-Lib initialized.\n";


    //// print numbers with 9 digit precision
    //std::cout.precision(9);

    std::string hostname = "*.polygon.io";
    std::string uri = "wss://socket.polygon.io/stocks";

    // TODO use lambda and call simulator and account in onSnapshot
    Simulator simulator({
        .account_balance = INITIAL_BALANCE,
        .max_trade_loss = MAX_LOSS_PER_TRADE,
        .max_daily_loss = MAX_LOSS_DAILY
    });

    const AGMode mode = AGMode::LIVE_TEST;

    Ctrl account(simulator, mode, algos, entries, exits);
    DataController ctrl(
        mode,
        [AccountPtr = &account, SimPtr = &simulator](const Symbol& symbol, const Snapshot& snapshot)
        {
            AccountPtr->onSnapshot(symbol, snapshot);
        },
        [AccountPtr = &account](const Symbol& symbol, const Snapshot& snapshot, const xtensor_raw& data, const xtensor_processed& data_processed, const quotes_queue& quotes, const trades_queue& trades)
        {
            AccountPtr->onUpdate(symbol, snapshot, data, data_processed, quotes, trades);
        }
    );
    
    const Symbol& symbol = Symbol::get_symbol("AAPL");
    ctrl.initSymbol(symbol);

    std::cout << "Symbol: " << std::string(symbol.symbol) << std::endl;
    
    try {
        // Set logging to be pretty verbose (everything except message payloads)
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);
        c.clear_access_channels(websocketpp::log::alevel::frame_header);
        c.clear_access_channels(websocketpp::log::alevel::control);
        c.set_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        c.init_asio();

        // Register handlers
        c.set_message_handler(bind(&on_message, &c, &ctrl, ::_1, ::_2));

        c.set_open_handler(bind(&on_conn_open, &c, ::_1));

        c.set_close_handler(bind(&on_conn_close, &c, ::_1));

        c.set_tls_init_handler(bind(&on_tls_init, hostname.c_str(), ::_1));

        //c.set_timer(250, bind(&on_timer, &c, ::_1));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 1;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        c.connect(con);

        c.get_alog().write(websocketpp::log::alevel::app, "Connecting to " + uri);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run() will exit when this connection is closed.
        c.run();

        // TODO properly close connection?
        //con->close()
    }
    catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    }


    retCode = TA_Shutdown();
    if (retCode != TA_SUCCESS)
    {
        std::cout << "TA-Lib shutdown error: " << retCode << std::endl;
        return retCode;
    }

    return 0;
}
