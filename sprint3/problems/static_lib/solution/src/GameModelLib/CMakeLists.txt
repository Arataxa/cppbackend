cmake_minimum_required(VERSION 3.9)

project(GameModelLib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(GameModel STATIC ${SOURCES})

target_include_directories(GameModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(GameModel PUBLIC CONAN_PKG::boost)