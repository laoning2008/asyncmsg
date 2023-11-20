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
        asio::steady_timer timer(io_context);
        for (;;) {
            timer.expires_after(std::chrono::milliseconds(1));
            co_await timer.async_wait(asio::use_awaitable);

            uint8_t data[] = {'h', 'e', 'l', 'l', 'o', '\0'};
            auto pack = asyncmsg::tcp::build_req_packet(1, data, sizeof(data));

            asyncmsg::base::print_log(std::string("send req, data = ") + (char*)data);

            auto rsp_pack_opt = co_await cli.send_packet_and_wait_rsp(pack);
            if (rsp_pack_opt == std::nullopt) {
                continue;
            }
            
            auto rsp_pack = rsp_pack_opt.value();
            asyncmsg::base::print_log(std::string("recv rsp, data = ") + (char*)(rsp_pack.body().data()));
        }
    };

    for (int i = 0; i < 10; ++i) {
        asio::co_spawn(io_context, task(), asio::detached);
    }
//    asio::co_spawn(io_context, [&]() -> asio::awaitable<void> {
//        asio::steady_timer timer(io_context);
//        timer.expires_after(std::chrono::seconds(5));
//        co_await timer.async_wait(asio::use_awaitable);
//        io_context.stop();
//    }, asio::detached);
    
    io_context.run();
    
    asyncmsg::base::print_log("main exit");
    return 0;
}
