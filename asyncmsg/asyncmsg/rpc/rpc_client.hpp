#pragma once

#include <concepts>

#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/detail/rpc_protocol.hpp>

namespace asyncmsg { namespace rpc {

template<std::derived_from<google::protobuf::Message> T>
asio::awaitable<rpc_result<T>> call(tcp::tcp_client& client, const std::string& rpc_name, const google::protobuf::Message& request_message) {
    auto req = detail::build_request_packet(rpc_name, request_message);
    auto rsp = co_await client.send_packet_and_wait_rsp(req);

    co_return asyncmsg::rpc::detail::parse_body<T>(rsp);
}


template<std::derived_from<google::protobuf::Message> REQ>
asio::awaitable<REQ> wait_push(tcp::tcp_client& client, const std::string& rpc_name) {
    auto id = detail::rpc_id(rpc_name);
    
    do {
        auto req = co_await client.wait_request(id);

        auto req_message = detail::parse_body<REQ>(req);
        if (!req_message) {
            base::print_log("wait_push, parse body failed");
            continue;
        }
        
        co_return req_message.value();
    } while (true);
}

}}

