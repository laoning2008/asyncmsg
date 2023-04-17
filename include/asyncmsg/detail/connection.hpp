#pragma once
#include <exception>
#include <unordered_map>
#include <chrono>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/channel.hpp>


//#include <asyncmsg/detail/asio_coro_util.hpp>
#include <asyncmsg/detail/io_buffer.hpp>
#include <asyncmsg/detail/asio_task_runner.hpp>

//#include <asyncmsg/async_event.hpp>
//#include <asyncmsg/async_mutex.hpp>
#include <asyncmsg/async_schedule.hpp>
#include <asyncmsg/oneshot.hpp>

using namespace asio::experimental::awaitable_operators;

namespace asyncmsg {

class connection {
    constexpr static uint32_t recv_buf_size = 128*1024;
    constexpr static uint32_t active_connection_lifetime_seconds = 60;
    using request_map_t = std::unordered_map<uint64_t, oneshot::sender<asyncmsg::packet>>;
    using packet_channel = asio::experimental::channel<void(asio::error_code, asyncmsg::packet)>;
    constexpr static uint32_t received_packet_channel_size = 64;
public:
    connection(asio::ip::tcp::socket socket_, std::string device_id_ = {})
    : socket(std::move(socket_))
    , device_id(std::move(device_id_))
    , last_recv_time(std::chrono::steady_clock::now())
    , received_packet_channel(socket.get_executor(), received_packet_channel_size) {
        start();
    }
    
    ~connection() {
        asio::error_code ec;
        socket.close(ec);
    }

    asio::awaitable<void> watchdog(std::chrono::steady_clock::time_point& deadline) {
      asio::steady_timer timer(co_await asio::this_coro::executor);
      auto now = std::chrono::steady_clock::now();
      while (deadline > now) {
        timer.expires_at(deadline);
        co_await timer.async_wait(asio::use_awaitable);
        now = std::chrono::steady_clock::now();
      }
      throw std::system_error(std::make_error_code(std::errc::timed_out));
    }
    
    asio::awaitable<std::pair<bool, packet>> send_packet(packet pack, uint32_t timeout_seconds, uint32_t tries) {
        auto buf = asio::buffer(pack.packet_data().buf(), pack.packet_data().len());
        auto [s, r] = oneshot::create<packet>();
        requests[pack.packet_cmd() << 31 | pack.packet_seq()] = std::move(s);
        
        for (auto i = 0; i < tries; ++i) {
            auto writen = co_await async_write(socket, buf, asio::use_awaitable);
            auto timeout = std::chrono::steady_clock::now();
            timeout += std::chrono::duration<uint64_t>(timeout_seconds);
            auto result = co_await(r.async_wait(asio::use_awaitable) || watchdog(timeout));
            if (result.index() == 1) {
                continue;
            }
            
            packet p = r.get();
            co_return std::make_pair<bool, packet>(true, std::move(p));
        }
        
        co_return std::make_pair<bool, packet>(false, {});
    }
    
    asio::awaitable<packet> request_received() {
        co_return co_await received_packet_channel.async_receive(asio::use_awaitable);
    }
    
    asio::awaitable<void> connection_disconnected() {
//        co_await on_disconnected.wait();
    }
    
    std::string get_device_id() {
        return device_id;
    }
private:
    void start() {
        co_spawn(socket.get_executor(), receive_packet(), asio::detached);
    }
    
    asio::awaitable<void> receive_packet() {
        bev::io_buffer recv_buffer(recv_buf_size);
        for (;;) {
            auto size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                if (!process_packet(recv_buffer)) {
//                    on_disconnected.set();
                    break;
                }
            }
            
            size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
//                on_disconnected.set();
                break;
            }

            auto buf = recv_buffer.prepare(size_to_read);
            auto read_size = co_await asio::async_read(socket, asio::buffer(buf.data, buf.size), asio::use_awaitable);
            
            if (read_size > 0) {
                recv_buffer.commit(read_size);
                if (!process_packet(recv_buffer)) {
//                    on_disconnected.set();
                    break;
                }
            }
        }
    }
    
    bool process_packet(bev::io_buffer& recv_buffer) {
        bool success = true;
        for(;;) {
            size_t consume_len = 0;
            auto pack = asyncmsg::parse_packet(recv_buffer.read_head(), recv_buffer.size(), consume_len);
            recv_buffer.consume(consume_len);
            
            if (!pack) {
                break;//finished
            }
            
            if (pack->packet_device_id().empty()) {
                continue;// ignore it
            }
            
            if (device_id.empty()) {
                device_id = pack->packet_device_id();
            }
            
            if (device_id != pack->packet_device_id()) {
                success = false;
                break;
            }
            
            last_recv_time = std::chrono::steady_clock::now();
            
            if (pack->is_response()) {
                auto pack_id = pack->packet_cmd() << 31 | pack->packet_seq();
                auto it = requests.find(pack_id);
                if (it != requests.end()) {
                    it->second.send(*pack.get());
                }
            } else {
                std::cout << "recv pack, cmd = " << pack->packet_cmd() << ", seq = " << pack->packet_seq() << std::endl;
                co_spawn(socket.get_executor(), received_packet_channel.async_send({}, *pack.get(), asio::use_awaitable), asio::detached);
            }
        }
        
        return success;
    }
private:
    asio::ip::tcp::socket socket;
    std::chrono::steady_clock::time_point last_recv_time;
    std::string device_id;
    
//    asio::awaitable_ext::async_event on_disconnected;
    request_map_t requests;
    packet_channel received_packet_channel;
//    async_request_producer_consumer_simple<std::shared_ptr<packet>> received_request_packets;
};

}
