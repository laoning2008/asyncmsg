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

## Server Example
Here's an example of how to use the AsyncMsg to create a simple server that listens for incoming requests on port 5555, and send responses back:

```cpp
#include <asio.hpp>
#include <asyncmsg/server.hpp>
#include <asyncmsg/detail/debug_helper.hpp>

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    auto srv = asyncmsg::server{5555};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto task = [&]() -> asio::awaitable<void> {
        for (;;) {
            auto req_pack = co_await srv.await_request(1);
            if (!req_pack.is_valid()) {
                continue;
            }
            
            std::cout << asyncmsg::detail::get_time_string() << ", recv req" << ", data = " << (char*)(req_pack.packet_body().buf()) << std::endl;
            
            uint8_t data[] = {'w', 'o', 'r', 'l', 'd', '\0'};
            auto rsp_pack = asyncmsg::packet(req_pack.packet_cmd(), true, req_pack.packet_device_id(), req_pack.packet_seq(), data, sizeof(data));
 
            co_await srv.send_packet(rsp_pack);
            std::cout << asyncmsg::detail::get_time_string() << ", send rsp" << ", data = " << (char*)data << std::endl;
        }
    };
    
    asio::co_spawn(io_context, task(), asio::detached);
    
    io_context.run();

    return 0;
}
```


## Client Example
Here's an example of how to use the AsyncMsg library to create a simple client that sends requests to the server:

```cpp
#include <asio.hpp>
#include <asyncmsg/client.hpp>
#include <asyncmsg/detail/debug_helper.hpp>

nt main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";

    asyncmsg::client cli{"localhost", 5555, device_id};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto task = [&]() -> asio::awaitable<void> {
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(100));
            co_await timer.async_wait(asio::use_awaitable);

            uint8_t data[] = {'h', 'e', 'l', 'l', 'o', '\0'};
            auto pack = asyncmsg::packet(1, false, device_id, 0, data, sizeof(data));

            std::cout << asyncmsg::detail::get_time_string() << ", send req" << ", data = " << (char*)data << std::endl;

            auto rsp_pack = co_await cli.send_packet(pack, 3, 3);
            if (rsp_pack.packet_body().len() > 0) {
                std::cout << asyncmsg::detail::get_time_string() << ", recv rsp" << ", data = " << (char*)(rsp_pack.packet_body().buf()) << std::endl;
            } else {
                std::cout << asyncmsg::detail::get_time_string() << ", recv rsp failed" << std::endl;
            }
        }
    };

    asio::co_spawn(io_context, task(), asio::detached);
    asio::co_spawn(io_context, task(), asio::detached);

    io_context.run();
    
    return 0;
}
```
