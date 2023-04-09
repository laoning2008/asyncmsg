add_executable(server ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp)
add_executable(client ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)

target_link_libraries(server PRIVATE asyncmsg)
target_link_libraries(client PRIVATE asyncmsg)