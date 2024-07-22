cmake_minimum_required(VERSION 3.11)

project(urlencode CXX)
set(CMAKE_CXX_STANDARD 20)

# Подключение Conan и настройка
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Добавляем основной исполняемый файл
add_executable(urlencode
    src/main.cpp
    src/urlencode.h
    src/urlencode.cpp
)

# Добавляем тестовый исполняемый файл
add_executable(tests
    tests/tests.cpp
    src/urlencode.h
    src/urlencode.cpp
)

# Убедитесь, что библиотека Google Test найдена через Conan
find_package(GTest REQUIRED)

if(GTest_FOUND)
    include_directories(${GTest_INCLUDE_DIRS})
    target_link_libraries(tests PRIVATE GTest::GTest GTest::Main)
else()
    message(FATAL_ERROR "Google Test not found!")
endif()

# Добавляем команду для запуска тестов
enable_testing()
add_test(NAME tests COMMAND tests)
