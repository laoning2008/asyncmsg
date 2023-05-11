//#include <asyncmsg.hpp>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio.hpp>
#include <iostream>
#include <asyncmsg/server.hpp>
#include <locale>
#include <iomanip>
#include <ctime>
#include <sstream>
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
            
            std::cout << asyncmsg::detail::get_time_string() << ", recv req" << ", data = " << (char*)(req_pack.packet_body().buf()) << std::endl;
            
            uint8_t data[] = {'w', 'o', 'r', 'l', 'd', '\0'};
            auto rsp_pack = asyncmsg::packet(req_pack.packet_cmd(), true, req_pack.packet_device_id(), req_pack.packet_seq(), data, sizeof(data));
 
            co_await srv.send_packet(rsp_pack);
            std::cout << asyncmsg::detail::get_time_string() << ", send rsp" << ", data = " << (char*)data << std::endl;
        }
    };
    
    asio::co_spawn(io_context, task(), asio::detached);
    
    io_context.run();

    return 0;
}

