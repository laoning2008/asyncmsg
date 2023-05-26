#pragma once
#include <exception>
#include <unordered_map>
#include <set>
#include <memory>
#include <chrono>
#include <list>
#include <future>
#include <cassert>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/as_tuple.hpp>

#include <asyncmsg/detail/tcp/connection.hpp>
#include <asyncmsg/base/debug_helper.hpp>

namespace asyncmsg { namespace tcp {

class tcp_server final {
    using connection_map = std::unordered_map<uint32_t, std::pair<std::unique_ptr<detail::connection>, std::unique_ptr<detail::signal_channel>>>;
    enum class object_state {running,stopping,stopped};
public:
    tcp_server(uint16_t port_)
    : work_guard(io_context.get_executor())
    , acceptor(io_context.get_executor(), {asio::ip::tcp::v4(), port_})
    , on_stopped(io_context.get_executor(), 1) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        
        asio::co_spawn(io_context, start(), asio::detached);
    }
    
    ~tcp_server() {
        base::print_log("~server begin");
        asio::co_spawn(io_context, stop(), asio::detached);
        
        if (io_thread.joinable()) {
            io_thread.join();
        }
        base::print_log("~server end");
    }
    
    asio::awaitable<void> stop() {
        auto task = [this]() -> asio::awaitable<void> {
            base::print_log("tcp server stop begin");
            if (state == object_state::stopped) {
                base::print_log("tcp server stopped");
                co_return;
            }
            
            if (state == object_state::stopping) {
                co_await on_stopped.async_receive(use_nothrow_awaitable);
                base::print_log("tcp server stopped2");
                co_return;
            }
            
            state = object_state::stopping;
            
            asio::error_code ec;
            acceptor.cancel(ec);
            
            for (auto& chan : received_request_channels) {
                chan.second->cancel();
            }
            
            for (auto& conn : connections) {
                co_await conn.second.second->async_send(asio::error_code{}, use_nothrow_awaitable);
            }
            
            work_guard.reset();
            
            state = object_state::stopped;
            co_await on_stopped.async_send(asio::error_code{}, use_nothrow_awaitable);
            base::print_log("tcp server stop end");
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(), asio::use_awaitable);
    }
    
    asio::awaitable<void> send_packet(packet pack) {
        auto task = [this](packet pack) -> asio::awaitable<void> {
            if (!can_work()) {
                co_return;
            }
            
            is_sending_without_try = true;
            for (auto& conn : connections) {
                if (conn.second.first->get_device_id() == pack.packet_device_id()) {
                    co_await conn.second.first->send_packet(pack);
                }
            }
            is_sending_without_try = false;
            remove_connections_safely();
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(pack), asio::use_awaitable);
    }
    
    asio::awaitable<packet> send_packet_with_retry(packet pack, uint32_t timeout_seconds = detail::default_timeout, uint32_t max_tries = detail::default_tries) {
        if (max_tries == 0) {
            max_tries = 1;
        }
        auto task = [this](packet pack, uint32_t timeout_seconds, uint32_t max_tries) -> asio::awaitable<packet> {
            if (!can_work()) {
                co_return packet{};
            }
            
            is_sending_with_try = true;
            for (auto& conn : connections) {
                if (conn.second.first->get_device_id() == pack.packet_device_id()) {
                    for (auto i = 0; i < max_tries; ++i) {
                        auto rsp_pack = co_await conn.second.first->send_packet(pack, timeout_seconds);
                        if (rsp_pack.is_valid()) {
                            is_sending_with_try = false;
                            remove_connections_safely();
                            co_return rsp_pack;
                        }
                    }
                    break;
                }
            }
            
            is_sending_with_try = false;
            remove_connections_safely();
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
                received_request_channels.emplace(cmd, std::make_unique<detail::packet_channel>(io_context, detail::received_packet_channel_size));
            }
            
            auto [e, pack] = co_await received_request_channels[cmd]->async_receive(use_nothrow_awaitable);
            co_return e ? packet{} : pack;
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(cmd), asio::use_awaitable);
    }
    
    asio::awaitable<void> cancel_await_request(uint32_t cmd) {
        auto task = [this](uint32_t cmd) -> asio::awaitable<void> {
            auto it = received_request_channels.find(cmd);
            if (it != received_request_channels.end()) {
                it->second->cancel();
            }
            co_return;
        };

        co_await asio::co_spawn(io_context.get_executor(), task(cmd), asio::use_awaitable);
    }
    
    asio::awaitable<void> cancel_all_await_request() {
        auto task = [this]() -> asio::awaitable<void> {
            for (auto& chan : received_request_channels) {
                chan.second->cancel();
            }
            co_return;
        };

        co_await asio::co_spawn(io_context.get_executor(), task(), asio::use_awaitable);
    }
private:
    bool can_work() {
        return state == object_state::running;
    }
    
    bool is_safe_to_remove_connection() {
        return !is_sending_with_try && !is_sending_without_try;
    }
    
    void remove_connections_safely() {
        if (!is_safe_to_remove_connection()) {
            return;
        }
        
        for (auto& id : to_remove_connections) {
            connections.erase(id);
        }
    }
    
    asio::awaitable<void> start() {
        while (can_work()) {
            auto [e, socket] = co_await acceptor.async_accept(use_nothrow_awaitable);
            if (e) {
                continue;
            }
            asio::co_spawn(io_context, handle_connection(std::move(socket)), asio::detached);
        }
        
        base::print_log("tcp server start() end");
    }
    
    asio::awaitable<void> handle_connection(asio::ip::tcp::socket socket) {
        auto cur_conn_id = ++connection_id;
        connections[cur_conn_id] = std::make_pair(std::make_unique<detail::connection>(io_context.get_executor(), std::move(socket)), std::make_unique<detail::signal_channel>(io_context.get_executor(), 1));
                
        while (can_work()) {
            auto result = co_await(connections[cur_conn_id].first->request_received() || connections[cur_conn_id].first->connection_disconnected() || connections[cur_conn_id].second->async_receive(use_nothrow_awaitable));
                                    
            if (result.index() == 0) {
                packet pack(std::get<0>(std::move(result)));
                if (!pack.is_valid()) {
                    base::print_log("rec invalid pack, exit connecion");
                    break;
                }
                                
                auto it = received_request_channels.find(pack.packet_cmd());
                if (it != received_request_channels.end()) {
                    co_await it->second->async_send(asio::error_code{}, pack, use_nothrow_awaitable);
                }
            } else {
                base::print_log("connection_disconnected");
                break;
            }
        }
        
        base::print_log("co_await conn->stop() begin");
        co_await connections[cur_conn_id].first->stop();
        if (is_safe_to_remove_connection()) {
            base::print_log("safe to remove connection now");
            connections.erase(cur_conn_id);
        } else {
            base::print_log("not safe to remove connection now");
            to_remove_connections.insert(cur_conn_id);
        }

        base::print_log("co_await conn->stop() end");
    }
private:
    object_state state{object_state::running};
    
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    
    asio::ip::tcp::acceptor acceptor;
    
    detail::received_request_channel_map received_request_channels;
    connection_map connections;
    
    detail::signal_channel on_stopped;
    std::atomic<uint32_t> connection_id{0};
    std::set<uint32_t> to_remove_connections;
    
    bool is_sending_without_try{false};
    bool is_sending_with_try{false};
};

}}
