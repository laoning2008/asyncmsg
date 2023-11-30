#pragma once
#include <asyncmsg/base/config.hpp>
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

constexpr static uint32_t received_packet_channel_size = 64;
constexpr static uint32_t default_timeout = 5;
constexpr static uint32_t default_tries = 3;

class connection;

using disconnected_callback = std::function<void(connection*, const std::string&)>;
using got_device_id_callback = std::function<void(connection*, const std::string&)>;
using receive_request_callback = std::function<void(connection*, const std::string&, packet)>;
using packet_channel = asio::experimental::channel<void(asio::error_code, packet)>;
using received_request_channel_map = std::unordered_map<uint32_t, std::unique_ptr<detail::packet_channel>>;

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
    
    asio::awaitable<void> send_packet(packet& pack) {
        auto pack_buf = encode_packet(pack);
        auto buf = asio::buffer(pack_buf.data(), pack_buf.size());
        
        auto weak_this = weak_from_this();
        co_await asio::async_write(socket, buf, asio::use_awaitable);
    }
    
    asio::awaitable<std::optional<packet>> send_packet_and_wait_rsp(packet& pack, uint32_t timeout_seconds) {
        auto pack_buf = encode_packet(pack);
        auto buf = asio::buffer(pack_buf.data(), pack_buf.size());
        auto weak_this = weak_from_this();
        
        co_await asio::async_write(socket, buf, asio::use_awaitable);
        
        if (!weak_this.lock()) {
            co_return std::nullopt;
        }
        
        std::optional<packet> rsp_packet = std::nullopt;
        uint64_t id = gen_packet_id(pack.cmd(), pack.seq());
        
        requests[id] = std::make_unique<packet_channel>(executor, 1);
        asio::steady_timer timeout(executor);
        timeout.expires_from_now(std::chrono::seconds(timeout_seconds));
        
        auto result = co_await(requests[id]->async_receive(asio::use_awaitable) || timeout.async_wait(asio::use_awaitable));
        if (result.index() == 0) {
            rsp_packet = std::get<0>(result);
        } else if (result.index() == 1) {
            base::print_log("send_packet----timeout");
        }

        auto shared_this = weak_this.lock();
        if (shared_this) {
            requests.erase(id);
        }
        
        co_return rsp_packet;
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
            auto [e] = co_await check_timer.async_wait(use_nothrow_awaitable);
            if (!weak_this.lock()) {
                break;
            }
            
            auto now = std::chrono::steady_clock::now();
            auto elapse = std::chrono::duration_cast<std::chrono::seconds>(now - last_recv_time).count();
            if (e || elapse > active_connection_lifetime_seconds) {
                base::print_log("connection timeout");
                on_disconnected(this, device_id_);
                break;
            }
        }
    }
    
    asio::awaitable<void> receive_packet() {
        base::io_buffer recv_buffer(recv_buf_size);
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
    
    bool process_packet(base::io_buffer& recv_buffer) {
        for(;;) {
            size_t consume_len = 0;
            auto pack = decode_packet(recv_buffer.read_head(), recv_buffer.size(), consume_len);
            recv_buffer.consume(consume_len);
            
            if (!pack) {
                return true;
            }
            
            if (pack->device_id().empty()) {
                continue;
            }
            
            if (device_id_.empty()) {
                device_id_ = pack->device_id();
                on_got_device_id(this, device_id_);
            } else if (device_id_ != pack->device_id()) {
                return false;
            }
            
            last_recv_time = std::chrono::steady_clock::now();
            
            auto pack_copy = *pack;
            
            if (pack_copy.is_response()) {
                uint64_t pack_id = gen_packet_id(pack_copy.cmd(), pack_copy.seq());
                
                base::print_log("recv rsp cmd = " + std::to_string(pack_copy.cmd()) + ", seq = " + std::to_string(pack_copy.seq()));
                
                auto weak_this = weak_from_this();
                asio::co_spawn(executor, [weak_this, pack_id, pack = std::move(pack_copy), this]() -> asio::awaitable<void> {
                    if (weak_this.lock()) {
                        auto it = requests.find(pack_id);
                        if (it != requests.end()) {
                            co_await it->second->async_send(asio::error_code{}, std::move(pack), use_nothrow_awaitable);
                        }
                    }
                }, asio::detached);
            } else {
                on_receive_request(this, device_id_, std::move(pack_copy));
            }
        }
        
        return true;
    }
    
    uint64_t gen_packet_id(uint32_t cmd, uint32_t seq) {
        uint64_t id = cmd;
        id = id << 31 | seq;
        return id;
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
