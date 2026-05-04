/* HTTP Restful API Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "Wireless/Wireless.h"
#include "vl53l1x.h"
#include "as5600.h"

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

static const char *TAG = "main_file";

i2c_master_bus_handle_t bus_handle;

static esp_err_t init_i2c_master(void)
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    return ESP_OK;
}


void get_sensor_data() {
    laser_data_t laser_data = get_data_laser(VL53L1X_ADDRESS);
    ESP_LOGI(TAG, "Distance: %u mm, Signal Rate: %u Mcps, Ambient Light: %u Mcps, SPADs: %u",
             laser_data.distance, laser_data.signal_rate, laser_data.ambient_light, laser_data.spad_num);
    as5600_status_t as5600_status = get_as5600_status();
    uint16_t angle = get_as5600_angle();
    uint8_t agc = get_as5600_agc();
    ESP_LOGI(TAG, "AS5600 - Angle: %u, AGC: %u, Status: [%c]",
             angle, agc, as5600_status.valid ? 'X' : '-');
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting up...");

    esp_err_t i2c_err = init_i2c_master();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(i2c_err));
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = (VL53L1X_ADDRESS >> 1),
        .scl_speed_hz = 100000,
    };
    
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    vl53l1x_set_i2c_device(dev_handle);
    
    init_vl53l1x(VL53L1X_ADDRESS, short_distance);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    i2c_device_config_t as5600_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AS5600_ADDRESS,
        .scl_speed_hz = 100000,
    };
    
    i2c_master_dev_handle_t as5600_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &as5600_cfg, &as5600_handle));
    as5600_set_i2c_device(as5600_handle);

    ESP_LOGW(TAG, "WiFi connection initiated, waiting for connection...");
    connect_to_wifi();
             
    while(1) {
        ESP_LOGI(TAG, "Dando vueltas en el loop...");

        get_sensor_data();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
