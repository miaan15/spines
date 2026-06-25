CC = gcc
SRC = spines.c main.c
TARGET = a

BUILD ?= debug

ifeq ($(BUILD),release)
    CFLAGS = -Wall -Wextra -O3
else
    CFLAGS = -Wall -Wextra -g -O0
endif

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)