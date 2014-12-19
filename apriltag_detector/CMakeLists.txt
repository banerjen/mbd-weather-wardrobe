project(mbd_apriltags_c)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

include_directories(apriltags apriltags/common)
link_directories(apriltags apriltags/common)

add_library(${PROJECT_NAME} STATIC apriltag_detector.cpp)
target_link_libraries(${PROJECT_NAME} libapriltag.a)

set(CMAKE_C_FLAGS "-std=c99")

find_package(jpeg)

include_directories(${jpeg_INCLUDE_DIRS})

add_executable(capture capture.c)
target_link_libraries(capture jpeg)
