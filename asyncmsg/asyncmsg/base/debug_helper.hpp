#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include <thread>

namespace asyncmsg {
namespace base {

inline static std::string get_time_string() {
    auto now = std::chrono::system_clock::now();
    uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
        - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto time_tm = localtime(&tt);
    char strTime[64] = { 0 };
    snprintf(strTime, sizeof(strTime), "%d-%02d-%02d %02d:%02d:%02d %03d", time_tm->tm_year + 1900,
        time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
        time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds);
    return strTime;
}


inline static void print_log(std::string log_content) {
#ifdef ASYNC_MSG_ENABLE_LOGGING
    std::cout << "[" << get_time_string()<< "]"  << "[" << std::this_thread::get_id() << "]" << log_content << std::endl;
#endif
}

}
}
