#pragma once
//#include <asio/experimental/cancellation_condition.hpp>
//#define wait_for_one_success wait_for_one
//#include <asio/experimental/awaitable_operators.hpp>
//#undef wait_for_on_success

#include <exception>
#include <unordered_map>
#include <chrono>
#include <iostream>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/experimental/as_tuple.hpp>

#include <asyncmsg/detail/io_buffer.hpp>
#include <asyncmsg/detail/async_event.hpp>
#include <asyncmsg/detail/async_schedule.hpp>
#include <asyncmsg/detail/oneshot.hpp>
#include <asyncmsg/detail/packet.hpp>
#include <asyncmsg/detail/debug_helper.hpp>

using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);

namespace asyncmsg {

class connection {
    constexpr static uint32_t recv_buf_size = 128*1024;
    constexpr static uint32_t active_connection_lifetime_seconds = 30;
    constexpr static uint32_t active_connection_lifetime_check_interval_seconds = 1;

    using request_map_t = std::unordered_map<uint64_t, std::pair<oneshot::sender<asyncmsg::packet>, std::unique_ptr<asio::steady_timer>>>;
public:
    using packet_channel = asio::experimental::channel<void(asio::error_code, asyncmsg::packet)>;
    constexpr static uint32_t received_packet_channel_size = 64;
public:
    connection(asio::ip::tcp::socket socket_, std::string device_id_ = {})
    : socket(std::move(socket_))
    , device_id(std::move(device_id_))
    , last_recv_time(std::chrono::steady_clock::now())
    , received_request_channel(socket.get_executor(), received_packet_channel_size)
    , on_disconnected(oneshot::create<void>())
    , check_timer(socket.get_executor()) {
        start();
    }
    
    ~connection() {
        std::cout << detail::get_time_string() << ", ~connection" << std::endl;
    }
    
    asio::awaitable<void> stop() {
        std::cout << detail::get_time_string() << ", stop connection begin" << std::endl;
        close();
        
        asio::steady_timer wait_timer(co_await asio::this_coro::executor);
        while (processing || !requests.empty()) {
            std::cout << detail::get_time_string() << ", stop connection running = " << processing << ", requests.size =" << requests.size() << std::endl;
            wait_timer.expires_after(std::chrono::milliseconds(100));
            co_await wait_timer.async_wait(use_nothrow_awaitable);
        }
        std::cout << detail::get_time_string() << ", stop connection end" << std::endl;
    }
    
    asio::awaitable<void> send_packet(packet pack) {
        if (stopped) {
            co_return;
        }
        
        auto pack_buf = pack.packet_data();
        auto buf = asio::buffer(pack_buf.buf(), pack_buf.len());
        co_await asio::async_write(socket, buf, use_nothrow_awaitable);
    }
    
    asio::awaitable<packet> send_packet(packet pack, uint32_t timeout_seconds) {
        if (stopped) {
            co_return packet{};
        }
                
        auto pack_buf = pack.packet_data();
        auto buf = asio::buffer(pack_buf.buf(), pack_buf.len());
        auto [e_write, _] = co_await asio::async_write(socket, buf, use_nothrow_awaitable);
        
        if (e_write) {
            std::cout << detail::get_time_string() << "async_write err=" << e_write.message() << std::endl;
            close();
            co_return packet{};
        }
        
        auto [s, r] = oneshot::create<packet>();
        auto id = pack.packet_cmd() << 31 | pack.packet_seq();
        
        requests.emplace(id, std::make_pair(std::move(s), std::make_unique<asio::steady_timer>(co_await asio::this_coro::executor, std::chrono::seconds(timeout_seconds))));
        auto result = co_await(requests[id].second->async_wait(use_nothrow_awaitable) || r.async_wait(use_nothrow_awaitable));
        requests.erase(id);
        
        if (result.index() == 0) {
            std::cout << detail::get_time_string() << "send timeout" << std::endl;
            co_return packet{};
        }
        
        auto [e_wait] = std::get<1>(result);
        co_return e_wait ? packet{} : r.get();
    }
    
    asio::awaitable<packet> request_received() {
        auto [e, pack] = co_await received_request_channel.async_receive(use_nothrow_awaitable);
        co_return e ? packet{} : pack;
    }
    
    asio::awaitable<void> connection_disconnected() {
        co_await on_disconnected.second.async_wait(use_nothrow_awaitable);
    }
    
    std::string get_device_id() {
        return device_id;
    }
private:
    void start() {
        auto task = [this]() -> asio::awaitable<void> {
            processing = true;
            co_await(check() || receive_packet());
            processing = false;
            std::cout << detail::get_time_string() << ", set processing = false" << std::endl;
        };
        
        co_spawn(socket.get_executor(), task, asio::detached);
    }
    
    void close() {
        if (!stopped) {
            std::cout << detail::get_time_string() << ", close begin" << std::endl;
            
            stopped = true;
            
            check_timer.cancel();
            for (auto& request : requests) {
                request.second.second->cancel();
            }
            
            received_request_channel.cancel();
            
            asio::error_code ec;
            socket.close(ec);
            
            on_disconnected.first.send();
            
            std::cout << detail::get_time_string() << ", close end" << std::endl;
        }
    }
    
    asio::awaitable<void> check() {
        for (;;) {
            check_timer.expires_after(std::chrono::seconds(active_connection_lifetime_check_interval_seconds));
            co_await check_timer.async_wait(use_nothrow_awaitable);
            
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_recv_time).count() > active_connection_lifetime_seconds) {
                close();
                break;
            }
        }
    }
    
    asio::awaitable<void> receive_packet() {
        bev::io_buffer recv_buffer(recv_buf_size);
        for (;;) {
            auto size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                if (! co_await process_packet(recv_buffer)) {
                    close();
                    break;
                }
            }
            
            size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                close();
                break;
            }

            auto buf = recv_buffer.prepare(size_to_read);
            auto [e, read_size] = co_await socket.async_read_some(asio::buffer(buf.data, buf.size), use_nothrow_awaitable);
            
            if (e) {
                close();
                std::cout << detail::get_time_string() << ", read err = " << e.message() << std::endl;
                break;
            }
            
            if (read_size > 0) {
                recv_buffer.commit(read_size);
                if (! co_await process_packet(recv_buffer)) {
                    close();
                    break;
                }
            }
        }
    }
    
    asio::awaitable<bool> process_packet(bev::io_buffer& recv_buffer) {
        bool success = true;
        for(;;) {
            size_t consume_len = 0;
            auto pack = asyncmsg::parse_packet(recv_buffer.read_head(), recv_buffer.size(), consume_len);
            recv_buffer.consume(consume_len);
            
            if (!pack) {
                break;
            }
            
            if (pack->packet_device_id().empty()) {
                std::cout << detail::get_time_string() << ", recv pack, device id is empty" << std::endl;
                continue;
            }
            
            if (device_id.empty()) {
                device_id = pack->packet_device_id();
            } else if (device_id != pack->packet_device_id()) {
                success = false;
                break;
            }
            
            last_recv_time = std::chrono::steady_clock::now();
            
            auto pack_copy = *pack.get();
            
            if (pack_copy.is_response()) {
                auto pack_id = pack_copy.packet_cmd() << 31 | pack_copy.packet_seq();
                auto it = requests.find(pack_id);
                if (it != requests.end()) {
                    it->second.first.send(std::move(pack_copy));
                }
            } else {
                co_await received_request_channel.async_send({}, std::move(pack_copy), use_nothrow_awaitable);
            }
        }
        
        co_return success;
    }
private:
    volatile bool processing{false};
    volatile bool stopped{false};
    
    asio::ip::tcp::socket socket;
    std::string device_id;
    
    std::chrono::steady_clock::time_point last_recv_time;
    request_map_t requests;
    
    std::pair<oneshot::sender<void>, oneshot::receiver<void>> on_disconnected;
    packet_channel received_request_channel;
    
    asio::steady_timer check_timer;
};

}
