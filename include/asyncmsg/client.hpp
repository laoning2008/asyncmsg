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

class client {
    constexpr static uint32_t heartbeat_interval_seconds = 20;

public:
    client(std::string host, const uint16_t port, std::string device_id_)
    : stopped(false), schedule(io_context), work_guard(io_context.get_executor()), device_id(std::move(device_id_)) {
        io_thread = std::thread([this]() {
            io_context.run();
        });

        start(host, port).via(&schedule).detach();
    }

    ~client() {
        stopped = true;

        auto destrust_task = [this]() -> async_simple::coro::Lazy<void> {
            conn = nullptr;
        };
        async_simple::coro::syncAwait(destrust_task().via(&schedule));


        io_context.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }
    }

    async_simple::coro::Lazy<std::shared_ptr<packet>> send_packet(std::shared_ptr<packet> pack, uint32_t try_timeout_seconds = 3, uint32_t max_tries = 3) {
        co_return co_await [this, pack, try_timeout_seconds, max_tries]() -> async_simple::coro::Lazy<std::shared_ptr<packet>> {
            if (conn == nullptr) {
                co_return nullptr;
            }

            for (int i = 0; i < max_tries; ++i) {
                auto responser = std::make_shared<async_value<std::shared_ptr<packet>>>();
                co_await conn->send_packet(pack, try_timeout_seconds, responser);
                
                auto rsp =  co_await responser->recv();
                
                if (stopped) {
                    co_return nullptr;
                }
                
                if (rsp) {
                    co_return rsp;
                }
            }
            
            co_return nullptr;
        } ().via(&schedule);
    }

    async_simple::coro::Lazy<std::shared_ptr<packet>> await_request(uint32_t cmd) {
        co_return co_await received_request_packets.consume(cmd).via(&schedule);
    }
private:
    async_simple::coro::Lazy<void> start(const std::string& host, const uint16_t port) {
        for (;;) {
            if (stopped) {
                break;
            }
            
            asio::ip::tcp::socket socket(io_context);
            auto ec = co_await async_connect(io_context, socket, host, std::to_string(port));
            
            if (stopped) {
                break;
            }
            
            if (ec) {
                co_await async_simple::coro::sleep(std::chrono::seconds(1));
                continue;
            }

            conn = std::make_unique<connection>(io_context, std::move(socket), schedule, device_id);
            std::chrono::steady_clock::time_point last_heartbeat_timepoit;
            
            for (;;) {
                auto result = co_await async_simple::coro::collectAny(conn->connection_disconnected(), conn->request_received(), async_simple::coro::sleep(std::chrono::seconds(2)));

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
                } else if (result.index() == 2) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapse = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_timepoit).count();
                    if (elapse > heartbeat_interval_seconds) {
                        last_heartbeat_timepoit = now;
                        heartbeat();
                    }
                } else {
                    break;
                }
            }
        }
    }
    
    void heartbeat() {
        if (!conn) {
            return;
        }
//        auto pack = packet::build_packet(0, false, device_id, nullptr, 0);
//        send_packet(pack).via(&schedule).detach();
    }
private:
    volatile bool stopped{false};

    asio::io_context io_context;
    std::thread io_thread;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    asyncmsg::AsioExecutor schedule;
    std::string device_id;

    std::unique_ptr<connection> conn;

    packet_producer_consumer_by_cmd received_request_packets;
};

}
