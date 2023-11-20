#pragma once
#include <functional>
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <vector>

namespace asyncmsg {
namespace base {

class async_event {
public:
    async_event(asio::io_context::executor_type executor) : executor_(executor) {
        
    }

    void raise() {
        if (!raised_) {
            raised_ = true;
            for (auto& timer : timers_) {
                timer->cancel();
            }
        }
    }
    
    asio::awaitable<void> async_wait() {
        if (!raised_) {
            auto timer = std::make_shared<asio::steady_timer>(executor_, std::chrono::steady_clock::time_point::max());
            timers_.push_back(timer);
            co_await timer->async_wait(asio::as_tuple(asio::use_awaitable));
        } else {
            co_return;
        }
    }
private:
    asio::io_context::executor_type executor_;
    std::atomic_bool raised_ = false;
    std::vector<std::shared_ptr<asio::steady_timer>> timers_;
};

}}
