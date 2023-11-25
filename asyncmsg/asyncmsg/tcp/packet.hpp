#pragma once
#include <memory>
#include <string>
#include <atomic>

#include <asyncmsg/base/crc.hpp>
#include <asyncmsg/base/byte_order.hpp>
#include <asyncmsg/base/ibuffer.hpp>

namespace asyncmsg { namespace tcp {

static std::atomic<uint32_t> seq_generator{0};
constexpr static uint8_t packet_begin_flag = 0x55;
constexpr static uint32_t device_id_size = 64;

#pragma pack(1)
struct packet_header {
    uint8_t         flag;
    uint32_t        cmd;
    uint32_t        seq;
    uint8_t         rsp;
    uint32_t        ec;
    uint32_t        device_id_len;
    char            device_id[device_id_size];
    uint32_t        body_len;
    uint8_t         crc;
};
#pragma pack()

constexpr static uint32_t header_length = sizeof(packet_header);
constexpr static uint32_t max_body_length = 16*1024;

class packet {
    friend base::ibuffer encode_packet(packet& pack);
    friend std::unique_ptr<packet> decode_packet(uint8_t* buf, size_t buf_len, size_t& consume_len);
public:
    packet() : cmd_(0), seq_(0), rsp_(false), ec_(0) {
    }
    
    packet(uint32_t cmd__, bool rsp__, uint8_t* body_buf__, uint32_t body_len__, std::string device_id__, uint32_t seq__, uint32_t ec__ = 0)
    : cmd_(cmd__), seq_((seq__==0)?++seq_generator:seq__), rsp_(rsp__), ec_(ec__), device_id_(std::move(device_id__)), body_( body_len__, body_buf__) {
    }

    uint32_t cmd() const {
        return cmd_;
    }
    
    uint32_t seq() const {
        return seq_;
    }
    
    bool is_response() const {
        return rsp_;
    }
    
    uint32_t ec() const {
        return ec_;
    }
    
    void set_device_id(std::string& device_id__) {
        device_id_ = device_id__;
    }
    
    const std::string& device_id() const {
        return device_id_;
    }
 
    const base::ibuffer& body() const {
        return body_;
    }
private:
    uint32_t        cmd_;
    uint32_t        seq_;
    bool            rsp_;
    uint32_t        ec_;
    std::string     device_id_;
    base::ibuffer   body_;
};

static packet build_req_packet(uint32_t cmd, uint8_t* body_buf = nullptr, uint32_t body_len = 0, std::string device_id = {}) {
    return packet{cmd, false, body_buf, body_len, device_id, 0};
}

static packet build_rsp_packet(uint32_t cmd, uint32_t seq, uint32_t ec, std::string device_id, uint8_t* body_buf, uint32_t body_len) {
    return packet{cmd, true, body_buf, body_len, device_id, seq, ec};
}

base::ibuffer encode_packet(packet& pack) {
    auto data_len = header_length + pack.body_.size();
    auto data_buf = base::ibuffer{data_len};
    
    packet_header header;
    header.flag = packet_begin_flag;
    header.cmd = base::host_to_network_32(pack.cmd_);
    header.seq = base::host_to_network_32(pack.seq_);
    header.ec = base::host_to_network_32(pack.ec_);
    header.rsp = pack.rsp_ ? 1 : 0;

    header.device_id_len = base::host_to_network_32((uint32_t)pack.device_id_.size());
    
    memset(header.device_id, 0, sizeof(header.device_id));
    assert(pack.device_id_.size() < sizeof(header.device_id));
    strcpy(header.device_id, pack.device_id_.c_str());
    
    header.body_len = base::host_to_network_32(pack.body_.size());
    header.crc = base::calc_crc8((uint8_t*)&header, header_length - 1);
    
    memcpy(data_buf.data(), &header, header_length);
    if (!pack.body_.empty()) {
        memcpy(data_buf.data() + header_length, pack.body_.data(), pack.body_.size());
    }
    
    return data_buf;
}

std::unique_ptr<packet> decode_packet(uint8_t* buf, size_t buf_len, size_t& consume_len) {
    consume_len = 0;
    do {
        for (; consume_len < buf_len; ++consume_len) {
            if (buf[consume_len] == packet_begin_flag) {
                break;
            }
        }
        
        uint8_t* buf_valid = buf + consume_len;
        size_t buf_valid_len = buf_len - consume_len;
        
        if (buf_valid_len < header_length) {
            return nullptr;
        }
        
        packet_header* header = (packet_header*)buf_valid;
        
        auto crc = base::calc_crc8(buf_valid, header_length - 1);
        if (crc != header->crc) {
            ++consume_len;
            continue;
        }
        
        uint32_t body_len = base::network_to_host_32(header->body_len);
        
        if (body_len > max_body_length) {
            consume_len += header_length;
            continue;
        }
        
        if (buf_valid_len < header_length + body_len) {
            return nullptr;
        }
        consume_len += header_length + body_len;

        uint32_t cmd = base::network_to_host_32(header->cmd);
        uint32_t seq = base::network_to_host_32(header->seq);
        uint32_t ec = base::network_to_host_32(header->ec);
        bool rsp = (header->rsp == 0) ? false : true;
        uint32_t device_id_len = base::network_to_host_32(header->device_id_len);
        std::string device_id;
        device_id.assign(header->device_id, header->device_id + device_id_len);
        return std::make_unique<packet>(cmd, rsp, buf_valid + header_length, body_len, device_id, seq, ec);
    } while (1);
    
    return nullptr;
}

}}
