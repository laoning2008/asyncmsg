#pragma once

#include <string_view>
#include <optional>

#include <asyncmsg/tcp/packet.hpp>
#include <asyncmsg/rpc/err_code.hpp>

#include <google/protobuf/message.h>


namespace asyncmsg { namespace rpc {

namespace detail {

uint32_t rpc_id(const std::string& rpc_name) {
    std::hash<std::string> hasher;
    return (uint32_t)hasher(rpc_name);
}

tcp::packet build_request_packet(const std::string& rpc_name, const google::protobuf::Message& message, const std::string& device_id = {}) {
    uint32_t size = (uint32_t)message.ByteSizeLong();
    asyncmsg::base::ibuffer body{size};
    message.SerializeToArray(body.data(), (int)size);
    
    auto id = rpc_id(rpc_name);
    
    return asyncmsg::tcp::build_req_packet(id, body.data(), body.size(), device_id);
    
}

template<std::derived_from<google::protobuf::Message> RSP>
tcp::packet build_response_packet(const tcp::packet& req, rpc_result<RSP>& rsp_message) {
    if (!rsp_message) {
        return asyncmsg::tcp::build_rsp_packet(req.cmd(), req.seq(), rsp_message.error(), req.device_id(), nullptr, 0);
    }
    
    auto& proto_message = rsp_message.value();
    auto size = (uint32_t)proto_message.ByteSizeLong();
    asyncmsg::base::ibuffer body{size};
    proto_message.SerializeToArray(body.data(), (int)size);
    
    return asyncmsg::tcp::build_rsp_packet(req.cmd(), req.seq(), 0, req.device_id(), body.data(), body.size());
}

template<std::derived_from<google::protobuf::Message> T>
rpc_result<T> parse_body(const tcp::packet& pack) {
    auto ec = pack.ec();
    if (ec != 0) {
        return rpc_unexpected_result{ec};
    }
    
    T message;
    if (!message.ParseFromArray(pack.body().data(), pack.body().size())) {
        return rpc_unexpected_result{err_deserialize};
    }
    
    return rpc_result<T>{message};
}

}

}}

