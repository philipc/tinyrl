cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wmissing-prototypes -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

add_library(tinyrl tinyrl.c history.c complete.c)

add_executable(example example.c)
target_link_libraries(example tinyrl)
