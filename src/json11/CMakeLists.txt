cmake_minimum_required(VERSION 3.6)

# Find out if we're part of a bigger project
set(SUBPROJECT OFF)
if (DEFINED PROJECT_NAME)
    set(SUBPROJECT ON)
endif()

project(json11 VERSION 0)

if (NOT SUBPROJECT)
    enable_testing()
endif()

if (UNIX)
add_definitions(
    -std=c++11
    -fno-rtti
    -fno-exceptions
    -Wall
    -Wextra
    -Werror)
endif()

set(json11_SRCS json11.cpp)

add_library(json11 STATIC ${json11_SRCS})

if (NOT SUBPROJECT)
    add_test(json11_test json11_test)

    add_executable(json11_test ${json11_SRCS} test.cpp)
endif()

