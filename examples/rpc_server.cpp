#include <asyncmsg.hpp>
#include <asyncmsg/rpc/rpc_server.hpp>
#include <asio.hpp>
#include "example.pb.h"


int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);
    
    std::string push_device_id = (argc == 1) ? "" : argv[1];


    auto srv = asyncmsg::tcp::tcp_server{5555};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

//    auto add_handler = [](add_req& req) -> asyncmsg::rpc::rpc_result<add_rsp> {
//        add_rsp rsp;
//        rsp.set_result(req.left() + req.right());
//        return asyncmsg::rpc::rpc_result<add_rsp>{rsp};
//    };
//    
//    auto async_add_handler = [](add_req& req) -> asio::awaitable<asyncmsg::rpc::rpc_result<add_rsp>> {
//        add_rsp rsp;
//        rsp.set_result(req.left() + req.right());
//        co_return asyncmsg::rpc::rpc_result<add_rsp>{rsp};
//    };
//    
    
    asyncmsg::rpc::register_sync_handler<add_req, add_rsp>(io_context.get_executor(), srv, "add", [](add_req& req) -> asyncmsg::rpc::rpc_result<add_rsp> {
        add_rsp rsp;
        rsp.set_result(req.left() + req.right());
        return asyncmsg::rpc::rpc_result<add_rsp>{rsp};
    });
    asyncmsg::rpc::register_async_handler<add_req, add_rsp>(io_context.get_executor(), srv, "async_add", [](add_req& req) -> asio::awaitable<asyncmsg::rpc::rpc_result<add_rsp>> {
        add_rsp rsp;
        rsp.set_result(req.left() + req.right());
        co_return asyncmsg::rpc::rpc_result<add_rsp>{rsp};
    });

    
    auto task = [&push_device_id, &srv, &io_context]() -> asio::awaitable<void> {
        if (push_device_id.empty()) {
            co_return;
        }
        
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            try {
                timer.expires_after(std::chrono::milliseconds(10));
                co_await timer.async_wait(asio::use_awaitable);
                
                hello_push msg;
                msg.set_hello("this is a rpc push");
                co_await asyncmsg::rpc::push<hello_push>(srv, push_device_id, "hello_push", msg);
            } catch(std::exception& e) {
                asyncmsg::base::print_log("push exception = " + std::string(e.what()));
            }
        }
    };
    
    asio::co_spawn(io_context, task(), asio::detached);
    
    io_context.run();
    
    asyncmsg::base::print_log("main exit");

    return 0;
}

