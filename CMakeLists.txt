cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asyncmsg CXX)

add_library(asyncmsg INTERFACE)
target_include_directories(asyncmsg INTERFACE 
                            ${asio_SOURCE_DIR}/asio/include
                            ${protobuf_SOURCE_DIR}/src
                            ${CMAKE_CURRENT_SOURCE_DIR}/asyncmsg)

target_link_libraries(asyncmsg INTERFACE protobuf::libprotobuf)                            
