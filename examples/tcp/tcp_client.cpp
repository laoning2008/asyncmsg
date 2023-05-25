#include <asio.hpp>
#include <asyncmsg/tcp/tcp_client.hpp>
#include <asyncmsg/base/debug_helper.hpp>

int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    std::string device_id = "test_device_id";

    asyncmsg::tcp::tcp_client cli{"localhost", 5555, device_id};

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    auto task = [&]() -> asio::awaitable<void> {
        asio::steady_timer timer(co_await asio::this_coro::executor);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(1));
            co_await timer.async_wait(asio::use_awaitable);

            uint8_t data[] = {'h', 'e', 'l', 'l', 'o', '\0'};
            auto pack = asyncmsg::tcp::build_req_packet(1, data, sizeof(data));

            asyncmsg::base::print_log(std::string("send req, data = ") + (char*)data);

            auto rsp_pack = co_await cli.send_packet_with_retry(pack);
            if (rsp_pack.packet_body().size() > 0) {
                asyncmsg::base::print_log(std::string("recv rsp, data = ") + (char*)(rsp_pack.packet_body().data()));
            } else {
                asyncmsg::base::print_log("recv rsp failed");
            }
        }
    };

    for (int i = 0; i < 1000; ++i) {
        asio::co_spawn(io_context, task(), asio::detached);
    }

    io_context.run();
    
    std::cout << asyncmsg::base::get_time_string() << ", main exit" << std::endl;
    return 0;
}
