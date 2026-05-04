#ifndef AS5600_H_
#define AS5600_H_

#include <stdio.h>
#include "driver/i2c_master.h"

#define AS5600_ADDRESS 0x36

#define AS5600_ANGLE_REG_HIGH 0x0E
#define AS5600_ANGLE_REG_LOW  0x0F
#define AS5600_STATUS_REG 0x0B

#define AS5600_AGC_REG 0x1A
#define AS5600_MAG_REG_HIGH 0x1B

typedef struct {
    uint8_t mh;
    uint8_t ml;
    uint8_t md;
    uint8_t valid;
} as5600_status_t;


void as5600_set_i2c_device(i2c_master_dev_handle_t handle);
// void init_as5600_dir(uint8_t dir_pin);
as5600_status_t get_as5600_status();
uint16_t get_as5600_angle();
uint8_t get_as5600_agc();

int8_t process_as5600_angle(uint16_t angle, uint16_t ref_angle);

#endif