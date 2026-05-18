#define _DEFAULT_SOURCE

#include "ec.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>

#define EC_OBF   0x01
#define EC_IBF   0x02
#define EC_READ  0x80
#define EC_WRITE 0x81

static int wait_ibf_clear(void) {
    for (int i = 0; i < 0x20000; i++) {
        if ((inb(EC_CMD_PORT) & EC_IBF) == 0) {
            return 0;
        }
        usleep(10);
    }

    return -1;
}

static int wait_obf_set(void) {
    for (int i = 0; i < 0x20000; i++) {
        if (inb(EC_CMD_PORT) & EC_OBF) {
            return 0;
        }
        usleep(15);
    }

    return -1;
}

int ec_init(void) {
    if (ioperm(EC_DATA_PORT, 1, 1) || ioperm(EC_CMD_PORT, 1, 1)) {
        perror("ioperm");
        return -1;
    }

    return 0;
}

int ec_read(uint8_t offset, uint8_t *value) {
    if (wait_ibf_clear()) {
        return -1;
    }
    outb(EC_READ, EC_CMD_PORT);

    if (wait_ibf_clear()) {
        return -1;
    }
    outb(offset, EC_DATA_PORT);

    if (wait_obf_set()) {
        return -1;
    }
    *value = inb(EC_DATA_PORT);
    return 0;
}

int ec_write(uint8_t offset, uint8_t value) {
    if (wait_ibf_clear()) {
        return -1;
    }
    outb(EC_WRITE, EC_CMD_PORT);

    if (wait_ibf_clear()) {
        return -1;
    }
    outb(offset, EC_DATA_PORT);

    if (wait_ibf_clear()) {
        return -1;
    }
    outb(value, EC_DATA_PORT);
    return 0;
}
