#include <asyncmsg/rpc/rpc_client.hpp>
#include <asyncmsg/base/string_util.hpp>
#include <asio/signal_set.hpp>
#include "example.pb.h"


int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";//asyncmsg::base::random_string(32);

    asyncmsg::tcp::tcp_client cli{"localhost", 5555, device_id};
    
    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto call_task = [&]() -> asio::awaitable<void> {
        add_req req;
        req.set_left(1);
        req.set_right(2);
        
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(1000));
            co_await timer.async_wait(asio::use_awaitable);

            try {
                auto result = co_await asyncmsg::rpc::call<add_rsp>(cli, "add", req);
                if (!result) {
                    asyncmsg::base::print_log("call add(1,2) error = " + std::to_string(result.error()));
                } else {
                    asyncmsg::base::print_log("call add(1,2) result = " + std::to_string(result.value().result()));
                }
            } catch (std::exception& e) {
                asyncmsg::base::print_log("call exception = " + std::string(e.what()));
            }
        }
    };
    
    auto push_task = [&]() -> asio::awaitable<void> {
        for (;;) {
            auto hell_msg = co_await asyncmsg::rpc::wait_push<hello_push>(cli, "hello_push");
            asyncmsg::base::print_log("recv server push, msg = " + hell_msg.hello());
        }
    };
    
    
    asio::co_spawn(io_context, call_task(), asio::detached);
    asio::co_spawn(io_context, push_task(), asio::detached);

    io_context.run();
    
    std::cout << asyncmsg::base::get_time_string() << ", main exit" << std::endl;
    return 0;
}
