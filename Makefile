CC  = gcc
CXX = g++

TARGET = a

BUILD ?= debug

ifeq ($(BUILD),release)
    CFLAGS   = -Wall -Wextra -O3
    CXXFLAGS = -Wall -Wextra -O3
else
    CFLAGS   = -Wall -Wextra -g -O0
    CXXFLAGS = -Wall -Wextra -g -O0
endif

OBJS = spines.o main.o

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

spines.o: spines.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
