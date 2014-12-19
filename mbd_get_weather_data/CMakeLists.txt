project(mbd_get_weather_data)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

#find_package(Boost REQUIRED)

#if(Boost_FOUND)
#	add_definitions(-DBoost_LIBRARIES_PRESENT)
#	string(ASCII 27 Esc)
#	set(BoldGreen "${Esc}[1;32m")
#	set(ColourReset "${Esc}[m")
#	message("${BoldGreen}-- Compiling ${PROJECT_NAME} with Boost support.${ColourReset}")
#endif()

add_executable(${PROJECT_NAME} ${SRC_LIST})

#if(Boost_FOUND)
#	target_link_libraries(${PROJECT_NAME} boost_thread-mt curl)
#else()
	target_link_libraries(${PROJECT_NAME} curl)
	#endif()
