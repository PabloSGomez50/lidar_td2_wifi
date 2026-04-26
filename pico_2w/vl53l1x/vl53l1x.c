#include "vl53l1x.h"


void init_vl53l1x(uint16_t dev) {
    VL53L1X_ERROR status = VL53L1X_SensorInit(dev);
    if (status != VL53L1_ERROR_NONE) {
        printf("VL53L1X Sensor Init failed with error: %d\n", status);
    } else {
        printf("VL53L1X Sensor Init successful\n");
    }
}
