#pragma once
#include <functional>
#include <asio/use_awaitable.hpp>
#include <asio/async_result.hpp>
#include <asio/associated_cancellation_slot.hpp>
#include <asio/error.hpp>

#include "asyncmsg/base/function2.hpp"
using std::system;

namespace asyncmsg {
namespace base {

class async_event {
    enum class State { not_set, not_set_consumer_waiting, set, canceled };
    mutable std::atomic<State> _state;
    mutable fu2::unique_function<void(asio::error_code)> _handler;

public:
    async_event() : _state{ State::not_set } {}

    async_event(const async_event&) = delete;
    async_event& operator=(const async_event&) = delete;

    template<asio::completion_token_for<void(asio::error_code)> CompletionToken>
    auto wait(CompletionToken&& completionToken) const {
        auto initiate = [this](auto&& handler) mutable
        {
            auto slot = get_associated_cancellation_slot(handler, asio::cancellation_slot());
            if (slot.is_connected()) {
                slot.assign([this](asio::cancellation_type) { const_cast<async_event*>(this)->cancel(); });
            }

            this->_handler = [executor = get_associated_executor(handler),
                handler = std::move(handler)](asio::error_code ec) mutable
            {
                auto wrap = [handler = std::move(handler), ec]() mutable
                {
                    handler(ec);
                };
                post(executor, std::move(wrap));
            };

            State oldState = State::not_set;
            const bool isWaiting = _state.compare_exchange_strong(
                oldState,
                State::not_set_consumer_waiting,
                std::memory_order_release,
                std::memory_order_acquire); // see side-effects from thread calling set()

            if (!isWaiting) {
                auto ec = (oldState == State::canceled) ? asio::error_code{ asio::error::operation_aborted }
                : asio::error_code{}; // not error
                this->_handler(ec);
            }
        };

        return async_initiate<
            CompletionToken, void(asio::error_code)>(
                initiate, completionToken);
    }

    void set() {
        State oldState = State::not_set;
        bool isSet = _state.compare_exchange_strong(
            oldState,
            State::set,
            std::memory_order_release,
            std::memory_order_acquire); // see change of handler if current state is not_set_consumer_waiting

        if (isSet) {
            return; // set before wait
        }
        else if (oldState == State::not_set_consumer_waiting) {
            // wait win
            isSet = _state.compare_exchange_strong(
                oldState,
                State::set,
                std::memory_order_relaxed,
                std::memory_order_relaxed);

            if (isSet) {
                auto dummy = asio::error_code{}; // not error
                _handler(dummy); // set after wait
                return;
            }
        }

        assert(oldState == State::canceled); // cancel before set and wait
    }

    void cancel() {
        State oldState = State::not_set;
        bool isCancel = _state.compare_exchange_strong(
            oldState,
            State::canceled,
            std::memory_order_release,
            std::memory_order_acquire); // see change of handler if current state is not_set_consumer_waiting

        if (isCancel) {
            return; // cancel before wait
        }
        else if (oldState == State::not_set_consumer_waiting) {
            // wait win
            isCancel = _state.compare_exchange_strong(
                oldState,
                State::canceled,
                std::memory_order_relaxed,
                std::memory_order_relaxed);

            if (isCancel) {
                asio::error_code ec = asio::error::operation_aborted;
                _handler(ec); // cancel after wait, but before
                return;
            }
        }

        assert(oldState == State::set); // set before wait and cancel
    }
};

}}
