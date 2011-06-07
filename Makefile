
all: test

CXXFLAGS=-I/usr/include/libdrm -std=c++0x -O0 -g3
CXX=g++-4.5

test: main.o radeon.o r800_state.o cs_image.o radeon_cs.o
	g++-4.5 -o test  main.o r800_state.o radeon.o cs_image.o radeon_cs.o -ldrm_radeon -ldrm

