cmake_minimum_required(VERSION 2.6)

PROJECT(r800_compute)
SET(CMAKE_VERBOSE_MAKEFILE OFF)

ENABLE_TESTING()

include_directories(. /usr/include/libdrm)
SET(CMAKE_CXX_FLAGS "-std=c++0x -O3 -g3 -save-temps")

SET(r800_src
	cs_image.cpp
	r800_state.cpp
	radeon_cs.cpp
)

ADD_LIBRARY(r800_compute ${r800_src})

SET(exp_main
	main.cpp
	first_cs.bin
)

ADD_EXECUTABLE(expr ${exp_main})
TARGET_LINK_LIBRARIES(expr r800_compute drm_radeon drm)

ADD_CUSTOM_COMMAND(OUTPUT first_cs.bin 
	COMMAND ./as_r800 first_cs.asm
	DEPENDS first_cs.asm)

add_subdirectory(tests)