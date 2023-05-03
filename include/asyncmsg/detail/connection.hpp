#pragma once
#include <exception>
#include <unordered_map>
#include <chrono>
#include <iostream>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>

#include <asyncmsg/detail/io_buffer.hpp>
#include <asyncmsg/detail/async_event.hpp>
#include <asyncmsg/detail/async_schedule.hpp>
#include <asyncmsg/detail/oneshot.hpp>
#include <asyncmsg/detail/packet.hpp>


using namespace asio::experimental::awaitable_operators;

namespace asyncmsg {

std::string get_time_string() {
    auto now = std::chrono::system_clock::now();
    //通过不同精度获取相差的毫秒数
    uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
        - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto time_tm = localtime(&tt);
    char strTime[25] = { 0 };
    sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d %03d", time_tm->tm_year + 1900,
        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
        time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds);
    return strTime;
}

class connection {
    constexpr static uint32_t recv_buf_size = 128*1024;
    constexpr static uint32_t active_connection_lifetime_seconds = 60;
    constexpr static uint32_t active_connection_lifetime_check_interval_seconds = 1;

    using request_map_t = std::unordered_map<uint64_t, oneshot::sender<asyncmsg::packet>>;
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
    , send_timer(socket.get_executor())
    , check_timer(socket.get_executor())
    {
        std::cout << "connection" << std::endl;
        start();
    }
    
    ~connection() {
        std::cout << "~connection" << std::endl;
    }

    void start() {
        auto task = [this]() -> asio::awaitable<void> {
            processing = true;
            
            try {
                co_await(check() || receive_packet());
            } catch (std::exception& e) {
                std::cout << "connection start exception: " << e.what() << std::endl;
            }
            
            std::cout << "set running = false" << std::endl;
            processing = false;
        };
        
        co_spawn(socket.get_executor(), task, asio::detached);
    }
    
    asio::awaitable<void> stop() {
        std::cout << get_time_string() << "stop connection begin" << std::endl;
        stopped = true;
        close();
        
        asio::steady_timer wait_timer(co_await asio::this_coro::executor);
        while (processing || !requests.empty()) {
            std::cout << get_time_string() << "stop connection running = " << processing << ", requests.size =" << requests.size() << std::endl;
            wait_timer.expires_after(std::chrono::milliseconds(500));
            co_await wait_timer.async_wait(asio::use_awaitable);
        }
        std::cout << get_time_string() << "stop connection end" << std::endl;
    }
    
    asio::awaitable<void> send_packet(packet pack) {
        if (stopped) {
            co_return;
        }
        
        try {
            auto pack_buf = pack.packet_data();
            auto buf = asio::buffer(pack_buf.buf(), pack_buf.len());
            co_await asio::async_write(socket, buf, asio::use_awaitable);
        } catch(std::exception& e) {
            std::cout << e.what() << std::endl;
            close(true);
        }
    }
    
    asio::awaitable<packet> send_packet(packet pack, uint32_t timeout_seconds) {
        if (stopped) {
            co_return packet{};
        }
        
        auto pack_buf = pack.packet_data();
        auto buf = asio::buffer(pack_buf.buf(), pack_buf.len());
        auto [s, r] = oneshot::create<packet>();
        auto id = pack.packet_cmd() << 31 | pack.packet_seq();
        requests.emplace(id, std::move(s));
        std::cout << get_time_string() << "start send packet, crc = " << (int)pack_buf.buf()[header_length-1] << std::endl;

        try {
            auto nwrite = co_await asio::async_write(socket, buf, asio::use_awaitable);
            std::cout << get_time_string() << "async_write, nwrite = " << nwrite << std::endl;

            send_timer.expires_after(std::chrono::seconds(timeout_seconds));
            auto result = co_await(send_timer.async_wait(asio::use_awaitable) || r.async_wait(asio::use_awaitable));
            requests.erase(id);
            
            std::cout << get_time_string() << "send packet result = " << result.index() << std::endl;
            if (result.index() == 0) {
                co_return packet{};
            } else {
                co_return r.get();
            }
        } catch(std::exception& e) {
            std::cout << e.what() << std::endl;
            requests.erase(id);
            close(true);
            std::cout << get_time_string() << "recturn empty packt after exception" << std::endl;
            co_return packet{};
        }
    }
    
    asio::awaitable<packet> request_received() {
        co_return co_await received_request_channel.async_receive(asio::use_awaitable);
    }
    
    asio::awaitable<void> connection_disconnected() {
        co_await on_disconnected.second.async_wait(asio::use_awaitable);
    }
    
    std::string get_device_id() {
        return device_id;
    }
private:
    void close(bool notify = false) {
        send_timer.cancel();
        check_timer.cancel();
        asio::error_code ec;
        socket.close(ec);
        if (notify) {
            std::cout << "on_disconnected.set" << std::endl;
            try {
                on_disconnected.first.send();
            } catch (std::exception& e) {
                std::cout << "on_disconnected.set exception = " <<e.what() << std::endl;
            }
        }
        
        std::cout << "close end" << std::endl;
    }
    
    asio::awaitable<void> check() {
        for (;;) {
            check_timer.expires_after(std::chrono::seconds(active_connection_lifetime_check_interval_seconds));
            co_await check_timer.async_wait(asio::use_awaitable);
            
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_recv_time).count() > active_connection_lifetime_seconds) {
                close(true);
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
                    close(true);
                    break;
                }
            }
            
            size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                close(true);
                break;
            }

            auto buf = recv_buffer.prepare(size_to_read);
            auto read_size = co_await socket.async_read_some(asio::buffer(buf.data, buf.size), asio::use_awaitable);
            
            if (read_size > 0) {
                recv_buffer.commit(read_size);
                if (! co_await process_packet(recv_buffer)) {
                    close(true);
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
//                std::cout << get_time_string() << "empty pack" << std::endl;
                break;//finished
            }
            
            if (pack->packet_device_id().empty()) {
                std::cout << get_time_string() << "recv pack, device id is empty" << std::endl;

                continue;// ignore it
            }
            
            if (device_id.empty()) {
                device_id = pack->packet_device_id();
            } else if (device_id != pack->packet_device_id()) {
                success = false;
                break;
            }
            
            last_recv_time = std::chrono::steady_clock::now();
            
            auto pack_copy = *pack.get();
            
            if (pack->is_response()) {
                auto pack_id = pack_copy.packet_cmd() << 31 | pack_copy.packet_seq();
                auto it = requests.find(pack_id);
                if (it != requests.end()) {
                    it->second.send(pack_copy);
                }
            } else {
                co_await received_request_channel.async_send({}, pack_copy, asio::use_awaitable);
            }
        }
        
        co_return success;
    }
private:
    asio::ip::tcp::socket socket;
    std::string device_id;
    volatile bool processing{false};
    volatile bool stopped{false};
    
    std::chrono::steady_clock::time_point last_recv_time;
    request_map_t requests;
    
    std::pair<oneshot::sender<void>, oneshot::receiver<void>> on_disconnected;
    packet_channel received_request_channel;
    
    asio::steady_timer send_timer;
    asio::steady_timer check_timer;
};

}
