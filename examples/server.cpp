//#include <asyncmsg.hpp>
#include <thread>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asyncmsg/oneshot.hpp>
#include <asio.hpp>
#include <iostream>
#include <asyncmsg/packet.hpp>
#include <asyncmsg/ibuffer.hpp>
#include <asio/experimental/channel.hpp>

//int main(int argc, char** argv) {
//    try {
//        asio::io_context io_context(std::thread::hardware_concurrency());
//        asio::signal_set signals(io_context, SIGINT, SIGTERM);
//
////        auto srv = std::make_shared<asyncmsg::server>(5555);
//
//        signals.async_wait([&](auto, auto) {
////            srv = nullptr;
//            io_context.stop();
//        });
//
//
////        auto task = [&srv]() -> async_simple::coro::Lazy<void> {
////            for (;;) {
//////                auto pack_recv = co_await srv->await_request(1);
//////                if (pack_recv) {
//////                    std::cout << "recv req, cmd = " << pack_recv->cmd << ", seq = " << pack_recv->seq << std::endl;
//////                } else {
//////                    std::cout << "recv req = null" << std::endl;
//////                }
//////
//////                pack_recv->rsp = true;
//////                co_await srv->send_packet(pack_recv);
////                std::this_thread::sleep_for(std::chrono::seconds(10));
////                srv.reset();
////                co_return;
////            }
////        };
////        std::thread t([task]() {
////            async_simple::coro::syncAwait(task());
////        });
//
//        io_context.run();
//    }
//    catch (std::exception& e) {
//      std::printf("Exception: %s\n", e.what());
//    }
//
//    return 0;
//}


using packet_channel = asio::experimental::channel<void(asio::error_code, asyncmsg::packet&&)>;


//asio::awaitable<void> sender_task(oneshot::sender<std::shared_ptr<asyncmsg::packet>> s)
//{
//    auto pack = asyncmsg::packet::build_packet(1, false, "test_device_id", nullptr, 0);
//    s.send(pack);
//    co_return;
//}
//
//asio::awaitable<void> receiver_task(oneshot::receiver<std::shared_ptr<asyncmsg::packet>> r)
//{
//    co_await r.async_wait(asio::deferred);
//    std::cout << "The result:" << r.get()->device_id << std::endl;
//}

asio::awaitable<void> sender_task(packet_channel& c)
{
    auto pack = asyncmsg::packet{1, false, "test_device_id"};
//    std::cout << "address = " << reinterpret_cast<uint64_t>(&pack.device_id.data()[0]) << std::endl;

    co_await c.async_send(asio::error_code{}, std::move(pack), asio::use_awaitable);
}

asio::awaitable<void> receiver_task(packet_channel& c)
{
    auto&& pack = co_await c.async_receive(asio::use_awaitable);
    std::cout << "The result:" << pack.packet_device_id() << std::endl;
}

int main()
{
    auto ctx = asio::io_context{};

//    auto [s, r] = oneshot::create<std::shared_ptr<asyncmsg::packet>>();
//
//    asio::co_spawn(ctx, sender_task(std::move(s)), asio::detached);
//    asio::co_spawn(ctx, receiver_task(std::move(r)), asio::detached);


    packet_channel c(ctx, 1);
    asio::co_spawn(ctx, sender_task(c), asio::detached);
    asio::co_spawn(ctx, receiver_task(c), asio::detached);
    
    ctx.run();
}
