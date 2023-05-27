#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/rpc/rpc_client.hpp>
#include <asio.hpp>
#include <google/protobuf/message.h>
#include "add.pb.h"


int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";

    asyncmsg::tcp::tcp_client cli{"localhost", 5556, device_id};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto sync_call_task = [&]() -> asio::awaitable<void> {
        add_req req;
        req.set_left(1);
        req.set_right(2);
        
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(1000));
            co_await timer.async_wait(asio::use_awaitable);

            auto result = co_await asyncmsg::rpc::call<add_rsp>(cli, "add", req);
            if (!result) {
                asyncmsg::base::print_log("call add(1,2) error = " + std::to_string(result.error()));
            } else {
                asyncmsg::base::print_log("call add(1,2) result = " + std::to_string(result.value().result()));
            }
        }
    };
    
    auto async_call_task = [&]() -> asio::awaitable<void> {
        add_req req;
        req.set_left(1);
        req.set_right(2);
        
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(1));
            co_await timer.async_wait(asio::use_awaitable);
            
            auto result = co_await asyncmsg::rpc::call<add_rsp>(cli, "async_add", req);
            if (!result) {
                asyncmsg::base::print_log("call async_add(1,2) error = " + std::to_string(result.error()));
            } else {
                asyncmsg::base::print_log("call async_add(1,2) result = " + std::to_string(result.value().result()));
            }
        }
    };

    for (int i = 0; i < 200; ++i) {
        asio::co_spawn(io_context, sync_call_task(), asio::detached);
    }
    
    for (int i = 0; i < 200; ++i) {
        asio::co_spawn(io_context, async_call_task(), asio::detached);
    }


    io_context.run();
    
    std::cout << asyncmsg::base::get_time_string() << ", main exit" << std::endl;
    return 0;
}
