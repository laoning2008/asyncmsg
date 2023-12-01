#pragma once
#include <cstdint>
#include <asyncmsg/base/expected.hpp>

namespace asyncmsg { namespace rpc {

template <typename T> using rpc_result = expected<T, uint32_t>;
using rpc_unexpected_result = tl::unexpected<uint32_t>;


constexpr static uint32_t err_network = 1;
constexpr static uint32_t err_deserialize = 3;

}}

