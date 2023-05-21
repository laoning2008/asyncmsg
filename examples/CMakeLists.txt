cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(asyncmsg_examples CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)
CPMAddPackage(NAME asio GITHUB_REPOSITORY chriskohlhoff/asio GIT_TAG asio-1-28-0)
CPMAddPackage(NAME protobuf GITHUB_REPOSITORY protocolbuffers/protobuf GIT_TAG v23.1 
            OPTIONS "protobuf_INSTALL ON" "protobuf_BUILD_TESTS OFF"
            "protobuf_BUILD_PROTOC_BINARIES ON" "BUILD_SHARED_LIBS OFF")

add_definitions(-DASYNC_MSG_ENABLE_LOGGING)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/build/asyncmsg)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tcp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rpc)
