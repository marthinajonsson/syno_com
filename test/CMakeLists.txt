# Find cmocka
#include_directories(${CMAKE_SOURCE_DIR}/../test/cmocka/include)
find_library(CMOCKA_LIBRARY cmocka HINTS ../../test/cmocka/build/src)

if(cmocka_FOUND)
    include_directories(${cmocka_INCLUDE_DIRS})
endif()

add_test(test_app_valgrind valgrind
      --error-exitcode=1 --read-var-info=yes
      --leak-check=full --show-leak-kinds=all
      ./test_app)

set(Sources
        ${CMAKE_CURRENT_LIST_DIR}/test_main.c)

add_executable(test_app ${Sources})

set_target_properties(test_app PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(test_app syno_com)
target_link_libraries(test_app cmocka)