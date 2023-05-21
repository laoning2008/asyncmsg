#pragma once
#include <thread>
#include <vector>
#include <list>
#include <asyncmsg/tcp/tcp_server.hpp>
#include <asyncmsg/base/oneshot.hpp>

#include <asyncmsg/detail/rpc/rpc_protocol.hpp>

namespace asyncmsg { namespace rpc {

constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);


/// note:
/// 1. calling register_sync_handler before calling start
/// 2. calling stop is optional
class rpc_server final {
public:
    rpc_server(uint32_t port)
    : server(port)
    , work_guard(io_context.get_executor()) {
    }
    
    ~rpc_server() {
        base::print_log("~rpc_server begin");
        stop();
        base::print_log("~rpc_server end");
    }
    
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    void register_sync_handler(std::string rpc_name, std::function<rpc_result<RSP>(REQ req)> handler) {
        auto [s, r] = base::create<void>();
        handler_exit_signals.emplace_back(std::move(s));
        
        asio::co_spawn(io_context.get_executor(), register_sync_handler_impl<REQ, RSP>(std::move(rpc_name), std::move(r), handler), asio::detached);
    }
    
    //not thread safe
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    void register_async_handler(std::string rpc_name, std::function<asio::awaitable<rpc_result<std::unique_ptr<RSP>>>(REQ req)> handler) {
        auto [s, r] = base::create<void>();
        handler_exit_signals.emplace_back(std::move(s));
        
        asio::co_spawn(io_context.get_executor(), register_async_handler_impl<REQ, RSP>(std::move(rpc_name), std::move(r), handler), asio::detached);
    }
    
    void start(uint32_t thread_num) {
        for (auto i = 0; i < thread_num; ++i) {
            workers.emplace_back([this]() {
                io_context.run();
            });
        }
    }
    
    void stop() {
        work_guard.reset();
        for (auto& signal : handler_exit_signals) {
            signal.send();
        }
        
        base::print_log("stop join thread begin");
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        base::print_log("stop join thread end");
    }
private:
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    asio::awaitable<void> register_sync_handler_impl(std::string rpc_name, base::receiver<void> exit_signal,  std::function<rpc_result<RSP>(REQ req)> handler) {
        auto id = detail::rpc_id(rpc_name);
        for (;;) {
            auto wait_result = co_await (exit_signal.async_wait(use_nothrow_awaitable) || server.await_request(id));

            if (wait_result.index() == 0) {
                base::print_log("exit_signal fired");
                break;
            }
            
            auto req_pack = std::get<1>(wait_result);

            if (!req_pack.is_valid()) {
                base::print_log("register_sync_handler_impl, invalid packet");
                continue;
            }

            auto request_message = detail::parse_body<REQ>(req_pack);
            if (!request_message) {
                base::print_log("register_sync_handler_impl, parse body failed");
                continue;
            }

            auto rsp_result = handler(request_message.value());
            auto rsp_pack = detail::build_response_packet(id, req_pack, rsp_result);
            co_await server.send_packet(rsp_pack);
        }
    }
    
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    asio::awaitable<void> register_async_handler_impl(std::string rpc_name, base::receiver<void> exit_signal,  std::function<asio::awaitable<rpc_result<std::unique_ptr<RSP>>>(REQ req)> handler) {
        auto id = detail::rpc_id(rpc_name);
        for (;;) {
            auto wait_result = co_await (exit_signal.async_wait(use_nothrow_awaitable) || server.await_request(id));

            if (wait_result.index() == 0) {
                base::print_log("exit_signal fired");
                break;
            }
            
            auto req_pack = std::get<1>(wait_result);
            if (!req_pack.is_valid()) {
                base::print_log("register_sync_handler_impl, invalid packet");
                continue;
            }

            auto request_message = detail::parse_body<REQ>(req_pack);
            if (!request_message) {
                base::print_log("register_sync_handler_impl, parse body failed");
                continue;
            }

            auto rsp_result = co_await handler(request_message.value());
            auto rsp_pack = detail::build_response_packet(id, req_pack, rsp_result);
            co_await server.send_packet(rsp_pack);
        }
    }
private:
    asyncmsg::tcp::tcp_server server;
    
    asio::io_context io_context;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    std::vector<std::thread> workers;
    
    std::list<base::sender<void>> handler_exit_signals;
};

}}
