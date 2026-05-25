#pragma once
#include "FrameProtocol.h"
#include "JwtValidator.h"
#include "TelemetryRepository.h"
#include <boost/asio.hpp>
#include <cstdint>

namespace ats {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class TcpServer {
public:
    TcpServer(asio::io_context& ctx,
              uint16_t port,
              JwtValidator& validator,
              TelemetryRepository& repo);

    asio::awaitable<void> listen();

protected:
    asio::io_context&    ctx_;
    tcp::acceptor        acceptor_;
    JwtValidator&        validator_;
    TelemetryRepository& repo_;

    asio::awaitable<void> handle_session(tcp::socket socket);
    asio::awaitable<Frame> read_frame(tcp::socket& socket);
    asio::awaitable<void>  write_frame(tcp::socket& socket, const Frame& frame);
};

} // namespace ats
