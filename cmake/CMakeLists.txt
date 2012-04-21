cmake_minimum_required (VERSION 2.6)
project (EAR)
 
# find_package(Threads)
# set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS thread REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
 	set(CMAKE_BUILD_TYPE "Release")
	message(STATUS "Defaulting to release build")
endif(NOT CMAKE_BUILD_TYPE)

file(GLOB ear_sources "../src/*.cpp")

set(libs "../lib")
set(lib_sources "${libs}/wave/WaveFile.cpp" "${libs}/equalizer/Equalizer.cpp")

include_directories("${libs}/wave" "${libs}/equalizer")

add_executable(EAR ${ear_sources} ${lib_sources})
target_link_libraries (EAR ${Boost_THREAD_LIBRARY})
#target_link_libraries (EAR ${Boost_THREAD_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS EAR DESTINATION bin)
