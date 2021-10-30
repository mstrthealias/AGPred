#include "src/consolidate.h"
#include "src/wsclient.h"
#include "src/wsclient_ssl.h"

#include "core/data_controller.h"


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
    
    std::string hostname = "*.polygon.io";
    std::string uri = "wss://socket.polygon.io/stocks";
    
    agpred::DataController ctrl;
    
    const agpred::Symbol& symbol = agpred::Symbol::get_symbol("AAPL");
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

    return 0;
}
