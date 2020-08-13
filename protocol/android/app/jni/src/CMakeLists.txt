cmake_minimum_required(VERSION 3.6)

project(MY_APP)

find_library(include/SDL2 include/SDL2)

add_library(main SHARED)

target_sources(main PRIVATE test.c)

target_link_libraries(main include/SDL2)
