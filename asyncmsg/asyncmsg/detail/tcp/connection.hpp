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

#include <asyncmsg/base/io_buffer.hpp>
#include <asyncmsg/tcp/packet.hpp>
#include <asyncmsg/base/debug_helper.hpp>

using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);

namespace asyncmsg { namespace tcp {
namespace detail {
using signal_channel = asio::experimental::channel<void(asio::error_code)>;
using packet_channel = asio::experimental::channel<void(asio::error_code, tcp::packet)>;
using received_request_channel_map = std::unordered_map<uint32_t, std::unique_ptr<packet_channel>>;
constexpr static uint32_t received_packet_channel_size = 64;
constexpr static uint32_t default_timeout = 5;
constexpr static uint32_t default_tries = 3;

class connection {
    constexpr static uint32_t recv_buf_size = 128*1024;
    constexpr static uint32_t active_connection_lifetime_seconds = 60;
    constexpr static uint32_t active_connection_lifetime_check_interval_seconds = 1;
    
    using request_map_t = std::unordered_map<uint64_t, std::pair<std::unique_ptr<packet_channel>, std::unique_ptr<asio::steady_timer>>>;
    enum class object_state {running,stopping,stopped};
    
public:
    connection(asio::io_context::executor_type executor_, asio::ip::tcp::socket socket_, std::string device_id_ = {})
    : executor(executor_)
    ,socket(std::move(socket_))
    , device_id(std::move(device_id_))
    , last_recv_time(std::chrono::steady_clock::now())
    , received_request_channel(executor, received_packet_channel_size)
    , on_disconnected(executor, 1)
    , on_stopped(executor, 1)
    , check_timer(executor) {
        start();
    }
    
    ~connection() {
        base::print_log("~connection");
    }
    
    asio::awaitable<void> stop() {
        if (state == object_state::stopped) {
            co_return;
        }
        
        if (state == object_state::stopping) {
            base::print_log("already stopping, wait for stopped signal begin");
            co_await on_stopped.async_receive(use_nothrow_awaitable);
            base::print_log("already stopping, wait for stopped signal end");
            co_return;
        }
        
        if (state == object_state::running) {
            co_await close();
        }
        
        state = object_state::stopping;
        
        base::print_log("wait_all_async_task_finished begin");
        asio::steady_timer wait_timer(co_await asio::this_coro::executor);
        while (processing || !requests.empty()) {
            base::print_log(std::string("stop connection processing = ") + std::to_string(processing) + ", requests.size =" + std::to_string(requests.size()));
            
            wait_timer.expires_after(std::chrono::milliseconds(100));
            co_await wait_timer.async_wait(use_nothrow_awaitable);
        }
        base::print_log("wait_all_async_task_finished end");
        
        state = object_state::stopped;
        base::print_log("send stopped signal begin");
        co_await on_stopped.async_send(asio::error_code{}, use_nothrow_awaitable);
        base::print_log("send stopped signal end");
    }
    
    asio::awaitable<void> send_packet(packet pack) {
        if (!can_work()) {
            co_return;
        }
        
        auto pack_buf = pack.packet_data();
        auto buf = asio::buffer(pack_buf.data(), pack_buf.size());
        co_await asio::async_write(socket, buf, use_nothrow_awaitable);
    }
    
    asio::awaitable<tcp::packet> send_packet(packet pack, uint32_t timeout_seconds) {
        if (!can_work()) {
            co_return packet{};
        }
        
        auto pack_buf = pack.packet_data();
        auto buf = asio::buffer(pack_buf.data(), pack_buf.size());
        auto [e_write, _] = co_await asio::async_write(socket, buf, use_nothrow_awaitable);
        
        if (e_write) {
            base::print_log("async_write err=" + e_write.message());
            co_await close();
            co_return packet{};
        }
        
        auto chan = std::make_unique<packet_channel>(executor, 1);
        uint64_t id = packet_id(pack.packet_cmd(), pack.packet_seq());

        requests.emplace(id, std::make_pair(std::move(chan), std::make_unique<asio::steady_timer>(co_await asio::this_coro::executor, std::chrono::seconds(timeout_seconds))));

        auto result = co_await(requests[id].second->async_wait(use_nothrow_awaitable) || requests[id].first->async_receive(use_nothrow_awaitable));
        requests.erase(id);

        if (result.index() == 0) {
            base::print_log("send timeout");
            co_return packet{};
        }

        auto [e_wait, pack_rsp] = std::get<1>(result);
        co_return e_wait ? packet{} : pack_rsp;
    }
    
    asio::awaitable<packet> request_received() {
        auto [e, pack] = co_await received_request_channel.async_receive(use_nothrow_awaitable);
        co_return e ? packet{} : pack;
    }
    
    asio::awaitable<void> connection_disconnected() {
        co_await on_disconnected.async_receive(use_nothrow_awaitable);
    }
    
    std::string get_device_id() {
        return device_id;
    }
private:
    void start() {
        auto task = [this]() -> asio::awaitable<void> {
            processing = true;
            co_await(check() && receive_packet());
            processing = false;
            base::print_log("set processing = false");
        };
        
        co_spawn(executor, task, asio::detached);
    }
    
    asio::awaitable<void> close() {
        if (!has_close) {
            has_close = true;
            base::print_log("close begin");
            
            //need to be early, before check_timer.cancel()
            co_await on_disconnected.async_send(asio::error_code{}, use_nothrow_awaitable);
            on_disconnected.cancel();

            for (auto& request : requests) {
                request.second.second->cancel();
            }
            
            received_request_channel.cancel();
            
            asio::error_code ec;
            socket.cancel();
            socket.shutdown(asio::socket_base::shutdown_both, ec);
            socket.close(ec);
            
            check_timer.cancel();
            base::print_log("close end");
        }
    }
    
    bool can_work() {
        return state == object_state::running;
    }
    
    asio::awaitable<void> check() {
        for (;;) {
            check_timer.expires_after(std::chrono::seconds(active_connection_lifetime_check_interval_seconds));
            auto [e] = co_await check_timer.async_wait(use_nothrow_awaitable);
            
            auto now = std::chrono::steady_clock::now();
            if (e || (std::chrono::duration_cast<std::chrono::seconds>(now - last_recv_time).count() > active_connection_lifetime_seconds)) {
                base::print_log("connection timeout");
                co_await close();
                break;
            }
        }
    }
    
    asio::awaitable<void> receive_packet() {
        base::io_buffer recv_buffer(recv_buf_size);
        for (;;) {
            auto size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                if (! co_await process_packet(recv_buffer)) {
                    co_await close();
                    break;
                }
            }
            
            size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                co_await close();
                break;
            }
            
            auto buf = recv_buffer.prepare(size_to_read);
            auto [e, read_size] = co_await socket.async_read_some(asio::buffer(buf.data, buf.size), use_nothrow_awaitable);
            
            if (e) {
                co_await close();
                base::print_log("read err = " + e.message());
                break;
            }
            
            if (read_size > 0) {
                recv_buffer.commit(read_size);
                if (! co_await process_packet(recv_buffer)) {
                    co_await close();
                    break;
                }
            }
        }
    }
    
    asio::awaitable<bool> process_packet(base::io_buffer& recv_buffer) {
        bool success = true;
        for(;;) {
            size_t consume_len = 0;
            auto pack = asyncmsg::tcp::parse_packet(recv_buffer.read_head(), recv_buffer.size(), consume_len);
            recv_buffer.consume(consume_len);
            
            if (!pack) {
                break;
            }
            
            if (pack->packet_device_id().empty()) {
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
                uint64_t pack_id = packet_id(pack_copy.packet_cmd(), pack_copy.packet_seq());
                
//                base::print_log("recv rsp cmd = " + std::to_string(pack_copy.packet_cmd()) + ", seq = " + std::to_string(pack_copy.packet_seq()));
                
                auto it = requests.find(pack_id);
                if (it != requests.end()) {
                    co_await it->second.first->async_send(asio::error_code{}, std::move(pack_copy), use_nothrow_awaitable);
                }
            } else {
                if (received_request_channel.is_open()) {
                    co_await received_request_channel.async_send({}, std::move(pack_copy), use_nothrow_awaitable);
                }
            }
        }
        
        co_return success;
    }
    
    uint64_t packet_id(uint32_t cmd, uint32_t seq) {
        uint64_t id = cmd;
        id = id << 31 | seq;
        return id;
    }
private:
    object_state state{object_state::running};
    volatile bool processing{false};
    
    asio::io_context::executor_type executor;
    asio::ip::tcp::socket socket;
    std::string device_id;
    
    std::chrono::steady_clock::time_point last_recv_time;
    request_map_t requests;
    
    packet_channel received_request_channel;
    signal_channel on_stopped;
    signal_channel on_disconnected;
    
    asio::steady_timer check_timer;
    
    std::atomic<bool> has_close{false};
};

}
}}
