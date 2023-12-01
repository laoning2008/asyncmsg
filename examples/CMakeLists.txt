cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(asyncmsg_examples CXX)
     
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/build/asyncmsg)

add_executable(tcp_client ${CMAKE_CURRENT_SOURCE_DIR}/tcp_client.cpp)
target_link_libraries(tcp_client PRIVATE asyncmsg)
set_target_properties(tcp_client PROPERTIES FOLDER "examples")

add_executable(tcp_server ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server.cpp)
target_link_libraries(tcp_server PRIVATE asyncmsg)
set_target_properties(tcp_server PROPERTIES FOLDER "examples")

add_executable(rpc_client ${CMAKE_CURRENT_SOURCE_DIR}/rpc_client.cpp example.pb.cc example.pb.h)
target_link_libraries(rpc_client PRIVATE asyncmsg)
set_target_properties(rpc_client PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(rpc_client PROPERTIES FOLDER "examples")

add_executable(rpc_server ${CMAKE_CURRENT_SOURCE_DIR}/rpc_server.cpp example.pb.cc example.pb.h)
target_link_libraries(rpc_server PRIVATE asyncmsg)
set_target_properties(rpc_server PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(rpc_server PROPERTIES FOLDER "examples")
