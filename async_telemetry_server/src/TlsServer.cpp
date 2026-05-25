#include "TlsServer.h"

namespace ats {

TlsServer::TlsServer(asio::io_context& ctx, uint16_t port,
                     JwtValidator& v, TelemetryRepository& r,
                     const std::string& /*cert*/, const std::string& /*key*/)
    : TcpServer{ctx, port, v, r}
    , tls_ctx_{ssl::context::tls_server}
{}

asio::awaitable<void> TlsServer::listen() { co_return; }

asio::awaitable<void> TlsServer::handle_tls_session(
        ssl::stream<tcp::socket> /*stream*/) {
    co_return;
}

asio::awaitable<Frame> TlsServer::read_frame_tls(
        ssl::stream<tcp::socket>& /*stream*/) {
    co_return Frame{};
}

asio::awaitable<void> TlsServer::write_frame_tls(
        ssl::stream<tcp::socket>& /*stream*/, const Frame& /*frame*/) {
    co_return;
}

} // namespace ats
