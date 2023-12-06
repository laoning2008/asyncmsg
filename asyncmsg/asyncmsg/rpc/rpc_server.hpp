#pragma once
#include <functional>
#include <asyncmsg/tcp/tcp_server.hpp>
#include <asyncmsg/detail/rpc_protocol.hpp>

namespace asyncmsg { namespace rpc {
template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
void register_sync_handler(asio::io_context::executor_type executor, asyncmsg::tcp::tcp_server& server, const std::string& rpc_name, std::function<rpc_result<RSP>(REQ& req)> handler) {
    asio::co_spawn(executor, [&server, rpc_name, handler]() -> asio::awaitable<void> {
        auto id = detail::rpc_id(rpc_name);
        for (;;) {
            auto req = co_await server.wait_request(id);

            auto req_message = detail::parse_body<REQ>(req);
            if (!req_message) {
                base::print_log("register_sync_handler_impl, parse body failed");
                continue;
            }

            try {
                base::print_log("recv rpc request");
                auto rsp_result = handler(req_message.value());
                auto rsp = detail::build_response_packet(req, rsp_result);
                co_await server.send_packet(rsp);
            } catch (std::exception& e) {
                base::print_log("handle client request exception = " + std::string(e.what()));
            }
        }
        
        base::print_log("handler exit, rpc_name = " + rpc_name);
    }, asio::detached);
}

template<std::derived_from<google::protobuf::Message> REQ, std::derived_from<google::protobuf::Message> RSP>
void register_async_handler(asio::io_context::executor_type executor, asyncmsg::tcp::tcp_server& server, std::string rpc_name, std::function<asio::awaitable<rpc_result<RSP>>(REQ& req)> handler) {
        asio::co_spawn(executor, [&server, rpc_name, handler]() -> asio::awaitable<void> {
            auto id = detail::rpc_id(rpc_name);
            for (;;) {
                auto req = co_await server.wait_request(id);

                auto req_message = detail::parse_body<REQ>(req);
                if (!req_message) {
                    base::print_log("register_sync_handler_impl, parse body failed");
                    continue;
                }

                try {
                    base::print_log("recv rpc request");
                    auto rsp_result = co_await handler(req_message.value());
                    auto rsp = detail::build_response_packet(req, rsp_result);
                    co_await server.send_packet(rsp);
                } catch (std::exception& e) {
                    base::print_log("handle client request exception = " + std::string(e.what()));
                }
            }
            base::print_log("handler exit, rpc_name = " + rpc_name);

        }, asio::detached);
    }

template<std::derived_from<google::protobuf::Message> T>
asio::awaitable<void> push(asyncmsg::tcp::tcp_server& server, const std::string& device_id, const std::string& rpc_name, const google::protobuf::Message& request_message) {
    auto req = detail::build_request_packet(rpc_name, request_message, device_id);
    co_await server.send_packet_and_wait_rsp(req);
}

}}
