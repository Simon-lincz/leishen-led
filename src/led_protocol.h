#ifndef LEISHEN_LED_PROTOCOL_H
#define LEISHEN_LED_PROTOCOL_H

#include <stdint.h>

#define LED_MODE       0x95
#define LED_BRIGHTNESS 0x98
#define LED_R          0x9A
#define LED_G          0x9B
#define LED_B          0x9C
#define LED_TIME_H     0x9D
#define LED_TIME_L     0x9E

struct led_state {
    uint8_t mode;
    uint8_t brightness;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint16_t time;
};

int led_parse_mode(const char *text, uint8_t *value);
const char *led_mode_label(uint8_t value);
int led_validate_state(const struct led_state *state);
int led_read_state(struct led_state *state);
int led_apply_state(const struct led_state *state);

#endif
