#include "met_eireann_weather.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WEATHER";
#define MAX_HTTP_RECV_BUFFER 16384

esp_err_t met_weather_fetch(float lat, float lon, met_weather_t *out_weather) {
    char url[128];
    snprintf(url, sizeof(url), "http://openaccess.pf.api.met.ie/metno-wdb2ts/locationforecast?lat=%.4f;long=%.4f", lat, lon);

    ESP_LOGI(TAG, "Fetching weather from %s", url);

    char *response_buffer = malloc(MAX_HTTP_RECV_BUFFER + 1);
    if (!response_buffer) return ESP_ERR_NO_MEM;

    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 30000, // Increased to 30s
        .method = HTTP_METHOD_GET,
        .user_agent = "ESP32-S3-Inky-Impression (Software Engineering Project)",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t ret = ESP_FAIL;
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        free(response_buffer);
        esp_http_client_cleanup(client);
        return err;
    }

    esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);

    if (status == 200) {
        int total_read = 0;
        while (total_read < MAX_HTTP_RECV_BUFFER) {
            int read = esp_http_client_read(client, response_buffer + total_read, MAX_HTTP_RECV_BUFFER - total_read);
            if (read <= 0) break;
            total_read += read;
        }
        response_buffer[total_read] = 0;
        
        if (total_read > 0) {
            ret = ESP_OK; // Data received
            memset(out_weather, 0, sizeof(met_weather_t));
            strcpy(out_weather->symbol_name, "Unknown");
            strcpy(out_weather->created_time, "Unknown");
            for(int i=0; i<3; i++) strcpy(out_weather->forecast[i].time_str, "--:--");

            // Parse Creation Time
            char *created_ptr = strstr(response_buffer, "created=\"");
            if (created_ptr) {
                char raw_ts[32] = {0};
                if (sscanf(created_ptr + 9, "%19s", raw_ts) == 1) {
                    // Extract just HH:MM from YYYY-MM-DDTHH:MM:SSZ
                    if (strlen(raw_ts) >= 16) {
                        snprintf(out_weather->created_time, sizeof(out_weather->created_time), "Created: %.5s", raw_ts + 11);
                    }
                }
            }

            char *search_ptr = response_buffer;
            int forecast_idx = -1; // -1 for current, 0-2 for hourly

            while ((search_ptr = strstr(search_ptr, "<time")) != NULL) {
                char from_time[32] = {0};
                char *from_ptr = strstr(search_ptr, "from=\"");
                if (from_ptr) {
                    sscanf(from_ptr + 6, "%19s", from_time);
                }

                char *to_ptr = strstr(search_ptr, "to=\"");
                char to_time[32] = {0};
                if (to_ptr) sscanf(to_ptr + 4, "%19s", to_time);

                // Parse Temperature
                char *temp_tag = strstr(search_ptr, "<temperature");
                if (temp_tag && temp_tag < strstr(search_ptr + 1, "<time")) {
                    char *val_ptr = strstr(temp_tag, "value=\"");
                    if (val_ptr) {
                        float t = strtof(val_ptr + 7, NULL);
                        if (forecast_idx == -1) out_weather->temperature = t;
                        else if (forecast_idx < 3) out_weather->forecast[forecast_idx].temperature = t;
                    }
                }

                // Parse Symbol
                char *symbol_tag = strstr(search_ptr, "<symbol");
                if (symbol_tag && symbol_tag < strstr(search_ptr + 1, "<time")) {
                    char *id_ptr = strstr(symbol_tag, "id=\"");
                    char *num_ptr = strstr(symbol_tag, "number=\"");
                    if (id_ptr && num_ptr) {
                        int sid = atoi(num_ptr + 8);
                        if (forecast_idx == -1) {
                            out_weather->symbol_id = sid;
                            sscanf(id_ptr + 4, "%31[^\"]", out_weather->symbol_name);
                        } else if (forecast_idx < 3) {
                            out_weather->forecast[forecast_idx].symbol_id = sid;
                            sscanf(id_ptr + 4, "%31[^\"]", out_weather->forecast[forecast_idx].symbol_name);
                        }
                    }
                    forecast_idx++;
                }
                
                if (forecast_idx >= 0 && forecast_idx < 3) {
                    if (strlen(from_time) >= 16) {
                        snprintf(out_weather->forecast[forecast_idx].time_str, sizeof(out_weather->forecast[forecast_idx].time_str), "%.5s", from_time + 11);
                    }
                }

                search_ptr += 5;
                if (forecast_idx >= 3) break; 
            }
        }
    }

    free(response_buffer);
    esp_http_client_cleanup(client);
    return ret;
}
