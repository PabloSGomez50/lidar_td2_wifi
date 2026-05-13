/* HTTP REST API for lidar control and data. */
#include <string.h>
#include <stdlib.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "lidar_data.h"

static const char *REST_TAG = "lidar-rest";

static cJSON *lidar_sample_to_json(const lidar_sample_t *sample)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(root, "timestamp_ms", sample->timestamp_ms);
    cJSON_AddBoolToObject(root, "as5600_valid", sample->as5600_valid);
    cJSON_AddNumberToObject(root, "as5600_angle", sample->angle);
    cJSON_AddNumberToObject(root, "as5600_agc", sample->agc);

    cJSON *laser = cJSON_CreateObject();
    if (laser == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddNumberToObject(laser, "ready", sample->laser_data.ready);
    cJSON_AddNumberToObject(laser, "distance", sample->laser_data.distance);
    cJSON_AddNumberToObject(laser, "range", sample->laser_data.range);
    cJSON_AddNumberToObject(laser, "signal_rate", sample->laser_data.signal_rate);
    cJSON_AddNumberToObject(laser, "ambient_light", sample->laser_data.ambient_light);
    cJSON_AddNumberToObject(laser, "spad_num", sample->laser_data.spad_num);
    cJSON_AddItemToObject(root, "vl53l1x", laser);

    return root;
}

static esp_err_t lidar_status_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json alloc failed");
        return ESP_FAIL;
    }

    cJSON_AddBoolToObject(root, "running", lidar_data_is_running());
    cJSON_AddNumberToObject(root, "buffer_count", (int)lidar_data_get_count());
    
    const char *payload = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, payload);
    free((void *)payload);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t lidar_start_post_handler(httpd_req_t *req)
{
    (void)req;

    esp_err_t err = lidar_data_start();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "start failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "started");
    return ESP_OK;
}

static esp_err_t lidar_stop_post_handler(httpd_req_t *req)
{
    (void)req;

    esp_err_t err = lidar_data_stop();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "stop failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "stopped");
    return ESP_OK;
}

static esp_err_t lidar_latest_get_handler(httpd_req_t *req)
{
    lidar_sample_t sample = {0};
    if (!lidar_data_get_latest(&sample)) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no data");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    cJSON *root = lidar_sample_to_json(&sample);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json alloc failed");
        return ESP_FAIL;
    }

    const char *payload = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, payload);
    free((void *)payload);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t lidar_samples_get_handler(httpd_req_t *req)
{
    char query[64] = {0};
    int limit = 16;
    int offset = 0;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char value[16] = {0};
        if (httpd_query_key_value(query, "limit", value, sizeof(value)) == ESP_OK) {
            limit = atoi(value);
        }
        if (httpd_query_key_value(query, "offset", value, sizeof(value)) == ESP_OK) {
            offset = atoi(value);
        }
    }

    if (limit <= 0) {
        limit = 1;
    }
    if (limit > (int)LIDAR_SAMPLE_BUFFER_SIZE) {
        limit = LIDAR_SAMPLE_BUFFER_SIZE;
    }
    if (offset < 0) {
        offset = 0;
    }

    lidar_sample_t samples[LIDAR_SAMPLE_BUFFER_SIZE] = {0};
    size_t copied = lidar_data_copy_samples(samples, (size_t)limit, (size_t)offset);

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    if (root == NULL || arr == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(arr);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json alloc failed");
        return ESP_FAIL;
    }

    for (size_t i = 0; i < copied; i++) {
        cJSON *sample_json = lidar_sample_to_json(&samples[i]);
        if (sample_json != NULL) {
            cJSON_AddItemToArray(arr, sample_json);
        }
    }
    cJSON_AddItemToObject(root, "samples", arr);
    cJSON_AddNumberToObject(root, "offset", offset);
    cJSON_AddNumberToObject(root, "count", (int)copied);

    const char *payload = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, payload);
    free((void *)payload);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t lidar_info_handler(httpd_req_t *req)
{
    static const char html[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>ESP32 Lidar</title>\n"
        "  <style>\n"
        "    body{font-family:Arial,Helvetica,sans-serif;background:#f7f7f7;margin:0;padding:24px;}\n"
        "    h1{margin:0 0 12px;}\n"
        "    .card{background:#fff;border-radius:8px;padding:16px;box-shadow:0 2px 8px rgba(0,0,0,.08);}\n"
        "    button{margin:4px;padding:8px 12px;}\n"
        "    pre{background:#111;color:#e6e6e6;padding:12px;border-radius:6px;overflow:auto;}\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class=\"card\">\n"
        "    <h1>ESP32 Lidar</h1>\n"
        "    <div>\n"
        "      <button onclick=\"post('/api/v1/lidar/start')\">Start</button>\n"
        "      <button onclick=\"post('/api/v1/lidar/stop')\">Stop</button>\n"
        "      <button onclick=\"get('/api/v1/lidar/status')\">Status</button>\n"
        "      <button onclick=\"get('/api/v1/lidar/latest')\">Latest</button>\n"
        "      <button onclick=\"get('/api/v1/lidar/samples?limit=8&offset=0')\">Samples</button>\n"
        "    </div>\n"
        "    <h3>Response</h3>\n"
        "    <pre id=\"out\">---</pre>\n"
        "  </div>\n"
        "  <script>\n"
        "    async function get(url){\n"
        "      const res = await fetch(url);\n"
        "      const text = await res.text();\n"
        "      document.getElementById('out').textContent = text;\n"
        "    }\n"
        "    async function post(url){\n"
        "      const res = await fetch(url,{method:'POST'});\n"
        "      const text = await res.text();\n"
        "      document.getElementById('out').textContent = text;\n"
        "    }\n"
        "  </script>\n"
        "</body>\n"
        "</html>\n";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t start_rest_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(REST_TAG, "Start server failed");
        return ESP_FAIL;
    }

    httpd_uri_t root_get_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = lidar_info_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_get_uri);

    httpd_uri_t lidar_status_get_uri = {
        .uri = "/api/v1/lidar/status",
        .method = HTTP_GET,
        .handler = lidar_status_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &lidar_status_get_uri);

    httpd_uri_t lidar_start_post_uri = {
        .uri = "/api/v1/lidar/start",
        .method = HTTP_POST,
        .handler = lidar_start_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &lidar_start_post_uri);

    httpd_uri_t lidar_stop_post_uri = {
        .uri = "/api/v1/lidar/stop",
        .method = HTTP_POST,
        .handler = lidar_stop_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &lidar_stop_post_uri);

    httpd_uri_t lidar_latest_get_uri = {
        .uri = "/api/v1/lidar/latest",
        .method = HTTP_GET,
        .handler = lidar_latest_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &lidar_latest_get_uri);

    httpd_uri_t lidar_samples_get_uri = {
        .uri = "/api/v1/lidar/samples",
        .method = HTTP_GET,
        .handler = lidar_samples_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &lidar_samples_get_uri);

    return ESP_OK;
}
