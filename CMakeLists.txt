cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(asyncmsg CXX)

option(ASYNC_MSG_ENABLE_LOGGING "enable log" ON)
set(ASIO_SOURCE_DIR "" CACHE STRING "asio source path")

if(ASIO_SOURCE_DIR STREQUAL "")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)
    CPMAddPackage(NAME ASIO GITHUB_REPOSITORY chriskohlhoff/asio GIT_TAG asio-1-28-2)
endif()

message(STATUS "asio source path: " ${ASIO_SOURCE_DIR})

add_library(asyncmsg INTERFACE)
target_include_directories(asyncmsg 
                            INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/asyncmsg
                            INTERFACE ${ASIO_SOURCE_DIR}/asio/include
                            )                        
