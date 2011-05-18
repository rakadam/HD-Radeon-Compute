
all: prb

CXXFLAGS=-I/usr/include/libdrm -std=c++0x
CXX=g++-4.6

prb: main.o radeon.o r800_state.o
	g++-4.6 -o prb -ldrm_radeon -ldrm main.o r800_state.o radeon.o

