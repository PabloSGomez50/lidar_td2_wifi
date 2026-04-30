#include "VL53L1X_api.h"
#include "Freertos/FreeRTOS.h"
#include "Freertos/task.h"
#include "esp_log.h"

#define MEASUREMENT_BUDGET_MS       50
#define INTER_MEASUREMENT_PERIOD_MS 55
#define VL53L1X_ADDRESS	 			0x52
 /* 1=short, 2=long */

typedef enum {
    short_distance = 1,
    long_distance = 2,
} laser_dist_mode;

typedef struct {
    uint8_t ready;
    uint16_t distance;
    uint8_t range;
    uint16_t signal_rate;
    uint16_t ambient_light;
    uint16_t spad_num;

} laser_data_t;


void init_vl53l1x(uint16_t dev, uint16_t modo);
laser_data_t get_data_laser(uint16_t dev);
// uint8_t send_laser_uart(laser_data_t data, USART_Type *port, uint16_t mot_angle);