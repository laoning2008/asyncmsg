#pragma once

#include <asio/use_awaitable.hpp>

namespace asyncmsg {

[[nodiscard]] inline auto schedule(asio::any_io_executor executor) -> asio::awaitable<void> {
    auto initiate = [executor]<typename Handler>(Handler&& handler) mutable {
        asio::post(executor, [handler = std::forward<Handler>(handler)]() mutable {
            handler();
        });
    };

    return async_initiate<decltype(asio::use_awaitable), void()>(initiate, asio::use_awaitable);
}

}
