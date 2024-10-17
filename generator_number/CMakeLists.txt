cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключение Boost (все компоненты)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


set(SOURCE
    "src/session.cpp"
    "src/listener.cpp"
)

add_executable(main ${SOURCE} main.cpp)

target_link_libraries(main ${Boost_LIBRARIES})