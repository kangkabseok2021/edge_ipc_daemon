#include "TcpServer.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

namespace ats {

TcpServer::TcpServer(asio::io_context& ctx, uint16_t port,
                     JwtValidator& v, TelemetryRepository& r)
    : ctx_{ctx}
    , acceptor_{ctx, tcp::endpoint{tcp::v4(), port}}
    , validator_{v}
    , repo_{r}
{}

asio::awaitable<void> TcpServer::listen() { co_return; }

asio::awaitable<void> TcpServer::handle_session(tcp::socket /*socket*/) {
    co_return;
}

asio::awaitable<Frame> TcpServer::read_frame(tcp::socket& /*socket*/) {
    co_return Frame{};
}

asio::awaitable<void> TcpServer::write_frame(tcp::socket& /*socket*/,
                                              const Frame& /*frame*/) {
    co_return;
}

} // namespace ats
