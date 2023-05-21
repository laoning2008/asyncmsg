#pragma once
#include <thread>
#include <vector>
#include <map>
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
        stop();
    }
    
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    void register_sync_handler(std::string rpc_name, std::function<rpc_result<RSP>(REQ req)> handler) {
        auto signal = base::create<void>();
        auto id = detail::rpc_id(rpc_name);
        handler_signals[id] = std::move(signal.first);
        
        asio::co_spawn(io_context.get_executor(), register_sync_handler_impl<REQ, RSP>((uint32_t)id, std::move(signal.second), handler), asio::detached);
    }
    
    void start(uint32_t thread_num) {
        for (auto i = 0; i < thread_num; ++i) {
            workers.emplace_back([this]() {
                io_context.run();
            });
        }
    }
    
    void stop() {
        auto task = [this]() -> asio::awaitable<void> {
            work_guard.reset();
            for (auto& signal : handler_signals) {
                signal.second.send();
            }
        };
        asio::co_spawn(io_context.get_executor(), task(), asio::detached);
        
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
private:
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    asio::awaitable<void> register_sync_handler_impl(uint32_t id, base::receiver<void>&& exit_signal, std::function<rpc_result<RSP>(REQ req)> handler) {
        for (;;) {
            auto wait_result = co_await(exit_signal.async_wait(use_nothrow_awaitable) || server.await_request((uint32_t)id));
            if (wait_result.index() == 0) {
                break;
            }

            asyncmsg::tcp::packet req_pack(std::get<1>(std::move(wait_result)));
            if (!req_pack.is_valid()) {
                continue;
            }

            auto request_message = detail::parse_body<REQ>(req_pack);
            if (!request_message) {
                continue;
            }

            auto rsp_result = handler(request_message.value());
            auto rsp_pack = detail::build_response_packet(id, req_pack, rsp_result);
            co_await server.send_packet_with_retry(rsp_pack);
        }
    }
private:
    asyncmsg::tcp::tcp_server server;
    
    asio::io_context io_context;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    std::vector<std::thread> workers;
    
    std::unordered_map<size_t, base::sender<void>> handler_signals;
};

}}
