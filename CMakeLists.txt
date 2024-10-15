cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключение Boost (все компоненты)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIR})

set(SOURCE
    "src/database.cpp"
    "src/executor_database.cpp"
    "src/session.cpp"
    "src/listener.cpp"
)

# Файлы исходного кода
add_executable(main ${SOURCE} main.cpp)
# Дополнительные опции компиляции
target_link_libraries(main ${Boost_LIBRARIES} ${PostgreSQL_LIBRARIES})