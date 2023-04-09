#pragma once
#include <memory>
#include <string>
#include <atomic>

#include <asyncmsg/detail/crc.hpp>
#include <asyncmsg/detail/byte_order.hpp>

namespace asyncmsg {

class packet {
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
public:
    
    packet(uint32_t cmd_, uint32_t seq_, bool rsp_, const std::string device_id_, uint8_t* body_buf_, uint32_t body_len_)
    : cmd(cmd_), seq(seq_), rsp(rsp_), device_id(std::move(device_id_)), body_length(body_len_) {
        packet_header header;
        header.flag = packet_begin_flag;
        header.cmd = asyncmsg::detail::host_to_network_32(cmd);
        header.seq = asyncmsg::detail::host_to_network_32(seq);
        header.rsp = rsp ? 1 : 0;

        memset(header.device_id, 0, sizeof(header.device_id));
        memcpy(header.device_id, device_id.c_str(), device_id.size());
        
        header.body_len = asyncmsg::detail::host_to_network_32(body_length);
        header.crc = asyncmsg::detail::calc_crc8((uint8_t*)&header, header_length - 1);
        
        memcpy(data, &header, header_length);
        if ((body_buf_ != nullptr) && (body_len_ > 0)) {
            memcpy(data + header_length, body_buf_, body_len_);
        }
    }
    ~packet() = default;

    const uint8_t* packet_body() const {
        return data+ header_length;
    }
    
    uint32_t packet_body_length() const {
        return body_length;
    }
    
    const uint8_t* packet_data() const {
        return data;
    }
    
    uint32_t packet_data_length() const {
        return header_length + body_length;
    }
    
public:
    packet(const packet& other) = delete;
    packet(packet&& other) = delete;
    packet& operator=(const packet& other) = delete;
    packet& operator=(packet&& other) = delete;
    
public:
    static std::shared_ptr<packet> build_packet(uint32_t cmd, uint32_t seq, bool rsp, const std::string& device_id, uint8_t* body_buf, uint32_t body_len) {
        if (header_length + body_len > max_body_length) {
            return nullptr;
        }
        
        if (device_id.size() >= device_id_size) {
            return nullptr;
        }
                
        return std::make_shared<packet>(cmd, seq, rsp, device_id, body_buf, body_len);
    }
    
    static std::shared_ptr<packet> build_packet(uint32_t cmd, bool rsp, const std::string& device_id, uint8_t* body_buf, uint32_t body_len) {
        return build_packet(cmd, ++seq_generator, rsp, device_id, body_buf, body_len);
    }

    static std::shared_ptr<packet> parse_packet(uint8_t* buf, size_t buf_len, size_t& consume_len)
    {
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
            if (crc != header->crc) {
                ++consume_len;
                continue;
            }
            
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
            return std::make_shared<packet>(cmd, seq, rsp, device_id, buf_valid + header_length, body_len);
        } while (1);
        
        return nullptr;
    }
    
  public:
    constexpr static uint32_t max_body_length = 16*1024;
    static std::atomic<uint32_t> seq_generator;
    
    uint8_t         data[max_body_length + header_length] = {0};
    uint32_t        cmd{0};
    uint32_t        seq{0};
    bool            rsp{false};
    std::string     device_id;
    uint32_t        body_length{0};
};

std::atomic<uint32_t> packet::seq_generator{0};

}

