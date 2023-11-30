#pragma once
#include <unordered_map>
#include <asio/cancellation_signal.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/awaitable.hpp>

namespace asyncmsg {
namespace base {

static std::atomic<uint32_t> cancellation_id_generator{0};


class asio_cancellation {
public:
    ~asio_cancellation() {
        emit_all();
    }
    
    uint32_t add_cancellation_signal() {
        auto id = ++cancellation_id_generator;
        cancellation_signals.emplace(id, std::make_unique<asio::cancellation_signal>());
        return id;
    }
    
    void remove_cancellation_signal(uint32_t id) {
        cancellation_signals.erase(id);
    }
    
    auto bind_cancellation_slot(uint32_t id) {
        return asio::bind_cancellation_slot(cancellation_signals[id]->slot(), asio::use_awaitable);
    }
    
    void emit_all() {
        for (auto& [id, signal] : cancellation_signals) {
            signal->emit(asio::cancellation_type::all);
        }
    }
    
private:
    std::unordered_map<uint32_t, std::unique_ptr<asio::cancellation_signal>> cancellation_signals;
};

}}
