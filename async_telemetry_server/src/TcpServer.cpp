#include "TcpServer.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

namespace ats {

TcpServer::TcpServer(asio::io_context& ctx, uint16_t port,
                     JwtValidator& v, TelemetryRepository& r)
    : ctx_{ctx}
    , acceptor_{ctx, tcp::endpoint{tcp::v4(), port}}
    , validator_{v}
    , repo_{r}
{
    acceptor_.set_option(tcp::acceptor::reuse_address{true});
}

asio::awaitable<Frame> TcpServer::read_frame(tcp::socket& sock) {
    std::vector<std::byte> hdr_buf(12);
    co_await asio::async_read(sock,
        asio::buffer(hdr_buf.data(), 12), asio::use_awaitable);

    uint32_t plen_be{};
    std::memcpy(&plen_be, hdr_buf.data() + 6, 4);
    uint32_t plen = ntohl(plen_be);

    std::vector<std::byte> full(12 + plen);
    std::memcpy(full.data(), hdr_buf.data(), 12);
    if (plen > 0) {
        co_await asio::async_read(sock,
            asio::buffer(full.data() + 12, plen), asio::use_awaitable);
    }
    co_return Frame::deserialize(full);
}

asio::awaitable<void> TcpServer::write_frame(tcp::socket& sock,
                                              const Frame& frame) {
    auto bytes = frame.serialize();
    co_await asio::async_write(sock,
        asio::buffer(bytes.data(), bytes.size()), asio::use_awaitable);
}

asio::awaitable<void> TcpServer::handle_session(tcp::socket socket) {
    std::string peer_ip = socket.remote_endpoint().address().to_string();
    try {
        // AUTH
        Frame auth_frame = co_await read_frame(socket);
        if (auth_frame.msg_type != MsgType::AUTH) {
            Frame err; err.msg_type = MsgType::ERROR;
            co_await write_frame(socket, err);
            co_return;
        }

        std::string token(
            reinterpret_cast<const char*>(auth_frame.payload.data()),
            auth_frame.payload.size());

        auto result = validator_.validate(token);
        std::string peer_id = result.subject.empty()
            ? std::string{"unknown"} : result.subject;
        repo_.insert_auth_log(peer_id,
            result.ok ? "AUTH_OK" : "AUTH_FAIL_SIG",
            peer_ip);

        if (!result.ok) {
            Frame err; err.msg_type = MsgType::ERROR;
            co_await write_frame(socket, err);
            co_return;
        }

        Frame ack; ack.msg_type = MsgType::ACK;
        co_await write_frame(socket, ack);

        // TELEMETRY loop
        while (true) {
            Frame tel = co_await read_frame(socket);
            if (tel.msg_type != MsgType::TELEMETRY) break;

            if (tel.payload.size() >= 20) {
                uint32_t sensor_id{};
                uint64_t ts_us{};
                double   value{};
                size_t   off = 0;
                std::memcpy(&sensor_id, tel.payload.data() + off, 4); off += 4;
                std::memcpy(&ts_us,     tel.payload.data() + off, 8); off += 8;
                std::memcpy(&value,     tel.payload.data() + off, 8);
                repo_.insert_telemetry(sensor_id, ts_us, value, result.subject);
            }

            Frame ack2; ack2.msg_type = MsgType::ACK;
            co_await write_frame(socket, ack2);
        }
    } catch (const boost::system::system_error& e) {
        if (e.code() != asio::error::eof &&
            e.code() != asio::error::connection_reset) {
            std::cerr << "[TcpServer] session error: " << e.what() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[TcpServer] exception: " << e.what() << "\n";
    }
}

asio::awaitable<void> TcpServer::listen() {
    asio::signal_set signals{ctx_, SIGTERM, SIGINT};
    asio::co_spawn(ctx_,
        [&]() -> asio::awaitable<void> {
            co_await signals.async_wait(asio::use_awaitable);
            ctx_.stop();
        },
        asio::detached);

    while (true) {
        tcp::socket socket = co_await acceptor_.async_accept(
            asio::use_awaitable);
        asio::co_spawn(ctx_,
            handle_session(std::move(socket)),
            asio::detached);
    }
}

} // namespace ats
