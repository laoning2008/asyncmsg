//#define ASIO_ENABLE_HANDLER_TRACKING
#include <asyncmsg/client.hpp>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <iostream>
#include <locale>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <asyncmsg/detail/debug_helper.hpp>

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";

    asyncmsg::client cli{"localhost", 5555, device_id};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
//        auto task = [&]() -> asio::awaitable<void> {
//            co_await cli.stop();
//        };
//        asio::co_spawn(io_context, task(), asio::detached);
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
    
    std::cout << asyncmsg::detail::get_time_string() << ", main exit" << std::endl;
    return 0;
}


//
//#include <asio/co_spawn.hpp>
//#include <asio/detached.hpp>
//#include <asio/io_context.hpp>
//#include <asio/ip/tcp.hpp>
//#include <asio/signal_set.hpp>
//#include <asio/write.hpp>
//#include <cstdio>
//#include <asyncmsg/detail/async_schedule.hpp>
//#include <asio/bind_executor.hpp>
//
//using asio::ip::tcp;
//using asio::awaitable;
//using asio::co_spawn;
//using asio::detached;
//using asio::use_awaitable;
//namespace this_coro = asio::this_coro;
////constexpr auto use_nothrow_awaitable = asio::as_tuple(asio::use_awaitable);
//
//asio::io_context io_context(1);
//asio::io_context io_context_io(1);
//
//auto bind_io_context = asio::bind_executor(io_context_io, use_nothrow_awaitable);
//
//awaitable<int> test2() {
////    co_await asio::post(bind_io_context);
//    auto f = []() -> awaitable<int> {
//        asio::steady_timer t = asio::steady_timer (io_context_io, std::chrono::seconds(1));
//    //    co_await t.async_wait(bind_io_context);
//        co_await t.async_wait(use_nothrow_awaitable);
//
//        asio::steady_timer t2 = asio::steady_timer (io_context_io, std::chrono::seconds(1));
//        co_await t2.async_wait(use_nothrow_awaitable);
//
//        std::cout << "end" << std::endl;
//
//        co_return 1;
//    };
//
//    co_return co_await asio::co_spawn(io_context_io, f(), asio::use_awaitable);
//}
//
//awaitable<void> test() {
////    auto d = co_await asio::co_spawn(io_context_io, test2(), asio::use_awaitable);
////    std::cout << d << std::endl;
//    auto d = co_await test2();
//    std::cout << d << std::endl;
//}
//
//int main()
//{
//  try
//  {
//    asio::signal_set signals(io_context, SIGINT, SIGTERM);
//    signals.async_wait([&](auto, auto){ io_context.stop(); });
//
//
//      auto t = std::thread([&]() {
//          io_context_io.run();
//          std::printf("thread end\n");
//      });
//
//      asio::executor_work_guard<asio::io_context::executor_type> work_guard(io_context_io.get_executor());
//
//
//      co_spawn(io_context, test(), detached);
//
//    io_context.run();
//
//      return 0;
//  }
//  catch (std::exception& e)
//  {
//    std::printf("Exception: %s\n", e.what());
//  }
//}
