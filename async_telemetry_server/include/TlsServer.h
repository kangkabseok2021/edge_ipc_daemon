#pragma once
#include "TcpServer.h"
#include <boost/asio/ssl.hpp>
#include <string>

namespace ats {

namespace ssl = boost::asio::ssl;

class TlsServer : public TcpServer {
public:
    TlsServer(asio::io_context& ctx,
              uint16_t port,
              JwtValidator& validator,
              TelemetryRepository& repo,
              const std::string& cert_file,
              const std::string& key_file);

    asio::awaitable<void> listen();

private:
    ssl::context tls_ctx_;

    asio::awaitable<void> handle_tls_session(ssl::stream<tcp::socket> stream);
    asio::awaitable<Frame> read_frame_tls(ssl::stream<tcp::socket>& stream);
    asio::awaitable<void>  write_frame_tls(ssl::stream<tcp::socket>& stream,
                                            const Frame& frame);
};

} // namespace ats
