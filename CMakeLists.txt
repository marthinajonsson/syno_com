cmake_minimum_required(VERSION 3.14)
project(syno_com C)

find_package(OpenSSL)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wpedantic") # -Werror

include_directories(src)
add_subdirectory(test)

set(src ${PROJECT_SOURCE_DIR}/src/syno_lib.c
        ${PROJECT_SOURCE_DIR}/src/syno_lib.h)

add_library(syno_com SHARED ${src})
enable_testing()

set_target_properties(syno_com PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(syno_com curl)