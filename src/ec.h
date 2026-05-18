#ifndef LEISHEN_EC_H
#define LEISHEN_EC_H

#include <stdint.h>

#define EC_DATA_PORT 0x62
#define EC_CMD_PORT  0x66

int ec_init(void);
int ec_read(uint8_t offset, uint8_t *value);
int ec_write(uint8_t offset, uint8_t value);

#endif
