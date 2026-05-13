#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "vl53l1x.h"

#define LIDAR_SAMPLE_BUFFER_SIZE 32

typedef struct {
    laser_data_t laser_data;
    bool as5600_valid;
    uint16_t angle;
    uint8_t agc;
    uint32_t timestamp_ms;
} lidar_sample_t;

void lidar_data_init(void);
esp_err_t lidar_data_start(void);
esp_err_t lidar_data_stop(void);
bool lidar_data_is_running(void);
size_t lidar_data_get_count(void);
bool lidar_data_get_latest(lidar_sample_t *out_sample);
size_t lidar_data_copy_samples(lidar_sample_t *out_samples, size_t max_samples, size_t offset);
