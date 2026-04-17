cmake_minimum_required(VERSION 3.28.3)
project(parallel_computing)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenCV REQUIRED)
find_package(RtAudio REQUIRED)

add_subdirectory(lab1)
add_subdirectory(lab2)
add_subdirectory(lab3)
add_subdirectory(lab5)