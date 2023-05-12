#pragma once
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <list>
#include <future>
#include <cassert>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/as_tuple.hpp>

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/detail/debug_helper.hpp>

namespace asyncmsg {

class server {
    using connection_list = std::list<std::unique_ptr<connection>>;
    enum class object_state {running,stopping,stopped};
public:
    server(uint16_t port_)
    : work_guard(io_context.get_executor()), acceptor(io_context.get_executor(), {asio::ip::tcp::v4(), port_}) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        
        asio::co_spawn(io_context, start(), asio::detached);
    }
    
    ~server() {
        std::cout << detail::get_time_string() << ", ~server bedin" << std::endl;
        asio::co_spawn(io_context, stop(), asio::detached);
        
        if (io_thread.joinable()) {
            io_thread.join();
        }
        std::cout << detail::get_time_string() << ", ~server end" << std::endl;
    }
    
    asio::awaitable<void> stop() {
        auto task = [this]() -> asio::awaitable<void> {
            if (state == object_state::stopped) {
                co_return;
            }
            
            if (state == object_state::stopping) {
                co_await on_stopped.second.async_wait(use_nothrow_awaitable);
                co_return;
            }
            
            state = object_state::stopping;
            
            asio::error_code ec;
            acceptor.cancel(ec);
            
            for (auto& chan : received_request_channels) {
                chan.second->cancel();
            }
            
            for (auto& conn : connections) {
                co_await conn->stop();
            }
            
            std::cout << detail::get_time_string() << ", work_guard.reset" << std::endl;
            work_guard.reset();
            
            state = object_state::stopped;
            on_stopped.first.send();
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(), asio::use_awaitable);
    }
    
    asio::awaitable<void> send_packet(packet pack) {
        auto task = [this](packet pack) -> asio::awaitable<void> {
            if (!can_work()) {
                co_return;
            }
            
            for (auto& conn : connections) {
                if (conn->get_device_id() == pack.packet_device_id()) {
                    co_await conn->send_packet(pack);
                }
            }
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(pack), asio::use_awaitable);
    }
    
    asio::awaitable<packet> send_packet(packet pack, uint32_t timeout_seconds, uint32_t max_tries) {
        auto task = [this](packet pack, uint32_t timeout_seconds, uint32_t max_tries) -> asio::awaitable<packet> {
            if (!can_work()) {
                co_return packet{};
            }
            
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
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(pack, timeout_seconds, max_tries), asio::use_awaitable);
    }
    
    asio::awaitable<packet> await_request(uint32_t cmd) {
        auto task = [this](uint32_t cmd) -> asio::awaitable<packet> {
            if (!can_work()) {
                co_return packet{};
            }

            auto it = received_request_channels.find(cmd);
            if (it == received_request_channels.end()) {
                received_request_channels.emplace(cmd, std::make_unique<packet_channel>(io_context, received_packet_channel_size));
            }
            
            auto [e, pack] = co_await received_request_channels[cmd]->async_receive(use_nothrow_awaitable);
            co_return e ? packet{} : pack;
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(cmd), asio::use_awaitable);
    }
private:
    bool can_work() {
        return state == object_state::running;
    }
    
    asio::awaitable<packet> start() {
        while (can_work()) {
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
                std::cout << detail::get_time_string() << ", connection_disconnected" << std::endl;
                break;
            }
        }
        
        co_await conn->stop();
        connections.remove(conn);
    }
private:
    object_state state{object_state::running};
    
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    
    asio::ip::tcp::acceptor acceptor;
    
    received_request_channel_map received_request_channels;
    connection_list connections;
    
    std::pair<oneshot::sender<void>, oneshot::receiver<void>> on_stopped;
};

}
