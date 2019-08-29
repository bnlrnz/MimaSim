CC = gcc
CFLAGS = -c -Wall -O3 -Iinclude
LD = $(CC)

UNAME_S := $(shell uname -s)

TARGET = MimaSim
OBJECTS = $(patsubst %.c, %.o, $(wildcard src/*.c))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(TARGET) $(OBJECTS)

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

	