#pragma once
#include <asyncmsg/detail/config.hpp>
#include <exception>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <stdexcept>

#include <asio/awaitable.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asyncmsg/detail/io_buffer.hpp>
#include <asyncmsg/tcp/packet.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asyncmsg/tcp/exception.hpp>

using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);

namespace asyncmsg { namespace tcp {
namespace detail {

constexpr static uint32_t received_packet_channel_size = 64;
constexpr static uint32_t default_timeout = 5*1000;
constexpr static uint32_t default_tries = 3;

class connection;

using disconnected_callback = std::function<void(connection*, const std::string&)>;
using got_device_id_callback = std::function<void(connection*, const std::string&)>;
using receive_request_callback = std::function<void(connection*, const std::string&, packet)>;
using packet_channel = asio::experimental::channel<void(asio::error_code, packet)>;
using received_request_channel_map = std::unordered_map<uint32_t, std::unique_ptr<detail::packet_channel>>;
constexpr static uint32_t active_connection_heartbeat_interval_seconds = 20;
constexpr static uint32_t heartbeat_cmd = 0;

class connection : public std::enable_shared_from_this<connection> {
    constexpr static uint32_t recv_buf_size = 128*1024;
    constexpr static uint32_t active_connection_lifetime_seconds = 60;
    constexpr static uint32_t active_connection_lifetime_check_interval_seconds = 1;

    
    using request_map_t = std::unordered_map<uint64_t, std::unique_ptr<packet_channel>>;
    enum class object_state {running, stopped};
    
public:
    connection(asio::io_context::executor_type executor_, asio::ip::tcp::socket socket_, disconnected_callback on_disconnected_, got_device_id_callback on_got_device_id_, receive_request_callback on_receive_request_, std::string device_id__ = {})
    : executor(executor_)
    , socket(std::move(socket_))
    , on_disconnected(on_disconnected_)
    , on_got_device_id(on_got_device_id_)
    , on_receive_request(on_receive_request_)
    , device_id_(std::move(device_id_))
    , last_recv_time(std::chrono::steady_clock::now()) {
        start();
    }
    
    ~connection() {
        base::print_log("~connection");
    }
    
    
    asio::awaitable<void> send_packet(const packet& pack) {
        auto pack_buf = encode_packet(pack);
        auto buf = asio::buffer(pack_buf.data(), pack_buf.size());
        
        base::print_log("send packet cmd = " + std::to_string(pack.cmd()) + ", seq = " + std::to_string(pack.seq()));

        co_await asio::async_write(socket, buf, asio::use_awaitable);
    }
    
    void send_packet_detach(packet pack) {
        asio::co_spawn(executor, [weak_this = weak_from_this(), this, pack = std::move(pack)]() -> asio::awaitable<void> {
            if (weak_this.lock()) {
                co_await send_packet(pack);
            }
        }, asio::detached);
    }
    
    asio::awaitable<packet> send_packet_and_wait_rsp(packet& pack, uint32_t timeout_millliseconds) {
        auto pack_buf = encode_packet(pack);
        auto buf = asio::buffer(pack_buf.data(), pack_buf.size());
        auto weak_this = weak_from_this();
        
        base::print_log("send_packet_and_wait_rsp cmd = " + std::to_string(pack.cmd()) + ", seq = " + std::to_string(pack.seq()));
        
        co_await asio::async_write(socket, buf, asio::use_awaitable);
        
        if (!weak_this.lock()) {
            throw invalid_state_error{};
        }
        
        uint64_t id = packet_id(pack);
        
        requests[id] = std::make_unique<packet_channel>(executor, 1);
        asio::steady_timer timeout(executor);
        timeout.expires_from_now(std::chrono::milliseconds(timeout_millliseconds));
        
        auto result = co_await(requests[id]->async_receive(asio::use_awaitable) || timeout.async_wait(asio::use_awaitable));
        if (result.index() == 1) {
            base::print_log("send_packet----timeout");
            throw timeout_error{};
        }
        
        auto shared_this = weak_this.lock();
        if (shared_this) {
            requests.erase(id);
        }
        
        co_return std::get<0>(result);
    }
private:
    void start() {
        //post to make sure shared_ptr has been constructed
        asio::post(executor, [this]() {
            asio::co_spawn(executor, [this]() -> asio::awaitable<void>  {
                co_await(receive_packet() || check());
            }, asio::detached);
        });
    }
    
    asio::awaitable<void> check() {
        asio::steady_timer check_timer(executor);
        for (;;) {
            check_timer.expires_after(std::chrono::seconds(active_connection_lifetime_check_interval_seconds));
            auto weak_this = weak_from_this();
            co_await check_timer.async_wait(asio::use_awaitable);
            if (!weak_this.lock()) {
                break;
            }
            
            auto now = std::chrono::steady_clock::now();
            auto elapse = std::chrono::duration_cast<std::chrono::seconds>(now - last_recv_time).count();
            if (elapse > active_connection_lifetime_seconds) {
                base::print_log("connection timeout");
                on_disconnected(this, device_id_);
                break;
            }
        }
    }
    
    asio::awaitable<void> receive_packet() {
        asyncmsg::detail::io_buffer recv_buffer(recv_buf_size);
        for (;;) {
            auto size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                if (!process_packet(recv_buffer)) {
                    on_disconnected(this, device_id_);
                    break;
                }
            }
            
            size_to_read = (recv_buffer.free_size() > 0) ? recv_buffer.free_size() : recv_buffer.capacity();
            if (size_to_read <= 0) {
                on_disconnected(this, device_id_);
                break;
            }
            
            auto buf = recv_buffer.prepare(size_to_read);
            auto weak_this = weak_from_this();
            auto [e, read_size] = co_await socket.async_read_some(asio::buffer(buf.data, buf.size), use_nothrow_awaitable);
            if (!weak_this.lock()) {
                break;
            }
            
            if (e) {
                on_disconnected(this, device_id_);
                base::print_log("read err = " + e.message());
                break;
            }
            
            if (read_size > 0) {
                recv_buffer.commit(read_size);
                if (!process_packet(recv_buffer)) {
                    on_disconnected(this, device_id_);
                    break;
                }
            }
        }
    }
    
    bool process_packet(asyncmsg::detail::io_buffer& recv_buffer) {
        for(;;) {
            size_t consume_len = 0;
            auto pack_ptr = decode_packet(recv_buffer.read_head(), recv_buffer.size(), consume_len);
            recv_buffer.consume(consume_len);
            
            if (!pack_ptr) {
                return true;
            }
            
            if (pack_ptr->device_id().empty()) {
                continue;
            }
            
            if (device_id_.empty()) {
                device_id_ = pack_ptr->device_id();
                on_got_device_id(this, device_id_);
            } else if (device_id_ != pack_ptr->device_id()) {
                return false;
            }
            
            last_recv_time = std::chrono::steady_clock::now();
            
            auto pack = *pack_ptr;
            
            base::print_log("recv packet cmd = " + std::to_string(pack.cmd()) + ", seq = " + std::to_string(pack.seq()));
            
            if (pack.is_response()) {
                uint64_t pack_id = packet_id(pack);
                                
                auto it = requests.find(pack_id);
                if (it != requests.end()) {
                    it->second->try_send(asio::error_code{}, std::move(pack));
                }
            } else {
                on_receive_request(this, device_id_, std::move(pack));
            }
        }
        
        return true;
    }
private:
    asio::io_context::executor_type executor;
    asio::ip::tcp::socket socket;
    std::string device_id_;
    std::chrono::steady_clock::time_point last_recv_time;
    
    disconnected_callback on_disconnected;
    got_device_id_callback on_got_device_id;
    receive_request_callback on_receive_request;
    
    request_map_t requests;
};

}
}}
