cmake_minimum_required(VERSION 3.14)
project(syno_com C)

set(CMAKE_C_STANDARD 11)

add_library(syno_com SHARED library.c library.h)