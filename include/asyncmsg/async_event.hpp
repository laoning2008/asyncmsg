#pragma once

#include <asio/use_awaitable.hpp>

namespace asio::awaitable_ext {

class async_event {
    enum class state { not_set, not_set_consumer_waiting, set };
    mutable std::atomic<state> _state;
    mutable std::move_only_function<void()> _handler;

public:
    async_event() : _state{State::not_set} {}

    async_event(const async_event&) = delete;
    async_event& operator=(const async_event&) = delete;

    [[nodiscard]] awaitable<void> wait(any_io_executor executor) const {
        auto initiate = [this, executor]<typename Handler>(Handler&& handler) mutable {
            this->_handler = [executor, handler = std::forward<Handler>(handler)]() mutable {
                post(executor, std::move(handler));
            };

            state oldState = state::not_set;
            const bool isWaiting = _state.compare_exchange_strong(
                oldState,
                state::not_set_consumer_waiting,
                std::memory_order_release,
                std::memory_order_relaxed);

            if (!isWaiting) {
                this->_handler();
            }
        };

        return async_initiate<decltype(use_awaitable), void()>(initiate, use_awaitable);
    }

    void set() {
        const state oldState = _state.exchange(state::set, std::memory_order_acquire);
        if (oldState == state::not_set_consumer_waiting) {
            _handler();
        }
    }
};

}
