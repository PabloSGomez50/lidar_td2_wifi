#include "as5600.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// void init_as5600_dir(uint8_t dir_pin) {
//     gpio_init(dir_pin);
//     gpio_set_dir(dir_pin, GPIO_OUT);
//     gpio_put(dir_pin, 0);
// }
static i2c_master_dev_handle_t s_dev_handle = NULL;

void as5600_set_i2c_device(i2c_master_dev_handle_t handle)
{
    s_dev_handle = handle;
}

as5600_status_t get_as5600_status() {
    as5600_status_t status = {0};
    uint8_t buf;
    
    // i2c_write_blocking(i2c, AS5600_ADDRESS, (uint8_t[]){AS5600_STATUS_REG}, 1, true);
    esp_err_t err = i2c_master_transmit(
        s_dev_handle,
        (uint8_t[]){AS5600_STATUS_REG},
        1,
        pdMS_TO_TICKS(1000)
    );
    if (err != ESP_OK) {
        return status;
    }
    // i2c_read_blocking(i2c, AS5600_ADDRESS, &buf, 1, false);
    err = i2c_master_receive(
        s_dev_handle,
        &buf,
        1,
        pdMS_TO_TICKS(1000)
    );
    if (err != ESP_OK) {
        return status;
    }
    buf = buf >> 3;
    status.mh  = buf & 0b001;
    status.ml = (buf & 0b010) >> 1;
    status.md = (buf & 0b100) >> 2;
    status.valid = (status.md & !status.ml & !status.mh) ? 1 : 0;

    return status;
}

uint16_t get_as5600_angle() {
    uint8_t buffer[2];
    // i2c_write_blocking(i2c, AS5600_ADDRESS, (uint8_t[]){AS5600_ANGLE_REG_HIGH}, 1, true);
    esp_err_t err = i2c_master_transmit(
        s_dev_handle,
        (uint8_t[]){AS5600_ANGLE_REG_HIGH},
        1,
        pdMS_TO_TICKS(1000)
    );
    if (err != ESP_OK) {
        return 0;
    }
    // i2c_read_blocking(i2c, AS5600_ADDRESS, buffer, 2, false);
    err = i2c_master_receive(
        s_dev_handle,
        buffer,
        2,
        pdMS_TO_TICKS(1000)
    );
    if (err != ESP_OK) {
        return 0;
    }
    uint16_t angle = ((buffer[0] & 0x0F) << 8 | buffer[1]);
    return angle;
}
    
uint8_t get_as5600_agc() {
    uint8_t agc;
    // i2c_write_blocking(i2c, AS5600_ADDRESS, (uint8_t[]){AS5600_AGC_REG}, 1, true);
    esp_err_t err = i2c_master_transmit(
        s_dev_handle,
        (uint8_t[]){AS5600_AGC_REG},
        1,
        pdMS_TO_TICKS(1000)
    );
    if (err != ESP_OK) {
        return 0;
    }
    // i2c_read_blocking(i2c, AS5600_ADDRESS, &agc, 1, false);
    err = i2c_master_receive(
        s_dev_handle,
        &agc,
        1,
        pdMS_TO_TICKS(1000)
    );
    if (err != ESP_OK) {
        return 0;
    }
    return agc;
}

int8_t process_as5600_angle(uint16_t angle, uint16_t ref_angle) {
    int16_t diff = (int16_t)angle - (int16_t)ref_angle;
    // Handle wrap-around (0-4095)
    if (diff > 2048) diff -= 4096;
    if (diff < -2048) diff += 4096;
    // Scale to -127 to 127
    int16_t out_angle = (diff * 127) / 2048;
    if (out_angle > 127) out_angle = 127;
    if (out_angle < -127) out_angle = -127;
    return (int8_t) out_angle;
}