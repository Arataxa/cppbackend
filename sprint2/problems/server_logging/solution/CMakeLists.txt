cmake_minimum_required(VERSION 3.9)
project(GameServer CXX)
set(CMAKE_CXX_STANDARD 20)

# обратите внимание на аргумент TARGETS у команды conan_basic_setup
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS log log_setup system filesystem thread regex chrono atomic)

add_executable(game_server 
	src/main.cpp
	src/logger.h
	src/model.h
	src/model.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/tagged.h
	src/front_controller.h
	src/api_request_handler.h
	src/static_request_handler.h
	src/static_request_handler.cpp
	src/serialization.h
	src/serialization.cpp
	src/http_server.h
	src/http_server.cpp
)

# используем "импортированную" цель CONAN_PKG::boost
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server CONAN_PKG::boost Threads::Threads)