cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wmissing-prototypes -Werror")

add_library(tinyrl tinyrl.c vt100.c history.c)

add_executable(example example.c)
target_link_libraries(example tinyrl)
