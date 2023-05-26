#pragma once
#include <asyncmsg/detail/base/expected.hpp>

#if __cpp_lib_expected >= 202202L && __cplusplus > 202002L
template <class T, class E>
using expected = std::expected<T, E>;

template <class T>
using unexpected = std::unexpected<T>;

using unexpect_t = std::unexpect_t;

#else
template <class T, class E>
using expected = tl::expected<T, E>;

//template <class T>
//using unexpected = tl::unexpected<T>;

using unexpect_t = tl::unexpect_t;
#endif
