cmake_minimum_required(VERSION 3.14)
project(syno_com C)

find_package(OpenSSL)

set(CMAKE_C_STANDARD 11)

add_library(syno_com SHARED syno_lib.c syno_lib.h)

target_link_libraries(syno_com curl)