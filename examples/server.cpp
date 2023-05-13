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
            
            asyncmsg::detail::print_log(std::string("recv req, data = ") + (char*)(req_pack.packet_body().buf()));
            
            uint8_t data[] = {'w', 'o', 'r', 'l', 'd', '\0'};
            auto rsp_pack = asyncmsg::packet(req_pack.packet_cmd(), true, req_pack.packet_seq(), data, sizeof(data), req_pack.packet_device_id());
 
            co_await srv.send_packet(rsp_pack);
            asyncmsg::detail::print_log(std::string("send rsp, data = ") + (char*)data);
        }
    };
    
    asio::co_spawn(io_context, task(), asio::detached);
    
    io_context.run();

    return 0;
}

