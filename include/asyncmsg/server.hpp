#pragma once
#include <exception>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <future>

#include <asyncmsg/detail/asio_coro_util.hpp>
#include <async_simple/coro/Collect.h>
#include <async_simple/coro/Sleep.h>

#include <asyncmsg/detail/connection.hpp>
#include <asyncmsg/detail/async_container.hpp>

namespace asyncmsg {

class server {
    using async_request_packet_queue = async_request_producer_consumer<std::pair<std::shared_ptr<packet>, uint32_t>, std::shared_ptr<packet>>;
    using pending_request_map_t = std::unordered_map<std::string, async_request_packet_queue>;

public:
    server(uint16_t port_)
    : stopped(false), work_guard(io_context.get_executor()), schedule(io_context.get_executor()), acceptor(io_context.get_executor(), {asio::ip::tcp::v4(), port_}) {
        io_thread = std::thread([this]() {
            io_context.run();
        });
        
        start().via(&schedule).detach();
    }
    
    ~server() {
//        stopped = true;
        
//        auto destrust_task = [this]() -> async_simple::coro::Lazy<void> {
//            asio::error_code ec;
//            acceptor.close(ec);
//        };
//        async_simple::coro::syncAwait(destrust_task().via(&schedule));
        
        
        io_context.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }
    }
    
    async_simple::coro::Lazy<std::shared_ptr<packet>> send_packet(std::shared_ptr<packet> pack, uint32_t timeout_seconds = 3, uint32_t max_tries = 3) {
       
    }
    
    async_simple::coro::Lazy<std::shared_ptr<packet>> await_request(uint32_t cmd) {
        co_return co_await received_request_packets.consume(cmd).via(&schedule);
    }
private:
    async_simple::coro::Lazy<void> start() {
        for (;;) {
            asio::ip::tcp::socket socket(io_context);

            auto error = co_await async_accept(acceptor, socket);
            
            std::this_thread::sleep_for(std::chrono::seconds(60));

            
            if (stopped) {
                break;
            }
            
            if (error) {
                continue;
            }
            
            handle_connection(std::move(socket)).via(&schedule).detach();
        }
    }
    
    async_simple::coro::Lazy<void> handle_connection(asio::ip::tcp::socket socket) {
        connection conn{io_context, std::move(socket), schedule};

        for (;;) {
            auto device_id = conn.get_device_id();
            if (device_id.empty()) {
                auto result = co_await async_simple::coro::collectAny(conn.connection_disconnected(), conn.request_received());

                if (stopped) {
                    break;
                }
                
                if (result.index() == 0) {
                    break;
                } else if (result.index() == 1) {
                    auto recv_packet_result = std::get<async_simple::Try<std::shared_ptr<packet>>>(std::move(result));
                    if (recv_packet_result.hasError() || !recv_packet_result.value()) {
                        break;
                    }
                    received_request_packets.product(recv_packet_result.value());
                } else {
                    break;
                }
            } else {
                auto result = co_await async_simple::coro::collectAny(conn.connection_disconnected(), conn.request_received(), sending_requests[device_id].consume_request());
                if (stopped) {
                    break;
                }
                
                if (result.index() == 0) {
                    std::cout << "connection disconnected " << std::endl;
                    break;
                } else if (result.index() == 1) {
                    auto recv_packet_result = std::get<async_simple::Try<std::shared_ptr<packet>>>(std::move(result));
                    if (recv_packet_result.hasError() || !recv_packet_result.value()) {
                        std::cout << "recv request error " << std::endl;
                        break;
                    }
                    std::cout << "recv pack" << std::endl;
                    received_request_packets.product(recv_packet_result.value());
                } else if (result.index() == 2) {
                    auto recv_packet_result = std::get<async_simple::Try<async_request_packet_queue::async_request_type>>(std::move(result));
                    if (recv_packet_result.hasError()) {
                        continue;;
                    }
                    
                    auto request = recv_packet_result.value();
                    std::cout << "send pack, cmd = " << request.requst_packet.first->cmd << ", seq = " << request.requst_packet.first->seq << std::endl;

                    conn.send_packet(request.requst_packet.first, request.requst_packet.second, request.responser).via(&schedule).detach();
                } else {
                    break;
                }
            }
        }
    }
    
private:
    volatile bool stopped{false};

    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    asyncmsg::AsioExecutor schedule;
    asio::ip::tcp::acceptor acceptor;
    
    packet_producer_consumer_by_cmd received_request_packets;
    pending_request_map_t sending_requests;
};

}
