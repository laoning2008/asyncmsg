#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/rpc/rpc_client.hpp>
#include <asio.hpp>
#include <google/protobuf/message.h>
#include "add.pb.h"


int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";

    asyncmsg::tcp::tcp_client cli{"localhost", 5555, device_id};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto task = [&]() -> asio::awaitable<void> {
        add_req req;
        req.set_left(1);
        req.set_right(2);
        
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(100));
            co_await timer.async_wait(asio::use_awaitable);

            auto result = co_await asyncmsg::rpc::call<add_rsp>(cli, "add", req);
            if (!result) {
                asyncmsg::base::print_log("call error = " + std::to_string(result.error()));
            } else {
                asyncmsg::base::print_log("call result = " + std::to_string(result.value().result()));

            }
        }
    };

    asio::co_spawn(io_context, task(), asio::detached);
//    asio::co_spawn(io_context, task(), asio::detached);

    io_context.run();
    
    std::cout << asyncmsg::base::get_time_string() << ", main exit" << std::endl;
    return 0;
}
