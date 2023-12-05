#pragma once
#include <asyncmsg/detail/config.hpp>
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <list>
#include <memory>
#include <optional>

#include <asio/ip/tcp.hpp>
#include <asio/detached.hpp>
#include <asio/connect.hpp>
#include <asio/experimental/channel.hpp>

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/detail/coroutine_util.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asyncmsg/detail/lru.hpp>

using namespace std::placeholders;

namespace asyncmsg { namespace tcp {

class tcp_client final {
    constexpr static uint32_t recently_received_request_cap = 1000;
    
public:
    tcp_client(std::string host, const uint16_t port, std::string device_id_)
    : work_guard(io_context.get_executor())
    , device_id(std::move(device_id_))
    , recently_received_request(recently_received_request_cap)
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

        co_await asyncmsg::detail::do_context_aware_task<void>(task, io_context.get_executor());
    }

    asio::awaitable<packet> send_packet_and_wait_rsp(packet& pack, uint32_t timeout_millliseconds = detail::default_timeout) {
        auto task = [&]() -> asio::awaitable<packet> {
            if (stopped) {
                throw invalid_state_error{};
            }
            
            if (!reset_device_id_if_needed(pack)) {
                throw invalid_device_id_error{};
            }
            
            asio::steady_timer wait_timer(io_context.get_executor());
            auto begin = std::chrono::steady_clock::now();
            while (conn == nullptr) {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() > timeout_millliseconds / 2) {
                    break;
                }
                wait_timer.expires_from_now(std::chrono::milliseconds(100));
                co_await wait_timer.async_wait(asio::use_awaitable);
            }
            
            if (conn == nullptr) {
                throw connection_error{};
            }
            
            uint32_t elapse = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
            if (elapse >= timeout_millliseconds) {
                throw timeout_error{};
            }
                        
            co_return co_await conn->send_packet_and_wait_rsp(pack, timeout_millliseconds - elapse);
        };

        
        co_return co_await asyncmsg::detail::do_context_aware_task<packet>(task, io_context.get_executor());
    }

    asio::awaitable<packet> wait_request(uint32_t cmd) {
        auto task = [&]() -> asio::awaitable<packet> {
            auto it = received_request_channels.find(cmd);
            if (it == received_request_channels.end()) {
                received_request_channels[cmd] = std::make_unique<detail::packet_channel>(io_context, detail::received_packet_channel_size);
            }
            
            co_return co_await received_request_channels[cmd]->async_receive(asio::use_awaitable);
        };

        co_return co_await asyncmsg::detail::do_context_aware_task<packet>(task, io_context.get_executor());
    }
private:
    void start() {
        asio::co_spawn(io_context, [this]() -> asio::awaitable<void> {
            co_await(connect() || heartbeat() || stop_signal.async_wait(asio::use_awaitable));
        }, asio::detached);
    }
    
    asio::awaitable<void> heartbeat() {
        asio::steady_timer check_timer(io_context);
        while (!stopped) {
            if (conn) {
                auto heartbeat_pack = asyncmsg::tcp::build_req_packet(detail::heartbeat_cmd, nullptr, 0, device_id);
                co_await conn->send_packet(heartbeat_pack);
            }
            
            check_timer.expires_after(std::chrono::seconds(detail::active_connection_heartbeat_interval_seconds));
            co_await check_timer.async_wait(asio::use_awaitable);
        }
    }
    
    void on_disconnected(detail::connection* connection, const std::string& device_id) {
        conn = nullptr;
    }
    
    void on_got_device_id(detail::connection* conn, const std::string& device_id) {
    }
    
    void on_receive_request(detail::connection* connection, const std::string& device_id, packet pack) {
        connection->send_packet_detach(asyncmsg::tcp::build_rsp_packet(pack.cmd(), pack.seq(), 0, device_id, nullptr, 0));
        auto id = packet_id(pack);
        if (recently_received_request.exist(id)) {
            return;
        }
        
        recently_received_request.put(id);
        received_request_channels[pack.cmd()]->try_send(asio::error_code{}, std::move(pack));
    }
    
    asio::awaitable<void> connect() {
        asio::steady_timer connect_timer(io_context.get_executor());
        
        while (!stopped) {
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
    
    asyncmsg::detail::lru<uint64_t> recently_received_request;
    
    bool stopped = false;

    std::shared_ptr<detail::connection> conn;
    detail::received_request_channel_map received_request_channels;
    asio::steady_timer stop_signal;
};

}}
