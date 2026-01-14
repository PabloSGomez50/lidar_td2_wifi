#include "pico_lidar_td2.h"
#include "as5600.h"
#include "vl53l1x.h"


#define WIFI_SSID "Telecentro-996b"
#define WIFI_PASSWORD "ZNYUW3MDZDTM"

// void connect_to_wifi(char * wifi_ssid, char * wifi_password) {
//     DEBUG_printf("Connecting to Wi-Fi...\n");
//     cyw43_arch_enable_sta_mode();
//     uint8_t attempt = 0;
//     int error_code; 
//     while ((error_code = cyw43_arch_wifi_connect_timeout_ms(wifi_ssid, wifi_password, CYW43_AUTH_WPA2_AES_PSK, 3000)) != 0) {
//         attempt++;
//         DEBUG_printf("Attempt %d: Failed. Error code: %d\n", attempt, error_code);
//         if (attempt >= 5) {
//             DEBUG_printf("giving up.\n");
//             return;
//         }
//         sleep_ms(500);
//     }
//     DEBUG_printf("Connected to %s.\n", wifi_ssid);
// }

int main()
{
    stdio_init_all();
    
    if (cyw43_arch_init()) {
        DEBUG_printf("Wi-Fi init failed");
        return -1;
    }
    DEBUG_printf("Loading...");
    cyw43_arch_gpio_put(GPIO_LED, 1);
    sleep_ms(1500);

    // connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    DEBUG_printf("Pico LIDAR TD2 Starting\n");
    cyw43_arch_gpio_put(GPIO_LED, 0);
    
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    init_pwm(GPIO_MOT, PWM_FREQ_KHZ, PWM_WRAP);

    xTaskCreate(task_as5600, "AS5600 Task", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
    xTaskCreate(task_laser, "VL53L1X Task", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
    vTaskStartScheduler();
    while (true) {
        tight_loop_contents();
    }
}

void task_as5600(void * pvParameters) {
    as5600_status_t status;
    uint16_t angle;
    while(1) {
        status = get_as5600_status(I2C_PORT);
        DEBUG_printf("AS5600 Status - MH: %d, ML: %d, MD: %d, Valid: %d\n", status.mh, status.ml, status.md, status.valid);
        angle = get_as5600_angle(I2C_PORT);
        DEBUG_printf("AS5600 Angle: %d\n", angle);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void task_laser(void * pvParameters) {
    uint16_t distance;
    VL53L1X_ERROR vl53l1x_status;

    vl53l1x_status = VL53L1X_SensorInit(VL53L1X_ADDRESS);
    if (vl53l1x_status != VL53L1_ERROR_NONE) {
        DEBUG_printf("VL53L1X Sensor Init failed with error: %d\n", vl53l1x_status);
        cyw43_arch_gpio_put(GPIO_LED, 1);
        vTaskDelete(NULL);
    }
    vl53l1x_status = VL53L1X_SetDistanceMode(VL53L1X_ADDRESS, short_distance); // Long mode
    if (vl53l1x_status != VL53L1_ERROR_NONE) {
        DEBUG_printf("VL53L1X Set Distance Mode failed with error: %d\n", vl53l1x_status);
        cyw43_arch_gpio_put(GPIO_LED, 1);
        vTaskDelete(NULL);
    }
    vl53l1x_status = VL53L1X_StartRanging(VL53L1X_ADDRESS);
    if (vl53l1x_status != VL53L1_ERROR_NONE) {
        DEBUG_printf("VL53L1X Sensor Start failed with error: %d\n", vl53l1x_status);
        cyw43_arch_gpio_put(GPIO_LED, 1);
        vTaskDelete(NULL);
    }

    for (int i = 0; i < 3; i++) {
        cyw43_arch_gpio_put(GPIO_LED, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        cyw43_arch_gpio_put(GPIO_LED, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while(1) {
        vl53l1x_status = VL53L1X_GetDistance(VL53L1X_ADDRESS, &distance);
        if (vl53l1x_status != VL53L1_ERROR_NONE) {
            DEBUG_printf("VL53L1X Get Distance failed with error: %d\n", vl53l1x_status);
            distance = 0;
        }
        DEBUG_printf("VL53L1X Distance: %d\n", distance);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}