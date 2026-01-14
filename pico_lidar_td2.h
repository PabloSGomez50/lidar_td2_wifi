#ifndef __PICO_LIDAR_TD2_H__
#define __PICO_LIDAR_TD2_H__
// 1. Headers del sistema
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "mongoose.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


#define DEBUG_printf printf
#define I2C_PORT i2c0
#define I2C_SDA 16
#define I2C_SCL 17

#define GPIO_MOT 0
#define PWM_WRAP 4096
#define PWM_MIN 2400
#define PWM_FREQ_KHZ 8.0f

#define GPIO_LED CYW43_WL_GPIO_LED_PIN


void task_as5600(void * pvParameters);
void task_laser(void * pvParameters);

void init_pwm(uint gpio, float frequency_khz, uint16_t wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint32_t freq_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    pwm_set_clkdiv(slice_num, (float) freq_khz /  (frequency_khz * (float) wrap));
    pwm_set_wrap(slice_num, wrap);
    pwm_set_gpio_level(gpio, 0);
    pwm_set_enabled(slice_num, true);
}

#endif // __PICO_LIDAR_TD2_H__