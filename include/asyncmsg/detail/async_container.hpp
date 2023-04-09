#pragma once
#include <unordered_map>
#include <list>
#include <map>
#include <memory>

#include <async_simple/coro/ConditionVariable.h>
#include <async_simple/coro/Sleep.h>
#include <async_simple/coro/Lazy.h>

#include <asyncmsg/packet.hpp>


namespace asyncmsg {

template<typename T>
class async_value {
public:
    void send(T v) {
        if (has_set_value) {
            return;
        }
        
        value = std::move(v);
        has_set_value = true;
        notifier.notify();
    }
    
    void send() {
        if (has_set_value) {
            return;
        }
        
        notifier.notify();
    }
    
    async_simple::coro::Lazy<T> recv() {
        if (has_set_value) {
            co_return value;
        }
        
        co_await notifier.wait();
        notifier.reset();
        co_return value;
    }
private:
    bool has_set_value{false};
    T value;
    async_simple::coro::Notifier notifier;
};


template<typename REQ, typename RSP>
struct async_request {
    REQ requst_packet;
    std::weak_ptr<async_value<RSP>> responser;
};

template<typename REQ, typename RSP>
class async_request_producer_consumer {
public:
    using async_request_type = async_request<REQ, RSP>;
    using container_t = std::list<async_request_type>;
public:
    async_request_producer_consumer() {

    }
    
    async_simple::coro::Lazy<RSP> product_request(REQ req) {
        auto responser = std::make_shared<async_value<RSP>>();
        async_request<REQ, RSP> request = {req, responser};
        container.push_back(std::move(request));
        notifier.notify();
        
        co_return co_await responser->recv();
    }
    
    async_simple::coro::Lazy<async_request_type> consume_request() {
        if (!container.empty()) {
            auto pack = container.front();
            container.pop_front();
            co_return pack;
        }

        co_await notifier.wait();
        notifier.reset();
        auto pack = container.front();
        container.pop_front();
        co_return pack;
    }

private:
    container_t container;
    async_simple::coro::Notifier notifier;
};


template<typename REQ>
class async_request_producer_consumer_simple {
    using container_t = std::list<REQ>;
public:
    async_request_producer_consumer_simple() {

    }
    
    void product(REQ pack) {
        if (pack == nullptr){
            std::cout << "pack == nullptr" << std::endl;
        }
        container.push_back(pack);
        notifier.notify();
    }
    
    async_simple::coro::Lazy<REQ> consume() {
        if (!container.empty()) {
            auto pack = container.front();
            container.pop_front();
            co_return pack;
        }
        
        co_await notifier.wait();
        notifier.reset();
        if (container.empty()) {
            std::cout << "pack == nullptr" << std::endl;
        }
        auto pack = container.front();
        container.pop_front();
        co_return pack;
    }

private:
    container_t container;
    async_simple::coro::Notifier notifier;
};


class packet_producer_consumer_by_cmd {
    using container_t = std::unordered_map<uint32_t, std::pair<std::list<std::shared_ptr<packet>>, std::unique_ptr<async_simple::coro::Notifier>>>;
public:
    packet_producer_consumer_by_cmd() {

    }
    
    void product(std::shared_ptr<packet> pack) {
        auto cmd = pack->cmd;
        auto it = container.find(cmd);
        if (it == container.end()) {
            container[cmd] = {std::list<std::shared_ptr<packet>>{}, std::make_unique<async_simple::coro::Notifier>()};
        }
        
        container[cmd].first.push_back(pack);
        container[cmd].second->notify();
    }
    
    async_simple::coro::Lazy<std::shared_ptr<packet>> consume(uint32_t cmd) {
        auto it = container.find(cmd);
        if (it == container.end()) {
            container[cmd] = {std::list<std::shared_ptr<packet>>{}, std::make_unique<async_simple::coro::Notifier>()};
        }
        
        if (!container[cmd].first.empty()) {
            auto pack = container[cmd].first.front();
            container[cmd].first.pop_front();
            co_return pack;
        }
        
        co_await container[cmd].second->wait();
        
        auto pack = container[cmd].first.front();
        container[cmd].first.pop_front();
        co_return pack;
    }

private:
    container_t container;
};


}
