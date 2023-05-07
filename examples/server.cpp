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

    auto srv = std::make_shared<asyncmsg::server>(5555);

    signals.async_wait([&](auto, auto) {
        srv = nullptr;
        io_context.stop();
    });


    auto task = [srv]() -> asio::awaitable<void> {
        for (;;) {
            auto req_pack = co_await srv->await_request(1);
            
            std::cout << get_time_string() << ", recv req" << ", data = " << (char*)(req_pack.packet_body().buf()) << std::endl;
            
            uint8_t data[] = {'w', 'o', 'r', 'l', 'd', '\0'};
            auto rsp_pack = asyncmsg::packet(req_pack.packet_cmd(), true, req_pack.packet_device_id(), req_pack.packet_seq(), data, sizeof(data));
            co_await srv->send_packet(rsp_pack);
            std::cout << get_time_string() << ", send rsp" << ", data = " << (char*)data << std::endl;
        }
    };
    
    asio::co_spawn(srv->get_io_context(), task(), asio::detached);

    io_context.run();

return 0;
}

