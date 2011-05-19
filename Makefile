
all: prb

CXXFLAGS=-I/usr/include/libdrm -std=c++0x -O0 -g3
CXX=g++-4.6

prb: main.o radeon.o r800_state.o cs_image.o
	g++-4.6 -o prb  main.o r800_state.o radeon.o cs_image.o -ldrm_radeon -ldrm

