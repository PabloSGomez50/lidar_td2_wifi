#ifndef VL53L1X_H
#define VL53L1X_H

#include "VL53L1X_api.h"
#include "vl53l1_error_codes.h"

#define MEASUREMENT_BUDGET_MS       50
#define INTER_MEASUREMENT_PERIOD_MS 55
#define VL53L1X_ADDRESS	 			0x29

typedef enum {
    short_distance = 1,
    long_distance = 2,
} vl53l1x_dist_mode;


void init_vl53l1x(uint16_t dev);
#endif /* VL53L1X_H */