#include <asyncmsg.hpp>
#include <asyncmsg/rpc/rpc_server.hpp>
#include <asio.hpp>
#include "add.pb.h"


int main(int argc, char** argv) {
    asio::io_context io_context(std::thread::hardware_concurrency());
    asio::signal_set signals(io_context, SIGINT, SIGTERM);

    auto srv = asyncmsg::rpc::rpc_server{5555};
    
    srv.register_sync_handler<add_req, add_rsp>("add", [](add_req req) -> asyncmsg::rpc::rpc_result<add_rsp> {
        add_rsp rsp;
        rsp.set_result(req.left() + req.right());
        return rsp;
//        return asyncmsg::rpc::rpc_unexpected_result{100};
    });
    
    srv.register_async_handler<add_req, add_rsp>("async_add", [](add_req req) -> asio::awaitable<asyncmsg::rpc::rpc_result<std::unique_ptr<add_rsp>>> {
    
        auto rsp = std::make_unique<add_rsp>();
        rsp->set_result(req.left() + req.right());
        co_return asyncmsg::rpc::rpc_result<std::unique_ptr<add_rsp>>{std::move(rsp)};
//        return asyncmsg::rpc::rpc_unexpected_result{100};
    });
    
    srv.start(std::thread::hardware_concurrency());

    signals.async_wait([&](auto, auto) {
        io_context.stop();
    });

    io_context.run();
    return 0;
}

