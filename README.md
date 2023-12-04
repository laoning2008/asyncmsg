# AsyncMsg

The AsyncMsg is a C++ library that provides a simple API for asynchronous message passing over TCP sockets. It is built on top of the asio library, and provides a lightweight and efficient way to implement server and client applications that communicate over a network.

## Integration
you can use CMake to integrate the asyncmsg into your project. Here is an example CMakeLists.txt file that demonstrates how to do this:

```cmake
cmake_minimum_required(VERSION 3.5)
project(my_project)

add_subdirectory(asyncmsg)

add_executable(my_executable main.cpp)
target_link_libraries(my_executable asyncmsg)
```

## TCP Server Example
Here's an example of how to use the AsyncMsg to create a simple tcp server that listens for incoming requests on port 5555, and send responses back:

```cpp
#include <asio.hpp>
#include <asyncmsg/tcp/tcp_server.hpp>
#include <asyncmsg/base/debug_helper.hpp>

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    auto srv = asyncmsg::tcp::tcp_server{5555};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });


    auto task = [&]() -> asio::awaitable<void> {
        for (;;) {
            auto req_pack = co_await srv.wait_request(1);
                        
            asyncmsg::base::print_log(std::string("recv req, data = ") + (char*)(req_pack.body().data()));
            
            uint8_t data[] = {'w', 'o', 'r', 'l', 'd', '\0'};
            auto rsp_pack = asyncmsg::tcp::build_rsp_packet(req_pack.cmd(), req_pack.seq(), 0, req_pack.device_id(), data, sizeof(data));
 
            co_await srv.send_packet(rsp_pack);
        }
    };
    
    asio::co_spawn(io_context, task(), asio::detached);
    
    io_context.run();
    
    asyncmsg::base::print_log("main exit");

    return 0;
}
```


## TCP Client Example
Here's an example of how to use the AsyncMsg library to create a simple tcp client that sends requests to the server:

```cpp
#include <asio/signal_set.hpp>
#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asyncmsg/base/string_util.hpp>
#include <asyncmsg/rpc/rpc_client.hpp>

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = asyncmsg::base::random_string(32);

    asyncmsg::tcp::tcp_client cli{"localhost", 5555, device_id};
    
    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto task = [&]() -> asio::awaitable<void> {
        asio::steady_timer timer(io_context);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(1));
            co_await timer.async_wait(asio::use_awaitable);

            uint8_t data[] = {'h', 'e', 'l', 'l', 'o', '\0'};
            auto pack = asyncmsg::tcp::build_req_packet(1, data, sizeof(data));

            asyncmsg::base::print_log(std::string("send req, data = ") + (char*)data);

            auto rsp = co_await cli.send_packet_and_wait_rsp(pack);

            asyncmsg::base::print_log(std::string("recv rsp, data = ") + (char*)(rsp.body().data()));
        }
    };

    for (int i = 0; i < 10; ++i) {
        asio::co_spawn(io_context, task(), asio::detached);
    }
    
    io_context.run();
    asyncmsg::base::print_log("main exit");
    return 0;
}
```
## RPC Server Example

```cpp
#include <asyncmsg.hpp>
#include <asyncmsg/rpc/rpc_server.hpp>
#include <asio.hpp>
#include "example.pb.h"


int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    auto srv = asyncmsg::tcp::tcp_server{5555};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    std::function<asyncmsg::rpc::rpc_result<add_rsp>(add_req&)> add_handler = [](add_req& req) -> asyncmsg::rpc::rpc_result<add_rsp> {
        add_rsp rsp;
        rsp.set_result(req.left() + req.right());
        return asyncmsg::rpc::rpc_result<add_rsp>{rsp};
    };

    auto task = [&srv, &io_context, add_handler]() -> asio::awaitable<void> {
        asyncmsg::rpc::register_sync_handler<add_req, add_rsp>(io_context.get_executor(), srv, "add", add_handler);
        
        std::string device_id = "test_device_id";
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            try {
                timer.expires_after(std::chrono::milliseconds(100));
                co_await timer.async_wait(asio::use_awaitable);
                
                hello_push msg;
                msg.set_hello("this is a rpc push");
                co_await asyncmsg::rpc::push<hello_push>(srv, device_id, "hello_push", msg);
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
```

## RPC Client Example

```cpp
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
```