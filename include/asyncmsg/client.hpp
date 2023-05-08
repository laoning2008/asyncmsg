#pragma once
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <future>

#include <asio/ip/tcp.hpp>
#include <asio/detached.hpp>
#include <asio/connect.hpp>

#include <asyncmsg/detail/connection.hpp>

namespace asyncmsg {

class client {
    using received_request_channel_map = std::unordered_map<uint32_t, std::unique_ptr<connection::packet_channel>>;
    constexpr static uint32_t reconnect_interval_seconds = 3;

public:
    client(std::string host, const uint16_t port, std::string device_id_)
    : work_guard(io_context.get_executor()), device_id(std::move(device_id_))
    , server_host(host), server_port(port) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        asio::co_spawn(io_context, start(), asio::detached);
    }

    ~client() {
        auto task = [this]() -> asio::awaitable<void> {
            if (conn) {
                co_await conn->stop();
            }
            conn = nullptr;
            io_context.stop();
        };
        
        asio::co_spawn(io_context.get_executor(), task, asio::detached);
        
        if (io_thread.joinable()) {
            io_thread.join();
        }
    }
    
    asio::io_context& get_io_context() {
        return io_context;
    }
    
    asio::awaitable<void> send_packet(packet pack) {
//        co_await schedule(io_context.get_executor());
        
        if (!conn) {
            std::cout << "send_packet--conn==nullptr" << std::endl;
            co_return;
        }
        
        co_return co_await conn->send_packet(pack);
    }

    asio::awaitable<packet> send_packet(packet pack, uint32_t timeout_seconds, uint32_t max_tries) {
//        co_await schedule(io_context.get_executor());
    

        for (auto i = 0; i < max_tries; ++i) {
            if (!conn) {
                std::cout << "send_packet--conn==nullptr" << std::endl;
                asio::steady_timer timer(io_context.get_executor(), std::chrono::seconds(timeout_seconds));
                co_await timer.async_wait(use_nothrow_awaitable);
                
//                std::cout << "connect, io_thread_id = " << io_thread.get_id() << ", this thead id=" << std::this_thread::get_id() << std::endl;

                continue;
            }
            
            auto rsp_pack = co_await conn->send_packet(pack, timeout_seconds);
            
            if (rsp_pack.is_valid()) {
                co_return rsp_pack;
            }
        }
        
        co_return packet{};
    }

    asio::awaitable<packet> await_request(uint32_t cmd) {
//        co_await schedule(io_context.get_executor());
        
        auto it = received_request_channels.find(cmd);
        if (it == received_request_channels.end()) {
            received_request_channels[cmd] = std::make_unique<connection::packet_channel>(io_context, connection::received_packet_channel_size);
        }
        
        auto [e, pack] = co_await received_request_channels[cmd]->async_receive(use_nothrow_awaitable);
        co_return e ? packet{} : pack;
    }
private:
    asio::awaitable<void> start() {
        for (;;) {
            std::cout << "connect--start" << std::endl;
            co_await connect();
            std::cout << "connect--end" << std::endl;
            
            if (!conn) {
                std::cout << "conn == nullptr" << std::endl;
                asio::steady_timer timer(io_context.get_executor(), std::chrono::seconds(reconnect_interval_seconds));
                co_await timer.async_wait(use_nothrow_awaitable);
                continue;
            }
            
            for (;;) {
                auto result = co_await(conn->request_received() || conn->connection_disconnected());
                
                if (result.index() == 0) {
                    packet pack(std::get<0>(std::move(result)));
                    auto it = received_request_channels.find(pack.packet_cmd());
                    if (it != received_request_channels.end()) {
                        co_await it->second->async_send(asio::error_code{}, pack, use_nothrow_awaitable);
                    }
                } else {
                    co_await conn->stop();
                    conn = nullptr;
                    break;
                }
            }

        }
    }
    
    asio::awaitable<void> connect() {
        asio::ip::tcp::resolver resolver(io_context);
        auto [e_resolver, endpoints] = co_await resolver.async_resolve(server_host, std::to_string(server_port), use_nothrow_awaitable);
        if (e_resolver) {
            co_return;
        }

        asio::ip::tcp::socket socket(io_context);
        
        std::cout << "async_connect begin" << std::endl;
        auto [e_connect, endpoint] = co_await asio::async_connect(socket, endpoints, use_nothrow_awaitable);
        std::cout << "async_connect end, e = " << e_connect.message() << std::endl;
        if (!e_connect) {
            conn = std::make_unique<connection>(std::move(socket), device_id);
        }
    }
private:
    volatile bool stopped{false};

    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    std::string device_id;
    std::string server_host;
    uint16_t server_port;

    std::unique_ptr<connection> conn;
    received_request_channel_map received_request_channels;
};

}
