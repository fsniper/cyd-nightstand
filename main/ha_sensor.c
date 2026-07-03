#include "ha_sensor.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

#include "secrets.h"

static const char *TAG = "HA_SENSOR";

esp_err_t _ha_http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (evt->user_data) {
                    strncat((char *)evt->user_data, (char *)evt->data, evt->data_len);
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t ha_sensor_fetch_temp(float *out_temp) {
    char *buffer = calloc(1, 2048);
    if (!buffer) return ESP_ERR_NO_MEM;

    esp_http_client_config_t config = {
        .url = HA_URL,
        .event_handler = _ha_http_event_handler,
        .user_data = buffer,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", HA_TOKEN);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            cJSON *root = cJSON_Parse(buffer);
            if (root) {
                cJSON *state = cJSON_GetObjectItem(root, "state");
                if (state && state->valuestring) {
                    *out_temp = atof(state->valuestring);
                }
                cJSON_Delete(root);
            }
        } else {
            ESP_LOGE(TAG, "HA HTTP Error: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(buffer);
    return err;
}
