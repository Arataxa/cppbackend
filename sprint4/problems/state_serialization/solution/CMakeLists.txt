cmake_minimum_required(VERSION 3.9)
project(GameServer CXX)
set(CMAKE_CXX_STANDARD 20)

# обратите внимание на аргумент TARGETS у команды conan_basic_setup
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS log log_setup system filesystem thread regex chrono atomic)

add_subdirectory(src/GameModelLib)

add_executable(game_server 
	src/main.cpp
	src/serialization/json_loader.h
	src/serialization/json_loader.cpp
	src/serialization/json_serialization.h
	src/serialization/json_serialization.cpp
	src/networking/http_server.h
	src/networking/http_server.cpp
	src/handlers/front_controller.h
	src/handlers/api_request_handler.h
	src/handlers/static_request_handler.h
	src/handlers/static_request_handler.cpp	
	src/logging/logger.h
	src/utility/ticker.h
	src/utility/loot_type_info.h
	src/utility/loot_type_info.cpp
	src/application/application.h
	src/application/application.cpp
	src/application/serialization.h
	src/application/serialization.cpp
)

target_include_directories(game_server PRIVATE
    src/serialization
    src/networking
    src/handlers
    src/logging
    src/utility
    src/application
)

# используем "импортированную" цель CONAN_PKG::boost
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE GameModel CONAN_PKG::boost Threads::Threads)