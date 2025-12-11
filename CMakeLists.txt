cmake_minimum_required(VERSION 3.21)

project(jxr2mp4 C)

set(JXRLIB_FORCE_STATIC ON)
add_subdirectory(jxrlib EXCLUDE_FROM_ALL)
add_library(decode OBJECT decode.c)
if(NOT MSVC)
    target_compile_definitions(decode PRIVATE __ANSI__)
endif()
target_compile_options(decode PRIVATE -Wall -Wextra -Werror)
target_include_directories(decode PRIVATE
    ${jxrlib_SOURCE_DIR}/common/include
    ${jxrlib_SOURCE_DIR}/image/sys
    ${jxrlib_SOURCE_DIR}/jxrgluelib
)
target_link_libraries(decode INTERFACE jxrglue)

add_executable(main main.c)
target_compile_options(main PRIVATE -Wall -Wextra -Werror)
target_link_libraries(main PRIVATE decode)
