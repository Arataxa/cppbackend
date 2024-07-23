cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(collision_detection_lib STATIC
    src/collision_detector.h
    src/collision_detector.cpp
)

target_link_libraries(collision_detection_lib PUBLIC CONAN_PKG::boost Threads::Threads)

include(FetchContent)

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.9
)

FetchContent_MakeAvailable(catch2)

add_executable(collision_detection_tests
    tests/collision-detector-tests.cpp
)

target_link_libraries(collision_detection_tests PRIVATE collision_detection_lib Catch2::Catch2 ${CONAN_LIBS})