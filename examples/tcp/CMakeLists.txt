set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(tcp_server ${CMAKE_CURRENT_SOURCE_DIR}/tcp_server.cpp)
add_executable(tcp_client ${CMAKE_CURRENT_SOURCE_DIR}/tcp_client.cpp)

target_link_libraries(tcp_server PRIVATE asyncmsg)
target_link_libraries(tcp_client PRIVATE asyncmsg)

target_include_directories(tcp_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../asyncmsg)
target_include_directories(tcp_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../asyncmsg)


set_target_properties(tcp_server PROPERTIES FOLDER "examples")
set_target_properties(tcp_client PROPERTIES FOLDER "examples")
