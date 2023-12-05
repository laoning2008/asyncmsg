#pragma once
#include <cstdint>
#include <list>
#include <unordered_map>

namespace asyncmsg {
namespace detail {

template<typename T>
class lru {
    using list_t = std::list<T>;
    using map_t = std::unordered_map<T, typename list_t::iterator>;
public:
    lru(int cap_) : cap(cap_){
    }
    
    bool exist(T v) {
        auto it = m.find(v);
        if(it == m.end()) {
            return false;
        }
        
        l.splice(l.begin(), l, it->second);
        return true;
    }
    
    void put(T v) {
        auto it = m.find(v);
        if(it != m.end()) {
            l.erase(it->second);
        }
        
        l.push_front(v);
        m[v] = l.begin();
        
        if(m.size() > cap) {
            m.erase(l.back());
            l.pop_back();
        }
    }
private:
    int cap;
    list_t l;
    map_t m;
};

}
}
