CC  = gcc
CXX = g++

BUILD ?= debug

ifeq ($(BUILD),release)
    CFLAGS   = -Wall -Wextra -O3
    CXXFLAGS = -Wall -Wextra -O3
else
    CFLAGS   = -Wall -Wextra -g -O0
    CXXFLAGS = -Wall -Wextra -g -O0
endif

a: spines.o main.o spines.c main.cpp
	$(CXX) -o $@ spines.o main.o

spines.o: spines.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f a spines.o main.o
