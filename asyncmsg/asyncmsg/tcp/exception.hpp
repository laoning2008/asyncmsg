#pragma once
#include <stdexcept>

namespace asyncmsg { namespace tcp {

class asyncmsg_error : public std::logic_error {
public:
    explicit asyncmsg_error(const std::string& __s) : logic_error(__s) {}
    explicit asyncmsg_error(const char* __s)   : logic_error(__s) {}
};


class timeout_error : public asyncmsg_error {
public:
    timeout_error() : asyncmsg_error("timeout") {}
};

class invalid_device_id_error : public asyncmsg_error {
public:
    invalid_device_id_error() : asyncmsg_error("invalid device id") {}
};

class invalid_state_error : public asyncmsg_error {
public:
    invalid_state_error() : asyncmsg_error("invalid state") {}
};

class connection_error : public asyncmsg_error {
public:
    connection_error() : asyncmsg_error("invalid state") {}
};

}}

