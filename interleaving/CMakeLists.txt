cmake_minimum_required(VERSION 3.14)

project(interleaved
    VERSION 1.0.0
    DESCRIPTION "Testing interleaved stores"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG v2.4.1
)

FetchContent_Declare(
  eztimer
  GIT_REPOSITORY https://github.com/tatami-inc/eztimer
  GIT_TAG master
)

FetchContent_MakeAvailable(cli11)
FetchContent_MakeAvailable(eztimer)

add_executable(interleaved test.cpp)
target_link_libraries(interleaved tatami::eztimer CLI11::CLI11)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(x64)|(AMD64)")
    target_compile_definitions(interleaved PRIVATE CACHE_LINE_SIZE=64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)")
    target_compile_definitions(interleaved PRIVATE CACHE_LINE_SIZE=128)
endif()
