#pragma once

#include <string_view>
#include <optional>

#include <google/protobuf/message.h>

#include <asyncmsg/tcp/packet.hpp>
#include <asyncmsg/rpc/err_code.hpp>

namespace asyncmsg { namespace rpc {

namespace detail {

uint32_t rpc_id(const std::string& rpc_name) {
    std::hash<std::string> hasher;
    return (uint32_t)hasher(rpc_name);
}

tcp::packet build_request_packet(const std::string& rpc_name, const google::protobuf::Message& message) {
    auto size = message.ByteSizeLong();
    asyncmsg::base::ibuffer body{size};
    message.SerializeToArray(body.data(), (int)size);
    
    auto id = rpc_id(rpc_name);
    
    return asyncmsg::tcp::build_req_packet(id, body.data(), body.size());
    
}

template<std::derived_from<google::protobuf::Message> RSP>
tcp::packet build_response_packet(uint32_t id, const tcp::packet& req_packet, rpc_result<RSP>& rsp_message) {
    if (!rsp_message) {
        return asyncmsg::tcp::build_rsp_packet(id, req_packet.packet_seq(), rsp_message.error(), req_packet.packet_device_id());
    }
    
    auto& proto_message = rsp_message.value();
    auto size = proto_message.ByteSizeLong();
    asyncmsg::base::ibuffer body{size};
    proto_message.SerializeToArray(body.data(), (int)size);
    
    return asyncmsg::tcp::build_rsp_packet(id, req_packet.packet_seq(), 0, req_packet.packet_device_id(), body.data(), body.size());
}

template<std::derived_from<google::protobuf::Message> RSP>
tcp::packet build_response_packet(const tcp::packet& req_packet, rpc_result<std::unique_ptr<RSP>>& rsp_message) {
//    base::print_log("build_response_packet id = " + std::to_string(id));
    
    if (!rsp_message) {
        return asyncmsg::tcp::build_rsp_packet(req_packet.packet_cmd(), req_packet.packet_seq(), rsp_message.error(), req_packet.packet_device_id());
    }
    
    auto& proto_message = rsp_message.value();
    auto size = proto_message->ByteSizeLong();
    asyncmsg::base::ibuffer body{size};
    proto_message->SerializeToArray(body.data(), (int)size);
    
    return asyncmsg::tcp::build_rsp_packet(req_packet.packet_cmd(), req_packet.packet_seq(), 0, req_packet.packet_device_id(), body.data(), body.size());
}


template<std::derived_from<google::protobuf::Message> T>
rpc_result<T> parse_body(const tcp::packet& pack) {
    auto ec = pack.get_ec();
    if (ec != 0) {
        return rpc_unexpected_result{ec};
    }
    
    T message;
    if (!message.ParseFromArray(pack.packet_body().data(), pack.packet_body().size())) {
        return rpc_unexpected_result{err_deserialize};
    }
    
    return message;
}

}

}}

