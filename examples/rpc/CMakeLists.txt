set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(rpc_server ${CMAKE_CURRENT_SOURCE_DIR}/rpc_server.cpp add.pb.cc add.pb.h)
add_executable(rpc_client ${CMAKE_CURRENT_SOURCE_DIR}/rpc_client.cpp add.pb.cc add.pb.h)

target_link_libraries(rpc_server PRIVATE asyncmsg)
target_link_libraries(rpc_client PRIVATE asyncmsg)

target_include_directories(rpc_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../asyncmsg)
target_include_directories(rpc_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../asyncmsg)

set_target_properties(rpc_server PROPERTIES FOLDER "examples")
set_target_properties(rpc_client PROPERTIES FOLDER "examples")
