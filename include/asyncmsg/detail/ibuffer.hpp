#pragma once
#include <stdint.h>
#include <cassert>

namespace asyncmsg {
    class ibuffer {
    public:
        ibuffer(uint8_t* buf = nullptr, uint32_t len = 0, bool copy = false) {
            if (copy) {
                if (len == 0) {
                    buf_ = nullptr;
                } else {
                    assert(buf != nullptr);
                    buf_ = new uint8_t[len];
                    memcpy(buf_, buf, len);
                }
            } else {
                buf_ = buf;
            }
            
            len_ = len;
            copy_ = copy;
        }
        
        ibuffer(uint32_t len) {
            if (len > 0) {
                buf_ = new uint8_t[len];
            }
            
            len_ = len;
            copy_ = true;
        }
        
        ~ibuffer() {
            if (copy_ && buf_) {
                delete[] buf_;
            }
        }
        
        ibuffer(const ibuffer& other) : ibuffer(other.buf_, other.len_, other.copy_) {
        }

        ibuffer(ibuffer&& other)
        : buf_(nullptr)
        , len_(0)
        , copy_(false) {
            std::swap(buf_, other.buf_);
            std::swap(len_, other.len_);
            std::swap(copy_, other.copy_);
        }

        ibuffer& operator=(ibuffer& other) = delete;
        
        ibuffer& operator=(ibuffer&& other) = delete;
        
        bool empty() const {
            return len_ == 0;
        }

        uint8_t* buf() const {
            return buf_;
        }

        uint32_t len() const {
            return len_;
        }
    private:
        uint8_t* buf_;
        uint32_t len_;
        bool copy_;
    };
}
