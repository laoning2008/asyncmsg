#include <asyncmsg.hpp>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>


int main(int argc, char** argv) {
    try {
        asio::io_context io_context(std::thread::hardware_concurrency());
        asio::signal_set signals(io_context, SIGINT, SIGTERM);

        auto srv = std::make_shared<asyncmsg::server>(5555);

        signals.async_wait([&](auto, auto) {
            srv = nullptr;
            io_context.stop();
        });
        
        
        auto task = [&srv]() -> async_simple::coro::Lazy<void> {
            for (;;) {
                auto pack_recv = co_await srv->await_request(1);
                if (pack_recv) {
                    std::cout << "recv req, cmd = " << pack_recv->cmd << ", seq = " << pack_recv->seq << std::endl;
                } else {
                    std::cout << "recv req = null" << std::endl;
                }
                
                pack_recv->rsp = true;
                co_await srv->send_packet(pack_recv);
            }
        };
        std::thread t([task]() {
            async_simple::coro::syncAwait(task());
        });
        
        io_context.run();
    }
    catch (std::exception& e) {
      std::printf("Exception: %s\n", e.what());
    }
   
    return 0;
}
