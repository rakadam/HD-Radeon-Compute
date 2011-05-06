
all: prb

CXXFLAGS=-I/usr/include/libdrm -std=c++0x

prb: main.o radeon.o r800_state.o
	g++ -o prb -ldrm_radeon -ldrm main.o r800_state.o radeon.o

