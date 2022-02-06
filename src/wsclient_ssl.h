
#ifndef WSCLIENT_SSL_H
#define WSCLIENT_SSL_H


#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#include <websocketpp/config/asio_client.hpp>



typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::placeholders::_3;
using websocketpp::lib::placeholders::_4;
using websocketpp::lib::bind;


#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif



/// Verify that one of the subject alternative names matches the given hostname
bool verify_subject_alternative_name(const char* hostname, X509* cert);

/// Verify that the certificate common name matches the given hostname
bool verify_common_name(char const* hostname, X509* cert);

/**
 * This code is derived from examples and documentation found ato00po
 * http://www.boost.org/doc/libs/1_61_0/doc/html/boost_asio/example/cpp03/ssl/client.cpp
 * and
 * https://github.com/iSECPartners/ssl-conservatory
 */
bool verify_certificate(const char* hostname, bool preverified, asio::ssl::verify_context& ctx);

/// TLS Initialization handler
/**
 * WebSocket++ core and the Asio Transport do not handle TLS context creation
 * and setup. This callback is provided so that the end user can set up their
 * TLS context using whatever settings make sense for their application.
 *
 * As Asio and OpenSSL do not provide great documentation for the very common
 * case of connect and actually perform basic verification of server certs this
 * example includes a basic implementation (using Asio and OpenSSL) of the
 * following reasonable default settings and verification steps:
 *
 * - Disable SSLv2 and SSLv3
 * - Load trusted CA certificates and verify the server cert is trusted.
 * - Verify that the hostname matches either the common name or one of the
 *   subject alternative names on the certificate.
 *
 * This is not meant to be an exhaustive reference implimentation of a perfect
 * TLS client, but rather a reasonable starting point for building a secure
 * TLS encrypted WebSocket client.
 *
 * If any TLS, Asio, or OpenSSL experts feel that these settings are poor
 * defaults or there are critically missing steps please open a GitHub issue
 * or drop a line on the project mailing list.
 *
 * Note the bundled CA cert ca-chain.cert.pem is the CA cert that signed the
 * cert bundled with echo_server_tls. You can use print_client_tls with this
 * CA cert to connect to echo_server_tls as long as you use /etc/hosts or
 * something equivilent to spoof one of the names on that cert
 * (websocketpp.org, for example).
 */
context_ptr on_tls_init(const char* hostname, websocketpp::connection_hdl);


#endif // WSCLIENT_SSL_H
