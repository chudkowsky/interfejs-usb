cmake_minimum_required(VERSION 3.23)
project(interfejs_USB)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(interfejs_USB main.cpp)
target_link_libraries(interfejs_USB setupapi)