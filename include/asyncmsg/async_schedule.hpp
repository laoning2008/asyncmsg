#pragma once

#include <asio/use_awaitable.hpp>

namespace asio::awaitable_ext {

[[nodiscard]] inline auto schedule(any_io_executor executor) -> awaitable<void> {
    auto initiate = [executor]<typename Handler>(Handler&& handler) mutable {
        post(executor, [handler = std::forward<Handler>(handler)]() mutable {
            handler();
        });
    };

    return async_initiate<decltype(use_awaitable), void()>(initiate, use_awaitable);
}

}
