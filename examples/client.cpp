#include <asyncmsg.hpp>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <async_simple/coro/Sleep.h>
#include <async_simple/coro/Lazy.h>


int main(int argc, char** argv) {
    try {
        asio::io_context io_context(std::thread::hardware_concurrency());
        asio::signal_set signals(io_context, SIGINT, SIGTERM);

        auto cli = std::make_shared<asyncmsg::client>("localhost", 5555, "test_device_id");

        signals.async_wait([&](auto, auto) {
            cli = nullptr;
            io_context.stop();
        });
        
        auto task = [&cli]() -> async_simple::coro::Lazy<void> {
            for (;;) {
                co_await async_simple::coro::sleep(std::chrono::seconds(3));
                std::cout << "send pack" << std::endl;
                auto pack = asyncmsg::packet::build_packet(1, false, "test_device_id", nullptr, 0);
                auto pack_recv = co_await cli->send_packet(pack);
                if (pack_recv) {
                    std::cout << "recv rsp, cmd = " << pack_recv->cmd << ", seq = " << pack_recv->seq << std::endl;
                } else {
                    std::cout << "rsp null" << std::endl;
                }
            }
        };
        std::thread t([task]() {
            async_simple::coro::syncAwait(task());
        });
        
        
        
        
        io_context.run();
        
        t.join();
    }
    catch (std::exception& e) {
      std::printf("Exception: %s\n", e.what());
    }
   
    return 0;
}
