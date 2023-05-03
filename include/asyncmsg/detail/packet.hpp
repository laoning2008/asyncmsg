#pragma once
#include <memory>
#include <string>
#include <atomic>

#include <asyncmsg/detail/crc.hpp>
#include <asyncmsg/detail/byte_order.hpp>
#include <asyncmsg/detail/ibuffer.hpp>

namespace asyncmsg {

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
    
    packet(uint32_t cmd_, bool rsp_, std::string device_id_, uint32_t seq_ = 0, uint8_t* body_buf_ = {}, uint32_t body_len_ = {})
    : cmd(cmd_), seq((seq_==0)?++seq_generator:seq_), rsp(rsp_), device_id(std::move(device_id_)), body(body_buf_, body_len_, true) {
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
    
    bool is_valid() {
        return (cmd != 0) && (seq != 0) && !device_id.empty();
    }
    
    const std::string& packet_device_id() const {
        return device_id;
    }
    
    const ibuffer packet_body() const {
        return body;
    }
    
    ibuffer packet_data() {
        auto data_len = header_length + body.len();
        auto data_buf = ibuffer{data_len};
        
        packet_header header;
        header.flag = packet_begin_flag;
        header.cmd = asyncmsg::detail::host_to_network_32(cmd);
        header.seq = asyncmsg::detail::host_to_network_32(seq);
        header.rsp = rsp ? 1 : 0;

        memset(header.device_id, 0, sizeof(header.device_id));
        memcpy(header.device_id, device_id.c_str(), device_id.size());
        
        header.body_len = asyncmsg::detail::host_to_network_32(body.len());
        header.crc = asyncmsg::detail::calc_crc8((uint8_t*)&header, header_length - 1);
        
        memcpy(data_buf.buf(), &header, header_length);
        if (!body.empty()) {
            memcpy(data_buf.buf() + header_length, body.buf(), body.len());
        }
        
        std::cout << "header.crc = " << (int)header.crc << ", crc buf = " << (int)data_buf.buf()[header_length - 1] << std::endl;
        
        return data_buf;
    }
private:
    uint32_t        cmd{0};
    uint32_t        seq{0};
    bool            rsp{false};
    std::string     device_id;
    ibuffer         body;
};

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
        auto crc = asyncmsg::detail::calc_crc8(buf_valid, header_length - 1);
        std::cout << "crc cacl = " << (int)crc << ", crc origin = " << (int)header->crc << std::endl;
//        if (crc != header->crc) {
//            std::cout << "crc invalid" << std::endl;
//            ++consume_len;
//            continue;
//        }
        
        uint32_t body_len = asyncmsg::detail::network_to_host_32(header->body_len);
        
        if (body_len > max_body_length) {
            consume_len += header_length;
            continue;
        }
        
        if (buf_valid_len < header_length + body_len) {
            return nullptr;
        }
        consume_len += header_length + body_len;

        uint32_t cmd = asyncmsg::detail::network_to_host_32(header->cmd);
        uint32_t seq = asyncmsg::detail::network_to_host_32(header->seq);
        bool rsp = (header->rsp == 0) ? false : true;
        std::string device_id = (char*)header->device_id;
        return std::make_unique<packet>(cmd, rsp, device_id, seq, buf_valid + header_length, body_len);
    } while (1);
    
    return nullptr;
}

}
