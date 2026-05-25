#include "TcpServer.h"
#include "SensorFrame.h"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <arpa/inet.h>
#include <cstring>
#include <cmath>
#include <random>
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <numbers>

using namespace ats;
namespace asio = boost::asio;
using asio_tcp = asio::ip::tcp;

static asio::awaitable<Frame> read_frame_plain(asio_tcp::socket& sock) {
    std::vector<std::byte> hdr_buf(12);
    co_await asio::async_read(sock,
        asio::buffer(hdr_buf.data(), 12), asio::use_awaitable);
    uint32_t plen_be{};
    std::memcpy(&plen_be, hdr_buf.data() + 6, 4);
    uint32_t plen = ntohl(plen_be);
    std::vector<std::byte> full(12 + plen);
    std::memcpy(full.data(), hdr_buf.data(), 12);
    if (plen > 0)
        co_await asio::async_read(sock,
            asio::buffer(full.data() + 12, plen), asio::use_awaitable);
    co_return Frame::deserialize(full);
}

static asio::awaitable<void> write_frame_plain(asio_tcp::socket& sock,
                                                const Frame& frame) {
    auto bytes = frame.serialize();
    co_await asio::async_write(sock,
        asio::buffer(bytes.data(), bytes.size()), asio::use_awaitable);
}

static asio::awaitable<void> run_client(
        const std::string& host, uint16_t port, const std::string& jwt_token) {
    auto executor = co_await asio::this_coro::executor;
    asio_tcp::resolver resolver{executor};
    auto endpoints = co_await resolver.async_resolve(
        host, std::to_string(port), asio::use_awaitable);
    asio_tcp::socket socket{executor};
    co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

    // AUTH
    Frame auth;
    auth.msg_type = MsgType::AUTH;
    auth.payload.assign(
        reinterpret_cast<const std::byte*>(jwt_token.data()),
        reinterpret_cast<const std::byte*>(jwt_token.data() + jwt_token.size()));
    co_await write_frame_plain(socket, auth);

    Frame ack = co_await read_frame_plain(socket);
    if (ack.msg_type != MsgType::ACK)
        throw std::runtime_error("AUTH rejected by server");

    std::cout << "[client] authenticated\n";

    // TELEMETRY — 60 frames at 1 Hz, sinusoidal + Gaussian noise
    constexpr double A     = 100.0;
    constexpr double F_HZ  = 0.1;
    constexpr double SIGMA = 2.0;

    std::mt19937_64            rng{std::random_device{}()};
    std::normal_distribution<> noise{0.0, SIGMA};

    for (int i = 0; i < 60; ++i) {
        double t   = static_cast<double>(i);
        double val = A * std::sin(2.0 * std::numbers::pi * F_HZ * t) + noise(rng);

        auto now_us = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        SensorFrame sf{1u, now_us, val};
        auto sf_bytes = sf.serialize();

        Frame tel;
        tel.msg_type = MsgType::TELEMETRY;
        tel.payload  = sf_bytes;
        co_await write_frame_plain(socket, tel);

        Frame ack2 = co_await read_frame_plain(socket);
        (void)ack2;

        std::cout << "[client] frame " << (i + 1) << "/60  value=" << val << "\n";
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
    std::cout << "[client] done.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: telemetry_client <host> <port> <jwt_token>\n";
        return 1;
    }
    std::string host  = argv[1];
    auto        port  = static_cast<uint16_t>(std::stoi(argv[2]));
    std::string token = argv[3];

    asio::io_context ctx;
    asio::co_spawn(ctx, run_client(host, port, token),
        [](std::exception_ptr ep) {
            if (ep) std::rethrow_exception(ep);
        });
    ctx.run();
    return 0;
}
