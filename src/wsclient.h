#ifndef WSCLIENT_H
#define WSCLIENT_H

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include "common.h"

#include "../core/data_controller.h"


typedef websocketpp::client<websocketpp::config::asio_tls_client> client;



void on_message(client* c, agpred::DataController* ctrl, websocketpp::connection_hdl hdl, client::message_ptr msg);
void on_conn_open(client* c, websocketpp::connection_hdl hdl);
void on_conn_close(client* c, websocketpp::connection_hdl hdl);



#endif // WSCLIENT_H
