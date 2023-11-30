#pragma once
#include <asio/experimental/cancellation_condition.hpp>
#define wait_for_one_success wait_for_one
#include <asio/experimental/awaitable_operators.hpp>
#undef wait_for_on_success

