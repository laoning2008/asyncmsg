cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asyncmsg CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

option(ASYNC_MSG_ENABLE_LOGGING "enable log" ON)
set(ASIO_SOURCE_DIR "" CACHE STRING "asio source path")
set(PROTOBUF_SOURCE_DIR "" CACHE STRING "protobuf source path")


if(ASIO_SOURCE_DIR STREQUAL "")
    CPMAddPackage(NAME ASIO GITHUB_REPOSITORY chriskohlhoff/asio GIT_TAG asio-1-28-2)
endif()

if(PROTOBUF_SOURCE_DIR STREQUAL "")
    CPMAddPackage(NAME PROTOBUF GITHUB_REPOSITORY protocolbuffers/protobuf GIT_TAG v23.1 OPTIONS "protobuf_INSTALL ON" "protobuf_BUILD_TESTS OFF" "protobuf_BUILD_PROTOC_BINARIES ON" "BUILD_SHARED_LIBS OFF")   
endif()

message(STATUS "asio source path: " ${ASIO_SOURCE_DIR})
message(STATUS "protobuf source path: " ${PROTOBUF_SOURCE_DIR})

add_library(asyncmsg INTERFACE)
target_include_directories(asyncmsg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/asyncmsg INTERFACE ${ASIO_SOURCE_DIR}/asio/include)             
target_link_libraries(asyncmsg INTERFACE protobuf::libprotobuf)   
