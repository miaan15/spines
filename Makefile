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

a: spines.o main.o
	$(CXX) -o $@ $^

b: spines_bm.o main_bm.o
	$(CXX) -o $@ $^

spines.o: spines.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

spines_bm.o: spines_bm.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

main_bm.o: main_bm.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f a b spines.o spines_bm.o main.o main_bm.o
