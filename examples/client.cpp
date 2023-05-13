#include <asio.hpp>
#include <asyncmsg/client.hpp>
#include <asyncmsg/detail/debug_helper.hpp>

int main(int argc, char** argv) {
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
            auto pack = asyncmsg::packet(1, false, 0, data, sizeof(data));

            asyncmsg::detail::print_log(std::string("send req, data = ") + (char*)data);

            auto rsp_pack = co_await cli.send_packet(pack, 3, 3);
            if (rsp_pack.packet_body().len() > 0) {
                asyncmsg::detail::print_log(std::string("recv rsp, data = ") + (char*)(rsp_pack.packet_body().buf()));
            } else {
                asyncmsg::detail::print_log("recv rsp failed");
            }
        }
    };

    asio::co_spawn(io_context, task(), asio::detached);
    asio::co_spawn(io_context, task(), asio::detached);

    io_context.run();
    
    std::cout << asyncmsg::detail::get_time_string() << ", main exit" << std::endl;
    return 0;
}
