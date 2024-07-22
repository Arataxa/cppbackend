cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

# Подключение Conan и настройка
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Добавляем основной исполняемый файл
add_executable(urldecode
    src/main.cpp
    src/urldecode.h
    src/urldecode.cpp
)

# Добавляем тестовый исполняемый файл
add_executable(tests
    tests/tests.cpp
    src/urldecode.h
    src/urldecode.cpp
)

# Убедитесь, что библиотека Boost найдена через Conan
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(tests PRIVATE Boost::unit_test_framework)
else()
    message(FATAL_ERROR "Boost not found!")
endif()