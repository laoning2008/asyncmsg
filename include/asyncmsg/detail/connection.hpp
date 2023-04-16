#pragma once
#include <exception>
#include <unordered_map>
#include <chrono>

#include <asio/awaitable.hpp>

#include <asyncmsg/detail/asio_coro_util.hpp>
#include <asyncmsg/detail/io_buffer.hpp>
#include <asyncmsg/detail/asio_task_runner.hpp>
#include <asyncmsg/detail/async_container.hpp>

#include <asyncmsg/async_event.hpp>
#include <asyncmsg/async_mutex.hpp>
#include <asyncmsg/async_schedule.hpp>
#include <asyncmsg/oneshot.hpp>

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
    connection(asio::ip::tcp::socket socket_, std::string device_id_ = {})
    : socket(std::move(socket_)), device_id(std::move(device_id_)), last_recv_time(std::chrono::steady_clock::now()) {
        start();
    }
    
    ~connection() {
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
    
    asio::awaitable<void> connection_disconnected() {
        co_await connection_disconnected_notifier.wait();
    }
    
    std::string get_device_id() {
        return device_id;
    }
private:
    void start() {
        co_spawn(socket.get_executor(), periodicly_check(), detached);
        co_spawn(socket.get_executor(), receive_packet(), detached);
    }
    
    asio::awaitable<void> periodicly_check() {
        asio::steady_timer timer(socket.get_executor());
        for (;;) {
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait(asio::use_nothrow_awaitable);
            
            for (auto it = rsp_map.begin(); it != rsp_map.end(); ) {
                auto elapse = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - it->second.begin_time).count();
                if (elapse > it->second.timeout_seconds) {
                    it = rsp_map.erase(it);
                } else {
                    ++it;
                }
            }
            
        }
    }
    
    asio::awaitable<void> receive_packet(bev::io_buffer& recv_buffer) {
        bev::io_buffer recv_buffer(recv_buf_size);
        for (;;) {
            auto size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                if (!process_packet(recv_buffer)) {
                    connection_disconnected_notifier.set();
                    break;
                }
            }
            
            size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                connection_disconnected_notifier.set();
                break;
            }

            auto buf = recv_buffer.prepare(size_to_read);
            auto [err, read_size] = asio::async_read(socket, asio::buffer(buf.data, buf.size));
            
            if (err) {
                on_disconnected.set();
                break;
            }
            
            if (read_size > 0) {
                recv_buffer.commit(read_size);
                if (!process_packet(recv_buffer)) {
                    on_disconnected.set();
                    break;
                }
            }
        }
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
    asio::ip::tcp::socket socket;
    std::chrono::steady_clock::time_point last_recv_time;
    std::string device_id;
    
    asio::awaitable_ext::async_event on_disconnected;
    async_value_map_t rsp_map;
//    async_request_producer_consumer_simple<std::shared_ptr<packet>> received_request_packets;
};

}
