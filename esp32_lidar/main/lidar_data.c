#include "lidar_data.h"

#include "as5600.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LIDAR_SAMPLE_PERIOD_MS 100

static const char *TAG = "lidar_data";

static SemaphoreHandle_t s_data_lock;
static TaskHandle_t s_task_handle;
static volatile bool s_running;
static lidar_sample_t s_latest;
static lidar_sample_t s_samples[LIDAR_SAMPLE_BUFFER_SIZE];
static size_t s_head;
static size_t s_count;

static void lidar_store_sample(const lidar_sample_t *sample)
{
    if (s_data_lock == NULL) {
        return;
    }

    xSemaphoreTake(s_data_lock, portMAX_DELAY);
    s_latest = *sample;
    s_samples[s_head] = *sample;
    s_head = (s_head + 1U) % LIDAR_SAMPLE_BUFFER_SIZE;
    if (s_count < LIDAR_SAMPLE_BUFFER_SIZE) {
        s_count++;
    }
    xSemaphoreGive(s_data_lock);
}

static void lidar_data_task(void *arg)
{
    (void)arg;

    while (s_running) {
        lidar_sample_t sample = {0};
        as5600_status_t as_status = {0};

        sample.timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        sample.laser_data = get_data_laser(VL53L1X_ADDRESS);

        as_status = get_as5600_status();
        sample.angle = get_as5600_angle();
        sample.agc = get_as5600_agc();
        sample.as5600_valid = as_status.valid && (sample.agc > 0) && (sample.agc < 255);

        lidar_store_sample(&sample);

        vTaskDelay(pdMS_TO_TICKS(LIDAR_SAMPLE_PERIOD_MS));
    }

    s_task_handle = NULL;
    vTaskDelete(NULL);
}

void lidar_data_init(void)
{
    if (s_data_lock == NULL) {
        s_data_lock = xSemaphoreCreateMutex();
        if (s_data_lock == NULL) {
            ESP_LOGE(TAG, "Failed to create data mutex");
        }
    }
}

esp_err_t lidar_data_start(void)
{
    if (s_task_handle != NULL) {
        return ESP_OK;
    }

    s_running = true;
    if (xTaskCreatePinnedToCore(lidar_data_task,
                                "lidar_data_task",
                                4096,
                                NULL,
                                3,
                                &s_task_handle,
                                1) != pdPASS) {
        s_running = false;
        s_task_handle = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t lidar_data_stop(void)
{
    if (s_task_handle == NULL) {
        s_running = false;
        return ESP_OK;
    }

    s_running = false;
    vTaskDelete(s_task_handle);
    s_task_handle = NULL;
    return ESP_OK;
}

bool lidar_data_is_running(void)
{
    return (s_task_handle != NULL) && s_running;
}

size_t lidar_data_get_count(void)
{
    size_t count = 0;

    if (s_data_lock == NULL) {
        return 0;
    }

    xSemaphoreTake(s_data_lock, portMAX_DELAY);
    count = s_count;
    xSemaphoreGive(s_data_lock);

    return count;
}

bool lidar_data_get_latest(lidar_sample_t *out_sample)
{
    if (out_sample == NULL || s_data_lock == NULL) {
        return false;
    }

    xSemaphoreTake(s_data_lock, portMAX_DELAY);
    if (s_count == 0) {
        xSemaphoreGive(s_data_lock);
        return false;
    }
    *out_sample = s_latest;
    xSemaphoreGive(s_data_lock);

    return true;
}

size_t lidar_data_copy_samples(lidar_sample_t *out_samples, size_t max_samples, size_t offset)
{
    size_t copied = 0;

    if (out_samples == NULL || s_data_lock == NULL || max_samples == 0) {
        return 0;
    }

    xSemaphoreTake(s_data_lock, portMAX_DELAY);
    if (offset >= s_count) {
        xSemaphoreGive(s_data_lock);
        return 0;
    }

    size_t available = s_count - offset;
    size_t to_copy = (available < max_samples) ? available : max_samples;
    size_t oldest_index = (s_head + LIDAR_SAMPLE_BUFFER_SIZE - s_count) % LIDAR_SAMPLE_BUFFER_SIZE;

    for (size_t i = 0; i < to_copy; i++) {
        size_t index = (oldest_index + offset + i) % LIDAR_SAMPLE_BUFFER_SIZE;
        out_samples[i] = s_samples[index];
    }

    copied = to_copy;
    xSemaphoreGive(s_data_lock);

    return copied;
}
