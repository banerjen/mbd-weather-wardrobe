project(mbd_fuzzy_prediction)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

cmake_policy(SET CMP0015 NEW)

include_directories(../apriltag_detector ../apriltag_detector/apriltags ../apriltag_detector/apriltags/common ../fuzzylite/fuzzylite)
link_directories(../apriltag_detector ../apriltag_detector/apriltags ../apriltag_detector/apriltags/common ../fuzzylite/fuzzylite/debug/bin)

add_executable(${PROJECT_NAME} main.cpp fuzzy_weather_prediction.cpp)
target_link_libraries(${PROJECT_NAME} libmbd_apriltags_c.a libfuzzylite-staticd.a libapriltag.a -lpthread)
