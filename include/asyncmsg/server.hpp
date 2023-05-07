#pragma once
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <list>
#include <future>
#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/detail/async_schedule.hpp>

namespace asyncmsg {

class server {
    using received_request_channel_map = std::unordered_map<uint32_t, std::unique_ptr<connection::packet_channel>>;
    using connection_list = std::list<std::unique_ptr<connection>>;
public:
    server(uint16_t port_)
    : work_guard(io_context.get_executor()), acceptor(io_context.get_executor(), {asio::ip::tcp::v4(), port_}) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        
        asio::co_spawn(io_context, start(), asio::detached);
    }
    
    ~server() {
        io_context.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }
    }
    
    asio::io_context& get_io_context() {
        return io_context;
    }
    
    asio::awaitable<void> send_packet(packet pack) {
//        co_await schedule(io_context.get_executor());
        for (auto& conn : connections) {
            if (conn->get_device_id() == pack.packet_device_id()) {
                co_await conn->send_packet(pack);
            }
        }
    }
    
    asio::awaitable<packet> send_packet(packet pack, uint32_t timeout_seconds, uint32_t max_tries) {
//        co_await schedule(io_context.get_executor());
        
        for (auto& conn : connections) {
            if (conn->get_device_id() == pack.packet_device_id()) {
                for (auto i = 0; i < max_tries; ++i) {
                    auto rsp_pack = co_await conn->send_packet(pack, timeout_seconds);
                    if (rsp_pack.is_valid()) {
                        co_return rsp_pack;
                    }
                }
                
                break;
            }
        }
        
        co_return packet{};
    }
    
    asio::awaitable<packet> await_request(uint32_t cmd) {
//        co_await schedule(io_context.get_executor());
        
        auto it = received_request_channels.find(cmd);
        if (it == received_request_channels.end()) {
            received_request_channels.emplace(cmd, std::make_unique<connection::packet_channel>(io_context, connection::received_packet_channel_size));
        }
        
        auto [e, pack] = co_await received_request_channels[cmd]->async_receive(use_nothrow_awaitable);
        co_return e ? packet{} : pack;
    }
private:
    asio::awaitable<packet> start() {
        for (;;) {
            auto [e, socket] = co_await acceptor.async_accept(use_nothrow_awaitable);
            if (e) {
                continue;
            }
            asio::co_spawn(io_context, handle_connection(std::move(socket)), asio::detached);
        }
    }
    
    asio::awaitable<void> handle_connection(asio::ip::tcp::socket socket) {
        connections.push_back(std::make_unique<connection>(std::move(socket)));
        auto& conn = connections.back();
        
        for (;;) {
            auto result = co_await(conn->request_received() || conn->connection_disconnected());
            
            if (result.index() == 0) {
                packet pack(std::get<0>(std::move(result)));
                auto it = received_request_channels.find(pack.packet_cmd());
                if (it != received_request_channels.end()) {
                    co_await it->second->async_send(asio::error_code{}, pack, use_nothrow_awaitable);
                }
            } else {
                std::cout << "connection_disconnected" << std::endl;
                break;
            }
        }
        
        co_await conn->stop();
        
        for (auto it = connections.begin(); it != connections.end(); ++it) {
            if (it->get() == conn.get()) {
                connections.erase(it);
                break;
            }
        }
    }
    
private:
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    
    asio::ip::tcp::acceptor acceptor;
    
    received_request_channel_map received_request_channels;
    connection_list connections;
};

}
