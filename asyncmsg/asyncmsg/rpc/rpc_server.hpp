#pragma once
#include <thread>
#include <vector>
#include <list>
#include <asyncmsg/tcp/tcp_server.hpp>
#include <asyncmsg/base/async_mutex.hpp>

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
        handler_exit_signals.emplace_back(std::make_unique<tcp::detail::signal_channel>(io_context.get_executor(), 1));
        
        asio::co_spawn(io_context.get_executor(), register_sync_handler_impl<REQ, RSP>(std::move(rpc_name), handler_exit_signals.back().get(), handler), asio::detached);
    }
    
    //not thread safe
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    void register_async_handler(std::string rpc_name, std::function<asio::awaitable<rpc_result<std::unique_ptr<RSP>>>(REQ req)> handler) {
        handler_exit_signals.emplace_back(std::make_unique<tcp::detail::signal_channel>(io_context.get_executor(), 1));
        
        asio::co_spawn(io_context.get_executor(), register_async_handler_impl<REQ, RSP>(std::move(rpc_name), handler_exit_signals.back().get(), handler), asio::detached);
    }
    
    void start(uint32_t thread_num) {
        worker = std::thread([this]() {
            io_context.run();
        });
    }
    
    void stop() {
        auto task = [this]() -> asio::awaitable<void> {
            work_guard.reset();
            
            co_await server.cancel_all_await_request();
            
            for (auto& signal : handler_exit_signals) {
                co_await signal->async_send(asio::error_code{}, use_nothrow_awaitable);
            }
            
            for (auto& signal : worker_exit_signals) {
                base::print_log("send worker exit signal");
                co_await signal.second->async_send(asio::error_code{}, use_nothrow_awaitable);
            }
        };
        
        asio::co_spawn(io_context.get_executor(), task(), asio::detached);
        
        base::print_log("stop join thread begin");
        if (worker.joinable()) {
            worker.join();
        }
        base::print_log("stop join thread end");
    }
private:
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    asio::awaitable<void> register_sync_handler_impl(std::string rpc_name, tcp::detail::signal_channel* exit_signal,  std::function<rpc_result<RSP>(REQ req)> handler) {
        auto id = detail::rpc_id(rpc_name);
        for (;;) {
            auto wait_result = co_await (exit_signal->async_receive(use_nothrow_awaitable) || server.await_request(id));

            if (wait_result.index() == 0) {
                base::print_log("exit_signal fired");
                break;
            }
            
            auto req_pack = std::get<1>(wait_result);

            if (!req_pack.is_valid()) {//exit
                base::print_log("register_sync_handler_impl, invalid packet");
                break;
            }

            auto request_message = detail::parse_body<REQ>(req_pack);
            if (!request_message) {
                base::print_log("register_sync_handler_impl, parse body failed");
                continue;
            }

            auto rsp_result = handler(request_message.value());
            auto rsp_pack = detail::build_response_packet(req_pack, rsp_result);
            co_await server.send_packet(rsp_pack);
        }
    }
    
    template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
    asio::awaitable<void> register_async_handler_impl(std::string rpc_name, tcp::detail::signal_channel* exit_signal,  std::function<asio::awaitable<rpc_result<std::unique_ptr<RSP>>>(REQ req)> handler) {
        auto id = detail::rpc_id(rpc_name);
        for (;;) {
            auto wait_result = co_await (exit_signal->async_receive(use_nothrow_awaitable) || server.await_request(id));

            if (wait_result.index() == 0) {
                base::print_log("exit_signal fired");
                break;
            }
            
            auto req_pack = std::get<1>(wait_result);
            if (!req_pack.is_valid()) {
                base::print_log("register_async_handler_impl, invalid packet");
                break;
            }

//            base::print_log("recv req cmd = " + std::to_string(req_pack.packet_cmd()) + ", seq = " + std::to_string(req_pack.packet_seq()) + ", id = " + std::to_string(id));
            
            auto request_message = detail::parse_body<REQ>(req_pack);
            if (!request_message) {
                base::print_log("register_sync_handler_impl, parse body failed");
                continue;
            }

            auto worker_id = ++cur_worker_id;
            auto worker_exit_signal = std::make_unique<tcp::detail::signal_channel>(io_context.get_executor(), 1);
            auto worker_exit_signal_raw = worker_exit_signal.get();
            worker_exit_signals[worker_id] = std::move(worker_exit_signal);
            
            
            auto worker = [this, handler](tcp::packet req_pack, REQ request_message, tcp::detail::signal_channel* exit_signal, uint64_t worker_id) -> asio::awaitable<void> {
                auto rsp_result = co_await (exit_signal->async_receive(use_nothrow_awaitable) || handler(request_message));
                
                if (rsp_result.index() == 0) {
                    base::print_log("worker exit_signal fired");
                    co_return;
                }
                
                auto rsp_pack = detail::build_response_packet(req_pack, std::get<1>(rsp_result));
                                
                co_await server.send_packet(rsp_pack);

                //delete it late. still need it to be alive now. should be last in here, after any async code(co_await)
                asio::post(io_context.get_executor(), [this, worker_id]() {worker_exit_signals.erase(worker_id);});
            };
            
            auto req_msg = request_message.value();
            asio::co_spawn(io_context.get_executor(), worker(req_pack, req_msg, worker_exit_signal_raw, worker_id), asio::detached);
        }
    }
private:
    asyncmsg::tcp::tcp_server server;
    
    asio::io_context io_context;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    std::thread worker;
    
    std::list<std::unique_ptr<tcp::detail::signal_channel>> handler_exit_signals;
    
    std::unordered_map<uint64_t, std::unique_ptr<tcp::detail::signal_channel>> worker_exit_signals;
    std::atomic<uint64_t> cur_worker_id{0};
};

}}
