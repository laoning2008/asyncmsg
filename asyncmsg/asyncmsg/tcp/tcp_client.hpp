#pragma once
#include <asyncmsg/base/config.hpp>
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <future>
#include <list>
#include <format>
#include <unordered_map>
#include <memory>
#include <optional>

#include <asio/ip/tcp.hpp>
#include <asio/detached.hpp>
#include <asio/connect.hpp>

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asio/experimental/channel.hpp>
#include <asyncmsg/base/coroutine_util.hpp>


using namespace std::placeholders;

namespace asyncmsg { namespace tcp {

class tcp_client final {
public:
    tcp_client(std::string host, const uint16_t port, std::string device_id_)
    : work_guard(io_context.get_executor())
    , device_id(std::move(device_id_))
    , stop_signal(io_context.get_executor(), std::chrono::steady_clock::time_point::max())
    , server_host(std::move(host)), server_port(port) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        start();
    }

    ~tcp_client() {
        base::print_log("~tcp_client bedin");
        asio::post(io_context, [this]() {
            stopped = true;
            stop_signal.cancel();
            conn = nullptr;
            for (auto& channel : received_request_channels) {
                channel.second->cancel();
            }
            work_guard.reset();
        });
        
        if (io_thread.joinable()) {
            io_thread.join();
        }
        base::print_log("~tcp_client end");
    }
    
    asio::awaitable<void> send_packet(packet& pack) {
        auto task = [&]() -> asio::awaitable<void> {
            if (reset_device_id_if_needed(pack) && conn != nullptr) {
                co_await conn->send_packet(pack);
            }
        };

        co_await base::do_context_aware_task<void>(task, io_context.get_executor());
    }

    asio::awaitable<std::optional<packet>> send_packet_and_wait_rsp(packet& pack, uint32_t timeout_seconds = detail::default_timeout, uint32_t max_tries = detail::default_tries) {
        auto task = [&]() -> asio::awaitable<std::optional<packet>> {
            std::optional<packet> rsp_packet = std::nullopt;
            asio::steady_timer wait_timer(io_context.get_executor());
            do {
                if (!reset_device_id_if_needed(pack)) {
                    break;
                }
                
                for (auto i = 0; i <= max_tries; ++i) {
                    try {
                        if (stopped) {
                            base::print_log("stopped");
                            break;
                        }
                        
                        if (conn == nullptr) {
                            base::print_log("wait connection");
                            wait_timer.expires_from_now(std::chrono::milliseconds(100));
                            co_await wait_timer.async_wait(asio::use_awaitable);
                            continue;
                        }
                        
                        auto packet_opt = co_await conn->send_packet_and_wait_rsp(pack, timeout_seconds);
                        
                        if (packet_opt != std::nullopt) {
                            rsp_packet = packet_opt.value();
                            break;
                        }
                    } catch (std::exception& e) {
                        base::print_log("send_packet_and_wait_rsp e = " + std::string(e.what()));
                    }
                }
            } while (0);
            
            co_return rsp_packet;
        };

        co_return co_await base::do_context_aware_task<std::optional<packet>>(task, io_context.get_executor());
    }

    asio::awaitable<packet> async_wait_request(uint32_t cmd) {
        auto task = [&]() -> asio::awaitable<packet> {
            auto it = received_request_channels.find(cmd);
            if (it == received_request_channels.end()) {
                received_request_channels[cmd] = std::make_unique<detail::packet_channel>(io_context, detail::received_packet_channel_size);
            }
            
            co_return co_await received_request_channels[cmd]->async_receive(asio::use_awaitable);
        };

        co_return co_await base::do_context_aware_task<packet>(task, io_context.get_executor());
    }
private:
    void start() {
        asio::co_spawn(io_context, [this]() -> asio::awaitable<void> {
            co_await(connect() || stop_signal.async_wait(asio::use_awaitable));
        }, asio::detached);
    }
    
    void on_disconnected(detail::connection* connection, const std::string& device_id) {
        conn = nullptr;
    }
    
    void on_got_device_id(detail::connection* conn, const std::string& device_id) {
    }
    
    void on_receive_request(detail::connection* connection, const std::string& device_id, packet packet) {
        received_request_channels[packet.cmd()]->try_send(asio::error_code{}, std::move(packet));
    }
    
    asio::awaitable<void> connect() {
        asio::steady_timer connect_timer(io_context.get_executor());
        
        for (;;) {
            try {
                if (conn == nullptr) {
                    asio::ip::tcp::resolver resolver(io_context);
                    auto endpoints = co_await resolver.async_resolve(server_host, std::to_string(server_port), asio::use_awaitable);
                    asio::ip::tcp::socket socket(io_context);
                    
                    base::print_log("async_connect begin");
                    co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
                    base::print_log("async_connect end");
                    
                    conn = std::make_shared<detail::connection>(io_context.get_executor(), std::move(socket)
                                                                , std::bind(&tcp_client::on_disconnected, this, _1, _2)
                                                                , std::bind(&tcp_client::on_got_device_id, this, _1, _2)
                                                                , std::bind(&tcp_client::on_receive_request, this, _1, _2, _3)
                                                                , device_id);
                }
            } catch (std::exception& e) {
                base::print_log(std::string("connect e = ") + e.what());
            }

            connect_timer.expires_from_now(std::chrono::milliseconds(100));
            co_await connect_timer.async_wait(use_nothrow_awaitable);
        }
        
        base::print_log("start() exit");
    }

    bool reset_device_id_if_needed(packet& pack) {
        if (pack.device_id().empty()) {
            pack.set_device_id(device_id);
            return true;
        }
        
        return (pack.device_id() == device_id);
    }
private:
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    
    std::string device_id;
    std::string server_host;
    uint16_t server_port;
    
    bool stopped = false;

    std::shared_ptr<detail::connection> conn;
    detail::received_request_channel_map received_request_channels;
    asio::steady_timer stop_signal;
};

}}
