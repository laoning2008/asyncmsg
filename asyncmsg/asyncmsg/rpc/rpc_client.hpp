#pragma once

#include <concepts>

#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/detail/rpc/rpc_protocol.hpp>

namespace asyncmsg { namespace rpc {

template<std::derived_from<google::protobuf::Message> T>
asio::awaitable<rpc_result<T>> call(tcp::tcp_client& client, const std::string& rpc_name, const google::protobuf::Message& request_message) {
    auto req_pack = detail::build_request_packet(rpc_name, request_message);
    auto rsp_pack = co_await client.send_packet_with_retry(req_pack);
    if (!rsp_pack.is_valid()) {
        co_return rpc_unexpected_result{err_network};
    }

    co_return detail::parse_body<T>(rsp_pack);
}

}}

