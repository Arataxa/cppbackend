cmake_minimum_required(VERSION 3.9)

project(book_manager CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(book_manager main.cpp)
target_link_libraries(book_manager CONAN_PKG::boost CONAN_PKG::libpqxx)