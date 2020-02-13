cmake_minimum_required(VERSION 3.2)
project(robot)

find_package(Threads REQUIRED)

set(SOURCES 
        src/main.c)

include_directories(
	include
)

add_executable(robot ${SOURCES})
target_link_libraries(robot Threads::Threads)
