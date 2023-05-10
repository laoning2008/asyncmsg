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

    auto cli = std::make_unique<asyncmsg::client>("localhost", 5555, device_id);

    signals.async_wait([&](auto, auto) {
        asio::post(cli->get_io_context(), [&]() {
            cli = nullptr;
        });
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

            if (!cli) {
                break;
            }
            auto rsp_pack = co_await cli->send_packet(pack, 3, 3);
            if (rsp_pack.packet_body().len() > 0) {
                std::cout << asyncmsg::detail::get_time_string() << ", recv rsp" << ", data = " << (char*)(rsp_pack.packet_body().buf()) << std::endl;
            } else {
                std::cout << asyncmsg::detail::get_time_string() << ", recv rsp failed" << std::endl;
            }
        }
    };

    asio::co_spawn(cli->get_io_context(), task(), asio::detached);
    asio::co_spawn(cli->get_io_context(), task(), asio::detached);

    io_context.run();
    return 0;
}
