#pragma once
#include <functional>
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

namespace asyncmsg {
namespace base {

template<typename T>
asio::awaitable<T> do_context_aware_task(std::function<asio::awaitable<T>()> task, asio::io_context::executor_type executor) {
    auto caller_ex = co_await asio::this_coro::executor;
    
    auto result0 = co_await asio::co_spawn(executor, task, asio::use_awaitable);
    auto result1 = co_await asio::co_spawn(caller_ex, [&result0]() -> asio::awaitable<T> {
        co_return result0;
    }, asio::use_awaitable);
    
    co_return result1;
}

template<typename T>
asio::awaitable<void> do_context_aware_task(std::function<asio::awaitable<void>()> task, asio::io_context::executor_type executor) {
    auto caller_ex = co_await asio::this_coro::executor;
    
    co_await asio::co_spawn(executor, task, asio::use_awaitable);
    co_await asio::co_spawn(caller_ex, []() -> asio::awaitable<T> {
        co_return;
    }, asio::use_awaitable);
    
    co_return;
}

}}
