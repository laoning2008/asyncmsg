#pragma once
#include <exception>
#include <unordered_map>
#include <chrono>

#include <async_simple/coro/Collect.h>
#include <async_simple/coro/Sleep.h>
#include <async_simple/coro/ConditionVariable.h>

#include <asyncmsg/detail/asio_coro_util.hpp>
#include <asyncmsg/detail/io_buffer.hpp>
#include <asyncmsg/detail/asio_task_runner.hpp>
#include <asyncmsg/detail/async_container.hpp>

namespace asyncmsg {

class connection {
    constexpr static uint32_t recv_buf_size = 128*1024;
    constexpr static uint32_t active_connection_lifetime_seconds = 60;
    struct request_info {
        std::weak_ptr<async_value<std::shared_ptr<packet>>> async_v;
        std::chrono::steady_clock::time_point begin_time;
        uint32_t timeout_seconds = 0;
    };
    using async_value_map_t = std::unordered_map<uint64_t, request_info>;
public:
    connection(asio::io_context& io_context_, asio::ip::tcp::socket socket_, asyncmsg::AsioExecutor& schedule_, std::string device_id_ = {})
    : io_context(io_context_), socket(std::move(socket_)), device_id(std::move(device_id_)), last_recv_time(std::chrono::steady_clock::now()) {
        start().via(&schedule_).detach();
    }
    
    ~connection() {
        stopped = true;
        
        asio::error_code ec;
        socket.close(ec);
        
        for (auto& req : rsp_map) {
            auto req_async_v = req.second.async_v.lock();
            if (req_async_v) {
                req_async_v->send();
            }
        }
    }

    async_simple::coro::Lazy<void> send_packet(std::shared_ptr<packet> pack, uint32_t timeout_seconds, std::weak_ptr<async_value<std::shared_ptr<packet>>> responser) {
        auto buf = asio::buffer(pack->packet_data(), pack->packet_data_length());
        auto [err, writen] = co_await async_write(socket, buf);
        if (stopped || err) {
            auto shared_responser = responser.lock();
            if (shared_responser) {
                shared_responser->send();
                co_return;
            }
        }

        rsp_map[pack->cmd << 31 | pack->seq] = {responser, std::chrono::steady_clock::now(), timeout_seconds};
    }
    
    async_simple::coro::Lazy<std::shared_ptr<packet>> request_received() {
        co_return co_await received_request_packets.consume();
    }
    
    async_simple::coro::Lazy<void> connection_disconnected() {
        co_await connection_disconnected_notifier.wait();
        connection_disconnected_notifier.reset();
    }
    
    std::string get_device_id() {
        return device_id;
    }
private:
    async_simple::coro::Lazy<void> start() {
        bev::io_buffer recv_buffer(recv_buf_size);
        for (;;) {            
            auto result = co_await async_simple::coro::collectAny(receive_packet(recv_buffer), async_simple::coro::sleep(std::chrono::seconds(1)));
            
            if (stopped) {
                std::cout << "stopped" << std::endl;
                break;
            }
            
            if (result.index() == 0) {
                auto recv_success = std::get<async_simple::Try<bool>>(std::move(result));
                
                if (recv_success.hasError()) {
                    std::cout << "recv error, disconnect" << std::endl;
                    connection_disconnected_notifier.notify();
                    break;
                }
                
                if (!recv_success.value()) {
                    std::cout << "recv error, disconnect" << std::endl;
                    connection_disconnected_notifier.notify();
                    break;
                }
            } else if (result.index() == 1) {
//                auto elapse_conn = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_recv_time).count();
//                if (elapse_conn > active_connection_lifetime_seconds) {
//                    std::cout << "inactive connection, disconnect" << std::endl;
//                    connection_disconnected_notifier.notify();
//                    break;
//                }
                
                for (auto it = rsp_map.begin(); it != rsp_map.end(); ) {
                    auto elapse = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - it->second.begin_time).count();
                    if (elapse > it->second.timeout_seconds) {
                        it = rsp_map.erase(it);
                    } else {
                        ++it;
                    }
                }
            } else {
                break;
            }
        }
    }
    
    async_simple::coro::Lazy<bool> receive_packet(bev::io_buffer& recv_buffer) {
        auto size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
        if (size_to_read <= 0) {
            if (!process_packet(recv_buffer)) {
                co_return false;
            }
        }
        
        size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
        if (size_to_read <= 0) {
            co_return false;
        }

        auto buf = recv_buffer.prepare(size_to_read);
        auto [err, read_size] = co_await async_read_some(socket, asio::buffer(buf.data, buf.size));
        
        if (stopped || err) {
            co_return false;
        }
        
        if (read_size > 0) {
            recv_buffer.commit(read_size);
            if (!process_packet(recv_buffer)) {
                co_return false;
            }
        }
        
        co_return true;
    }
    
    bool process_packet(bev::io_buffer& recv_buffer) {
        bool success = true;
        for(;;) {
            size_t consume_len = 0;
            auto pack = packet::parse_packet(recv_buffer.read_head(), recv_buffer.size(), consume_len);
            recv_buffer.consume(consume_len);
            
            if (!pack) {
                break;//finished
            }
            
            if (pack->device_id.empty()) {
                continue;// ignore it
            }
            
            if (device_id.empty()) {
                device_id = pack->device_id;
            }
            
            if (device_id != pack->device_id) {
                success = false;
                break;
            }
            
            last_recv_time = std::chrono::steady_clock::now();
            
            if (pack->rsp) {
                auto pack_id = pack->cmd << 31 | pack->seq;
                auto it = rsp_map.find(pack_id);
                if (it != rsp_map.end()) {
                    auto async_value = it->second.async_v.lock();
                    if (async_value) {
                        async_value->send(pack);
                    }
                }
            } else {
                std::cout << "recv pack, cmd = " << pack->cmd << ", seq = " << pack->seq << std::endl;
                received_request_packets.product(pack);
            }
        }
        
        return success;
    }
    
private:
    volatile bool stopped{false};
    asio::io_context& io_context;
    asio::ip::tcp::socket socket;
    std::chrono::steady_clock::time_point last_recv_time;
    std::string device_id;
    
    async_simple::coro::Notifier connection_disconnected_notifier;
    async_value_map_t rsp_map;
    async_request_producer_consumer_simple<std::shared_ptr<packet>> received_request_packets;
};

}
