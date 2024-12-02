CXX=g++ 
INC=
LIBS=-lpim
CXXFLAGS += -std=c++11 -g -O3
# CXXFLAGS += -std=c++11 -g
TARGET = pmm
OBJS = pmm.o convert_numeric.o pim_mac_simul.o result_checker.o

OBJECTS = $(patsubst %.cpp,%.o, $(wildcard ${PWD}/*.cpp))

# Default target executed when no arguments are given to make.
.DEFAULT_GOAL=all

${TARGET}: ${OBJS}
	${CXX} ${CXXFLAGS} -o $@ $^ ${LIBS}
	
all: ${TARGET}
.PHONY: all

clean:
	rm -f *.o *.d ${TARGET}
