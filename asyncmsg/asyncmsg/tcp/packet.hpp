#pragma once
#include <memory>
#include <string>
#include <atomic>

#include <asyncmsg/base/crc.hpp>
#include <asyncmsg/base/byte_order.hpp>
#include <asyncmsg/base/ibuffer.hpp>

namespace asyncmsg { namespace tcp {

static std::atomic<uint32_t> seq_generator{0};
constexpr static uint8_t protocol_version = 1;
constexpr static uint8_t packet_begin_flag = 0x55;
constexpr static uint32_t device_id_size = 64;

#pragma pack(1)
struct packet_header {
    uint8_t         flag;
    uint32_t        cmd;
    uint32_t        seq;
    uint8_t         rsp;
    uint32_t        ec;
    uint8_t         device_id[device_id_size];
    uint32_t        body_len;
    uint8_t         crc;
};
#pragma pack()

constexpr static uint32_t header_length = sizeof(packet_header);
constexpr static uint32_t max_body_length = 16*1024;

class packet {
public:
    packet() = default;
    
    packet(uint32_t cmd_, bool rsp_, uint8_t* body_buf_ = {}, uint32_t body_len_ = {}, uint32_t seq_ = 0, std::string device_id_ = {}, uint32_t ec_ = 0)
    : cmd(cmd_), seq((seq_==0)?++seq_generator:seq_), rsp(rsp_), ec(ec_), device_id(std::move(device_id_)), body(body_buf_, body_len_, true) {
    }

    uint32_t packet_cmd() const {
        return cmd;
    }
    
    uint32_t packet_seq() const {
        return seq;
    }
    
    bool is_response() const {
        return rsp;
    }
    
    void set_is_response(bool is_rsp) {
        rsp = is_rsp;
    }
    
    bool is_valid() {
        return (cmd != 0) && (seq != 0) && !device_id.empty();
    }
    
    uint32_t get_ec() const {
        return ec;
    }
    
    const std::string& packet_device_id() const {
        return device_id;
    }
    
    void set_packet_device_id(std::string id) {
        device_id = std::move(id);
    }
 
    const base::ibuffer& packet_body() const {
        return body;
    }
    
//    void set_packet_body(ibuffer buf_body) {
//        body = std::move(buf_body);
//    }
    
    base::ibuffer packet_data() {
        auto data_len = header_length + body.size();
        auto data_buf = base::ibuffer{data_len};
        
        packet_header header;
        header.flag = packet_begin_flag;
        header.cmd = base::host_to_network_32(cmd);
        header.seq = base::host_to_network_32(seq);
        header.ec = base::host_to_network_32(ec);
        header.rsp = rsp ? 1 : 0;

        memset(header.device_id, 0, sizeof(header.device_id));
        memcpy(header.device_id, device_id.c_str(), std::min(device_id.size(), sizeof(header.device_id) - 1));
        
        header.body_len =base::host_to_network_32(body.size());
        header.crc = base::calc_crc8((uint8_t*)&header, header_length - 1);
        
        memcpy(data_buf.data(), &header, header_length);
        if (!body.empty()) {
            memcpy(data_buf.data() + header_length, body.data(), body.size());
        }
        
        return data_buf;
    }
private:
    uint32_t        cmd{0};
    uint32_t        seq{0};
    bool            rsp{false};
    uint32_t        ec{0};
    std::string     device_id;
    base::ibuffer   body;
};

static packet build_req_packet(uint32_t cmd_, uint8_t* body_buf_ = {}, uint32_t body_len_ = {}, std::string device_id_ = {}) {
    return packet{cmd_, false, body_buf_, body_len_, 0, device_id_};
}

static packet build_rsp_packet(uint32_t cmd_, uint32_t seq_, uint32_t ec_, std::string device_id_ = {}, uint8_t* body_buf_ = {}, uint32_t body_len_ = {}) {
    return packet{cmd_, true, body_buf_, body_len_, seq_, device_id_, ec_};
}

static std::unique_ptr<packet> parse_packet(uint8_t* buf, size_t buf_len, size_t& consume_len) {
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
        std::string device_id = (char*)header->device_id;
        return std::make_unique<packet>(cmd, rsp, buf_valid + header_length, body_len, seq, device_id, ec);
    } while (1);
    
    return nullptr;
}

}}
