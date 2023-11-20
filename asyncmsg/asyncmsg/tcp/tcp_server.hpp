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

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asyncmsg/base/coroutine_util.hpp>
#include <asyncmsg/base/async_event.hpp>


using namespace std::placeholders;

namespace asyncmsg { namespace tcp {

class tcp_server final {
public:
    tcp_server(uint16_t port_)
    : work_guard(io_context.get_executor())
    , stop_signal(io_context.get_executor())
    , acceptor(io_context.get_executor(), {asio::ip::tcp::v4(), port_}) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        
        start();
    }
    
    ~tcp_server() {
        base::print_log("~server begin");

        asio::post(io_context, [this]() {
            stop_signal.raise();
            
            asio::error_code ec;
            acceptor.cancel(ec);
            
            for (auto& chan : received_request_channels) {
                chan.second->cancel();
            }
            
            connection_map.clear();
            connections.clear();

            work_guard.reset();
        });
        
        if (io_thread.joinable()) {
            io_thread.join();
        }
        base::print_log("~server end");
    }

    asio::awaitable<void> send_packet(packet pack) {
        auto task = [&]() -> asio::awaitable<void> {
            try {
                auto it_conn = connection_map.find(pack.device_id());
                if (it_conn != connection_map.end()) {
                    co_await(it_conn->second->send_packet(pack) || stop_signal.async_wait());
                }
            } catch (std::exception& e) {
            }
        };

        co_await base::do_context_aware_task<void>(task, io_context.get_executor());
    }
    
    asio::awaitable<std::optional<packet>> send_packet_and_wait_rsp(packet pack, uint32_t timeout_seconds = detail::default_timeout, uint32_t max_tries = detail::default_tries) {
        auto task = [&]() -> asio::awaitable<std::optional<packet>> {
            std::optional<packet> rsp_packet = std::nullopt;
            do {
                for (auto i = 0; i <= max_tries; ++i) {
                    try {
                        auto it_conn = connection_map.find(pack.device_id());
                        if (it_conn == connection_map.end()) {
                            break;
                        }
                        
                        auto send_result = co_await(it_conn->second->send_packet(pack, timeout_seconds) || stop_signal.async_wait());
                        if (send_result.index() == 1) {
                            base::print_log("send_packet_and_wait_rsp----recv stop signal");
                            break;
                        }
                        
                        auto packet_opt = std::get<0>(send_result);
                        if (rsp_packet != std::nullopt) {
                            rsp_packet = packet_opt.value();
                            break;
                        }
                    } catch (std::exception& e) {
                    }
                }
            } while (0);

            co_return rsp_packet;
        };

        co_return co_await base::do_context_aware_task<std::optional<packet>>(task, io_context.get_executor());
    }
    
    asio::awaitable<std::optional<packet>> async_wait_request(uint32_t cmd) {
        auto task = [&]() -> asio::awaitable<std::optional<packet>> {
            std::optional<packet> request_packet = std::nullopt;
            try {
                auto it = received_request_channels.find(cmd);
                if (it == received_request_channels.end()) {
                    received_request_channels[cmd] = std::make_unique<detail::packet_channel>(io_context, detail::received_packet_channel_size);
                }
                
                auto wait_result = co_await(received_request_channels[cmd]->async_receive(asio::use_awaitable) || stop_signal.async_wait());
                if (wait_result.index() == 0) {
                    request_packet = std::get<0>(wait_result);;
                } else {
                    base::print_log("async_wait_request----recv stop signal");
                }
                
            } catch (std::exception& e) {
            }
            
            co_return request_packet;
        };

        co_return co_await base::do_context_aware_task<std::optional<packet>>(task, io_context.get_executor());
    }
private:
    void start() {
        asio::co_spawn(io_context, [this]() -> asio::awaitable<void> {
            auto result = co_await(accept() || stop_signal.async_wait());
            if (result.index() == 1) {
                base::print_log("start----recv stop signal");
            }
        }, asio::detached);
    }
    
    asio::awaitable<void> accept() {
        try {
            for (;;) {
                auto socket = co_await acceptor.async_accept(asio::use_awaitable);
                auto conn = std::make_shared<detail::connection>(io_context.get_executor(), std::move(socket)
                                                         , std::bind(&tcp_server::on_disconnected, this, _1)
                                                         , std::bind(&tcp_server::on_got_device_id, this, _1, _2)
                                                         , std::bind(&tcp_server::on_receive_request, this, _1, _2));
                connections.push_back(conn);
            }
        } catch (std::exception& e) {
        }
        
        base::print_log("tcp server accept() end");
    }
    
    void on_disconnected(detail::connection* conn) {
        connection_map.erase(conn->device_id());
        std::remove_if(connections.begin(), connections.end(), [conn](const std::shared_ptr<detail::connection>& it) {
            return it.get() == conn;
        });
    }
    
    void on_got_device_id(detail::connection* conn, const std::string& device_id) {
        auto it = connection_map.find(device_id);
        if (it == connection_map.end()) {
            connection_map[device_id] = conn;
        }
    }
    
    void on_receive_request(detail::connection* connection, packet packet) {
        auto buffer_pack_success = received_request_channels[packet.cmd()]->try_send(asio::error_code{}, std::move(packet));
        if (!buffer_pack_success) {
            base::print_log("buffer pack failed");
        }
        
//        asio::co_spawn(io_context, [this, packet = std::move(packet)]() -> asio::awaitable<void> {
//            co_await(received_request_channels[packet.cmd()]->async_send(asio::error_code{}, std::move(packet), use_nothrow_awaitable)
//                     || stop_signal.async_wait());
//        }, asio::detached);
    }
private:
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    
    asio::ip::tcp::acceptor acceptor;
    
    detail::received_request_channel_map received_request_channels;
    
    std::unordered_map<std::string, detail::connection*> connection_map;
    std::list<std::shared_ptr<detail::connection>> connections;
    
    base::async_event stop_signal;
};

}}
