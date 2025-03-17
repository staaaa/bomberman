cmake_minimum_required(VERSION 3.10)

project(bomberman)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

add_executable(server
    backend/server.c
    backend/state_updater.c
)

add_executable(client
    client/client.c
    client/client_reciver.c
    client/client_sender.c
)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE ${CURSES_LIBRARIES})

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "Building server and client executables")
message(STATUS "Server executable will be in: ${CMAKE_BINARY_DIR}")
message(STATUS "Client executable will be in: ${CMAKE_BINARY_DIR}")