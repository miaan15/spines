CC = gcc
CFLAGS = -Wall -Wextra

DEBUG_FLAGS = -g -O1 -fno-omit-frame-pointer -fsanitize=address
RELEASE_FLAGS = -O3

SRC = spines.c main.c
TARGET = a

.PHONY: all debug release clean

all: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
