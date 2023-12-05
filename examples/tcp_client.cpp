#include <asio/signal_set.hpp>
#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/base/debug_helper.hpp>
#include <asyncmsg/base/string_util.hpp>
#include <asyncmsg/rpc/rpc_client.hpp>

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = (argc == 1) ? asyncmsg::base::random_string(32) : argv[1];

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
            auto pack = asyncmsg::tcp::build_req_packet(1, data, sizeof(data), {});

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
