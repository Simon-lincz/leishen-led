CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?=

BIN_DIR := build
COMMON := src/ec.c src/led_protocol.c

.PHONY: all clean install

all: $(BIN_DIR)/leishen_led $(BIN_DIR)/leishen-ledd

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/leishen_led: src/leishen_led.c $(COMMON) src/ec.h src/led_protocol.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/leishen_led.c $(COMMON) $(LDFLAGS)

$(BIN_DIR)/leishen-ledd: src/leishen-ledd.c $(COMMON) src/ec.h src/led_protocol.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/leishen-ledd.c $(COMMON) $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)
