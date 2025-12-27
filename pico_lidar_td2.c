#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "as5600.h"
#include "vl53l1x.h"

#include "pico/cyw43_arch.h"

#define I2C_PORT i2c0
#define I2C_SDA 16
#define I2C_SCL 17

#define PWM_GPIO 0
#define PWM_WRAP 4096
#define PWM_MIN 2400
#define PWM_FREQ_KHZ 8.0f

#define GPIO_LED CYW43_WL_GPIO_LED_PIN

void init_pwm(uint gpio, float frequency_khz, uint16_t wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint32_t freq_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    pwm_set_clkdiv(slice_num, (float) freq_khz /  (frequency_khz * (float) wrap));
    pwm_set_wrap(slice_num, wrap);
    pwm_set_gpio_level(PWM_GPIO, 0);
    pwm_set_enabled(slice_num, true);
}

int main()
{
    stdio_init_all();
    uint16_t distance;
    VL53L1X_ERROR vl53l1x_status;
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    vl53l1x_status = VL53L1X_SensorInit(VL53L1X_ADDRESS);
    if (vl53l1x_status != VL53L1_ERROR_NONE) {
        printf("VL53L1X Sensor Init failed with error: %d\n", vl53l1x_status);
        cyw43_arch_gpio_put(GPIO_LED, 1);
        while (1) {
            printf("Retrying VL53L1X Sensor Init...\n");
            sleep_ms(100);
        }
    } else {
        for (int i = 0; i < 3; i++) {
            cyw43_arch_gpio_put(GPIO_LED, 1);
            sleep_ms(100);
            cyw43_arch_gpio_put(GPIO_LED, 0);
            sleep_ms(100);
        }
        printf("VL53L1X Sensor Init successful\n");
    }
    vl53l1x_status = VL53L1X_StartRanging(VL53L1X_ADDRESS);
    if (vl53l1x_status != VL53L1_ERROR_NONE) {
        printf("VL53L1X Sensor Start failed with error: %d\n", vl53l1x_status);
        cyw43_arch_gpio_put(GPIO_LED, 1);
        while (1) {
            printf("Retrying VL53L1X Sensor Start...\n");
            sleep_ms(100);
        }
    } else {
        for (int i = 0; i < 3; i++) {
            cyw43_arch_gpio_put(GPIO_LED, 1);
            sleep_ms(100);
            cyw43_arch_gpio_put(GPIO_LED, 0);
            sleep_ms(100);
        }
        printf("VL53L1X Sensor Start successful\n");
    }
    VL53L1X_SetDistanceMode(VL53L1X_ADDRESS, short_distance); // Long mode

    init_pwm(PWM_GPIO, PWM_FREQ_KHZ, PWM_WRAP);


    while (true) {

        as5600_status_t status = get_as5600_status(I2C_PORT);
        printf("AS5600 Status - MH: %d, ML: %d, MD: %d, Valid: %d\n", status.mh, status.ml, status.md, status.valid);
        uint16_t angle = get_as5600_angle(I2C_PORT);

        printf("AS5600 Angle: %d\n", angle);
        vl53l1x_status = VL53L1X_GetDistance(VL53L1X_ADDRESS, &distance);
        if (vl53l1x_status != VL53L1_ERROR_NONE) {
            printf("VL53L1X Get Distance failed with error: %d\n", vl53l1x_status);
            distance = 0;
        }
        printf("VL53L1X Distance: %d\n", distance);
        // cyw43_arch_gpio_put(GPIO_LED, 1);
        // pwm_set_gpio_level(PWM_GPIO, PWM_MIN + 256);
        // sleep_ms(500);
        pwm_set_gpio_level(PWM_GPIO, 0);
        // cyw43_arch_gpio_put(GPIO_LED, 0);
        sleep_ms(500);
    }
}
