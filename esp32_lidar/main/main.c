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
#include "Wireless/Wireless.h"
#include "lidar_data.h"
#include "rest_server.h"
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

static void probe_i2c_device(uint8_t address, const char *name)
{
    esp_err_t err = i2c_master_probe(bus_handle, address, pdMS_TO_TICKS(1000));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C device %s found at 0x%02X", name, address);
    } else {
        ESP_LOGW(TAG, "I2C device %s not found at 0x%02X (%s)", name, address, esp_err_to_name(err));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting up...");

    esp_err_t i2c_err = init_i2c_master();
    if (i2c_err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(i2c_err));
        return;
    }

    probe_i2c_device((VL53L1X_ADDRESS >> 1), "VL53L1X");
    probe_i2c_device(AS5600_ADDRESS, "AS5600");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = (VL53L1X_ADDRESS >> 1),
        .scl_speed_hz = 100000,
    };
    
    i2c_master_dev_handle_t vl53l1x_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &vl53l1x_handle));
    vl53l1x_set_i2c_device(vl53l1x_handle);
    
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

    ESP_LOGI(TAG, "WiFi connection initiated, waiting for connection...");
    connect_to_wifi();

    lidar_data_init();
    if (start_rest_server() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start REST server");
    }

    while(1) {
        ESP_LOGI(TAG, "Dando vueltas en el loop...");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
