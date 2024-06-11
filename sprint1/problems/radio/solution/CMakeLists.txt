cmake_minimum_required(VERSION 3.11)

project(Radio CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Ищем Boost версии 1.78
find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(MINIAUDIO_DIR ${CMAKE_SOURCE_DIR}/external/miniaudio)
include_directories(${MINIAUDIO_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Под Windows нужно определить макрос NOMINMAX для корректной работы при включении
# библиотеки minisound
if(WIN32)
  add_definitions(-DNOMINMAX)
endif()

add_executable(radio src/main.cpp src/audio.h src/Application.h)

if(UNIX)
  target_link_libraries(radio ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} dl)
else()
  target_link_libraries(radio ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()
