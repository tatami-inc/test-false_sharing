cmake_minimum_required(VERSION 3.14)

project(summation
    VERSION 1.0.0
    DESCRIPTION "Performance tests for summation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG v2.4.1
)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench
    GIT_TAG v4.3.11
)

FetchContent_MakeAvailable(cli11)
FetchContent_MakeAvailable(nanobench)

add_executable(sharing sharing.cpp)
target_link_libraries(sharing nanobench::nanobench CLI11::CLI11)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(x64)|(AMD64)")
    target_compile_definitions(sharing PRIVATE CACHE_LINE_SIZE=64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)")
    target_compile_definitions(sharing PRIVATE CACHE_LINE_SIZE=128)
endif()
