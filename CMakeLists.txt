cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

project(asyncmsg CXX)

CPMAddPackage(NAME asio GITHUB_REPOSITORY chriskohlhoff/asio GIT_TAG asio-1-28-0)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
find_package(Threads REQUIRED)
target_link_libraries(asio INTERFACE Threads::Threads)

add_library(asyncmsg INTERFACE)
target_link_libraries(asyncmsg INTERFACE asio)
target_include_directories(asyncmsg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(ASYNC_MSG_ENABLE_LOGGING "Build the exmples" ON)
if (${ASYNC_MSG_ENABLE_LOGGING})
    add_definitions(-DASYNC_MSG_ENABLE_LOGGING)
endif()

option(ASYNC_MSG_ENABLE_EXAMPLES "Build the exmples" ON)
if (${ASYNC_MSG_ENABLE_EXAMPLES})
    add_subdirectory(examples)
    set_target_properties(server PROPERTIES FOLDER "examples")
    set_target_properties(client PROPERTIES FOLDER "examples")
endif()
