CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?=

BIN_DIR := build
COMMON := src/ec.c src/led_protocol.c
API := src/api/api_router.c src/api/api_status.c src/api/api_presets.c src/api/api_apply.c src/api/api_effect.c src/api/api_modes.c
UTILS := src/logger.c src/safe_utils.c

.PHONY: all clean install

all: $(BIN_DIR)/leishen_led $(BIN_DIR)/leishen-ledd

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/leishen_led: src/leishen_led.c $(COMMON) src/ec.h src/led_protocol.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/leishen_led.c $(COMMON) $(LDFLAGS)

$(BIN_DIR)/leishen-ledd: src/leishen-ledd.c $(COMMON) $(API) $(UTILS) src/ec.h src/led_protocol.h src/api/api.h src/logger.h src/safe_utils.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ src/leishen-ledd.c $(COMMON) $(API) $(UTILS) $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)
