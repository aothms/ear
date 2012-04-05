cmake_minimum_required (VERSION 2.6)
project (EAR)

find_package(Boost COMPONENTS thread REQUIRED)

IF(NOT CMAKE_BUILD_TYPE)
 	SET(CMAKE_BUILD_TYPE "Release")
	MESSAGE("Defaulting to release build")
ELSE(NOT CMAKE_BUILD_TYPE)
	ADD_DEFINITIONS("-g")
ENDIF(NOT CMAKE_BUILD_TYPE)

FILE(GLOB ear_sources "../src/*.cpp")

SET(libs "../lib")
SET(lib_sources "${libs}/wave/WaveFile.cpp" "${libs}/equalizer/Equalizer.cpp")

INCLUDE_DIRECTORIES("${libs}/gmtl-0.6.1" "${libs}/wave" "${libs}/equalizer")

ADD_EXECUTABLE(EAR ${ear_sources} ${lib_sources})
TARGET_LINK_LIBRARIES (EAR ${Boost_THREAD_LIBRARY})

INSTALL(TARGETS EAR DESTINATION bin)
