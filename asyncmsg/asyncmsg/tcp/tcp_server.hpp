#pragma once
#include <exception>
#include <unordered_map>
#include <list>
#include <memory>
#include <chrono>

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asyncmsg/detail/coroutine_util.hpp>


using namespace std::placeholders;

namespace asyncmsg { namespace tcp {

class tcp_server final {
public:
    tcp_server(uint16_t port_)
    : work_guard(io_context.get_executor())
    , acceptor(io_context.get_executor(), {asio::ip::tcp::v4(), port_}) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        
        start();
    }
    
    ~tcp_server() {
        base::print_log("~server begin");

        asio::post(io_context, [this]() {
            stopped = true;
            acceptor.cancel();
            
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

    asio::awaitable<void> send_packet(packet& pack) {
        auto task = [&]() -> asio::awaitable<void> {
            if (stopped) {
                throw invalid_state_error{};
            }
            
            auto it_conn = connection_map.find(pack.device_id());
            if (it_conn != connection_map.end()) {
                auto conn = it_conn->second;
                co_await conn->send_packet(pack);
            }
        };

        co_await asyncmsg::detail::do_context_aware_task<void>(task, io_context.get_executor());
    }
    
    asio::awaitable<packet> send_packet_and_wait_rsp(packet& pack, uint32_t timeout_millliseconds = detail::default_timeout, uint32_t max_tries = detail::default_tries) {
        auto task = [&]() -> asio::awaitable<packet> {
            for (auto i = 0; i <= max_tries; ++i) {
                if (stopped) {
                    throw invalid_state_error{};
                }
                    
                auto it_conn = connection_map.find(pack.device_id());
                if (it_conn == connection_map.end()) {
                    throw connection_error{};
                }
                
                try {
                    auto conn = it_conn->second;
                    auto rsp = co_await conn->send_packet_and_wait_rsp(pack, timeout_millliseconds);
                    co_return rsp;
                } catch (std::exception& e) {
                    base::print_log("send_packet_and_wait_rsp e = " + std::string(e.what()));
                }
            }
            
            throw timeout_error{};
        };

        co_return co_await asyncmsg::detail::do_context_aware_task<packet>(task, io_context.get_executor());
    }
    
    asio::awaitable<packet> wait_request(uint32_t cmd) {
        auto task = [&]() -> asio::awaitable<packet> {
            if (stopped) {
                throw invalid_state_error{};
            }
            
            base::print_log("wait_request cmd = " + std::to_string(cmd));
            
            auto it = received_request_channels.find(cmd);
            if (it == received_request_channels.end()) {
                received_request_channels[cmd] = std::make_unique<detail::packet_channel>(io_context, detail::received_packet_channel_size);
            }
            
            co_return co_await received_request_channels[cmd]->async_receive(asio::use_awaitable);
        };

        auto exe = io_context.get_executor();
        co_return co_await asyncmsg::detail::do_context_aware_task<packet>(task, io_context.get_executor());
    }
private:
    void start() {
        asio::co_spawn(io_context, accept(), asio::detached);
    }
    
    asio::awaitable<void> accept() {
        for (;;) {
            auto socket = co_await acceptor.async_accept(asio::use_awaitable);
            auto conn = std::make_shared<detail::connection>(io_context.get_executor(), std::move(socket)
                                                     , std::bind(&tcp_server::on_disconnected, this, _1, _2)
                                                     , std::bind(&tcp_server::on_got_device_id, this, _1, _2)
                                                     , std::bind(&tcp_server::on_receive_request, this, _1, _2, _3));
            connections.push_back(conn);
        }
        
        base::print_log("tcp server accept() end");
    }
    
    void on_disconnected(detail::connection* conn, const std::string& device_id) {
        connection_map.erase(device_id);
        std::remove_if(connections.begin(), connections.end(), [conn](const std::shared_ptr<detail::connection>& it) {
            return it.get() == conn;
        });
    }
    
    void on_got_device_id(detail::connection* conn, const std::string& device_id) {
        for (auto it = connections.begin(); it != connections.end(); ++it) {
            if ((*it).get() == conn) {
                connection_map[device_id] = (*it);
                connections.erase(it);
                break;
            }
        }
    }
    
    void on_receive_request(detail::connection* connection, const std::string& device_id, packet packet) {
        auto it = received_request_channels.find(packet.cmd());
        if (it != received_request_channels.end()) {
            it->second->try_send(asio::error_code{}, std::move(packet));
        }
    }
private:
    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    
    asio::ip::tcp::acceptor acceptor;
    
    bool stopped = false;
    
    detail::received_request_channel_map received_request_channels;
    
    std::unordered_map<std::string, std::shared_ptr<detail::connection>> connection_map;
    std::list<std::shared_ptr<detail::connection>> connections;
};

}}
