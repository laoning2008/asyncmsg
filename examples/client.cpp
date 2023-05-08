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

std::string get_time_string() {
    auto now = std::chrono::system_clock::now();
    //通过不同精度获取相差的毫秒数
    uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
        - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto time_tm = localtime(&tt);
    char strTime[25] = { 0 };
    sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d %03d", time_tm->tm_year + 1900,
        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
        time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds);
    return strTime;
}

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";

    auto cli = std::make_shared<asyncmsg::client>("localhost", 5555, device_id);

    signals.async_wait([&](auto, auto) {
        cli = nullptr;
        io_context.stop();
    });

    auto task = [device_id, cli]() -> asio::awaitable<void> {
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(100));
            co_await timer.async_wait(asio::use_awaitable);

            uint8_t data[] = {'h', 'e', 'l', 'l', 'o', '\0'};
            auto pack = asyncmsg::packet(1, false, device_id, 0, data, sizeof(data));

            std::cout << get_time_string() << ", send req" << ", data = " << (char*)data << std::endl;

            auto rsp_pack = co_await cli->send_packet(pack, 3, 3);
            if (rsp_pack.packet_body().len() > 0) {
                std::cout << get_time_string() << ", recv rsp" << ", data = " << (char*)(rsp_pack.packet_body().buf()) << std::endl;
            } else {
                std::cout << get_time_string() << ", recv rsp failed" << std::endl;
            }
        }
    };

    asio::co_spawn(cli->get_io_context(), task(), asio::detached);
    asio::co_spawn(cli->get_io_context(), task(), asio::detached);

    io_context.run();
    return 0;
}



//#include <asio/co_spawn.hpp>
//#include <asio/detached.hpp>
//#include <asio/io_context.hpp>
//#include <asio/ip/tcp.hpp>
//#include <asio/signal_set.hpp>
//#include <asio/write.hpp>
//#include <cstdio>
//#include <asyncmsg/detail/async_schedule.hpp>
//
//using asio::ip::tcp;
//using asio::awaitable;
//using asio::co_spawn;
//using asio::detached;
//using asio::use_awaitable;
//namespace this_coro = asio::this_coro;
//
//
//asio::io_context io_context(1);
//asio::io_context io_context_io(1);
//
//awaitable<void> test2() {
//        co_await asyncmsg::schedule(io_context_io.get_executor());
//        asio::steady_timer t = asio::steady_timer (io_context_io, std::chrono::seconds(1));
//        co_await t.async_wait(asio::use_awaitable);
//    co_await asyncmsg::schedule(io_context_io.get_executor());
//
//    asio::steady_timer t2 = asio::steady_timer (io_context_io, std::chrono::seconds(1));
//    co_await t2.async_wait(asio::use_awaitable);
//
//        std::cout << "end" << std::endl;
//}
//
//awaitable<void> test() {
//    co_await test2();
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

