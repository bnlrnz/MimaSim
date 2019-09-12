CC = gcc
CFLAGS = -c -Wall -Wextra -O3 -Iinclude
LD = $(CC)

UNAME_S := $(shell uname -s)

TARGET = MimaSim
OBJECTS = $(patsubst %.c, %.o, $(wildcard src/*.c))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJECTS)
	rm -rf web/MimaSim.wasm web/MimaSim.js

debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

.PHONY: web
web: CC = emcc
web: CFLAGS = -O3 -DWEBASM -s WASM=1 -Iinclude -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='["_mima_init", "_mima_compile_s", "_mima_delete", "_mima_run_micro_instruction_step", "_mima_run_instruction_step"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap"]'
web: 
	$(CC) $(wildcard src/*.c) $(CFLAGS) -o web/MimaSim.js