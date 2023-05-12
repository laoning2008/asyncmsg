#pragma once
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <future>
#include <list>

#include <asio/ip/tcp.hpp>
#include <asio/detached.hpp>
#include <asio/connect.hpp>
#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/as_tuple.hpp>

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/detail/debug_helper.hpp>


namespace asyncmsg {

class client {
    using list_timer = std::list<std::unique_ptr<asio::steady_timer>>;
    constexpr static uint32_t reconnect_interval_seconds = 3;
    enum class object_state {running,stopping,stopped};
public:
    client(std::string host, const uint16_t port, std::string device_id_)
    : work_guard(io_context.get_executor()), device_id(std::move(device_id_))
    , server_host(std::move(host)), server_port(port)
    , connect_timer(io_context.get_executor())
    , on_stopped(oneshot::create<void>()) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        asio::co_spawn(io_context, start(), asio::detached);
    }

    ~client() {
        std::cout << detail::get_time_string() << ", ~clien bedin" << std::endl;
        asio::co_spawn(io_context, stop(), asio::detached);
        
        if (io_thread.joinable()) {
            io_thread.join();
        }
        std::cout << detail::get_time_string() << ", ~client end" << std::endl;
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
            
            connect_timer.cancel();
            
            for (auto& timer : pending_send_timers) {
                timer->cancel();
            }
            
            for (auto& chan : received_request_channels) {
                chan.second->cancel();
            }
            
            if (conn) {
                co_await conn->stop();
            }
            
            std::cout << detail::get_time_string() << ", set conn = nullptr" << std::endl;
            conn = nullptr;
            
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
            
            if (!conn) {
                std::cout << detail::get_time_string() << ", send_packet--conn==nullptr" << std::endl;
                co_return;
            }
            
            co_return co_await conn->send_packet(pack);
        };

        co_return co_await asio::co_spawn(io_context.get_executor(), task(pack), asio::use_awaitable);
    }

    asio::awaitable<packet> send_packet(packet pack, uint32_t timeout_seconds, uint32_t max_tries) {
        auto task = [this](packet pack, uint32_t timeout_seconds, uint32_t max_tries) -> asio::awaitable<packet> {
            if (!can_work()) {
                co_return packet{};
            }
            
            pending_send_timers.push_back(std::make_unique<asio::steady_timer>(io_context.get_executor()));
            auto& timer = pending_send_timers.back();
            
            for (auto i = 0; i < max_tries; ++i) {
                if (!can_work()) {
                    break;
                }
                
                if (!conn) {
                    std::cout << detail::get_time_string() << ", send_packet--conn==nullptr" << std::endl;
                    timer->expires_after(std::chrono::seconds(timeout_seconds));
                    co_await timer->async_wait(use_nothrow_awaitable);
                    continue;
                }
                                
                auto rsp_pack = co_await conn->send_packet(pack, timeout_seconds);
                
                if (rsp_pack.is_valid()) {
                    co_return rsp_pack;
                }
            }
            
            pending_send_timers.remove(timer);
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
                received_request_channels[cmd] = std::make_unique<packet_channel>(io_context, received_packet_channel_size);
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
    
    asio::awaitable<void> start() {
        while (can_work()) {
            co_await connect();
            
            if (!conn) {
                std::cout << detail::get_time_string() << ", conn == nullptr" << std::endl;
                connect_timer.expires_after(std::chrono::seconds(reconnect_interval_seconds));
                co_await connect_timer.async_wait(use_nothrow_awaitable);
                std::cout << detail::get_time_string() << ", connect async_wait end" << std::endl;
                continue;
            }
            
            for (;;) {
                auto result = co_await(conn->request_received() || conn->connection_disconnected());
                
                if (result.index() == 0) {
                    packet pack(std::get<0>(result));
                    auto it = received_request_channels.find(pack.packet_cmd());
                    if (it != received_request_channels.end()) {
                        co_await it->second->async_send(asio::error_code{}, pack, use_nothrow_awaitable);
                    }
                } else {
                    std::cout << detail::get_time_string() << ", stop conn begin2" << std::endl;
                    co_await conn->stop();
                    std::cout << detail::get_time_string() << ", stop conn end2" << std::endl;
                    conn = nullptr;
                    break;
                }
            }
        }
        
        std::cout << detail::get_time_string() << ", start() exit" << std::endl;
    }
    
    asio::awaitable<void> connect() {
        asio::ip::tcp::resolver resolver(io_context);
        auto [e_resolver, endpoints] = co_await resolver.async_resolve(server_host, std::to_string(server_port), use_nothrow_awaitable);
        if (e_resolver) {
            co_return;
        }

        asio::ip::tcp::socket socket(io_context);
        
        std::cout << detail::get_time_string() << ", async_connect begin" << std::endl;
        auto [e_connect, endpoint] = co_await asio::async_connect(socket, endpoints, use_nothrow_awaitable);
        std::cout << detail::get_time_string() << ", async_connect end, e = " << e_connect.message() << std::endl;
        if (!e_connect) {
            conn = std::make_unique<connection>(std::move(socket), device_id);
        }
    }
private:
    object_state state{object_state::running};
    
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    std::string device_id;
    std::string server_host;
    uint16_t server_port;

    std::unique_ptr<connection> conn;
    received_request_channel_map received_request_channels;
    
    asio::steady_timer connect_timer;
    
    list_timer pending_send_timers;
    
    std::pair<oneshot::sender<void>, oneshot::receiver<void>> on_stopped;
};

}
