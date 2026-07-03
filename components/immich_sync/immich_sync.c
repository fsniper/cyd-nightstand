#include "immich_sync.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "IMMICH_SYNC";

#include "secrets.h"
#define PHOTOS_DIR "/sdcard/photos"

static esp_err_t download_to_file(const char *url, const char *filepath) {
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .buffer_size = 4096,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "HTTP Error %d", status);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    FILE *f = fopen(filepath, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    char *buf = malloc(4096);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate download buffer");
        fclose(f);
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }

    int read_len;
    while ((read_len = esp_http_client_read(client, buf, 4096)) > 0) {
        fwrite(buf, 1, read_len, f);
    }
    
    if (read_len < 0) {
        ESP_LOGE(TAG, "Error reading HTTP data");
        err = ESP_FAIL;
    } else {
        err = ESP_OK;
    }

    fclose(f);
    free(buf);
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t immich_sync_run(void) {
    ESP_LOGI(TAG, "Starting Immich Sync...");

    // Create photos dir if it doesn't exist
    mkdir(PHOTOS_DIR, 0777);

    char url[512];
    snprintf(url, sizeof(url), "%s/api/albums/%s?key=%s", IMMICH_BASE_URL, IMMICH_ALBUM_ID, IMMICH_SHARE_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
        .buffer_size = 16384,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    esp_http_client_fetch_headers(client);

    int content_length = esp_http_client_get_content_length(client);
    if (content_length <= 0) content_length = 16384; 

    char *json_buf = malloc(content_length + 1);
    if (!json_buf) {
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }

    int read_len = 0;
    int chunk;
    while ((chunk = esp_http_client_read(client, json_buf + read_len, content_length - read_len)) > 0) {
        read_len += chunk;
    }
    json_buf[read_len] = 0;
    esp_http_client_cleanup(client);

    cJSON *root = cJSON_Parse(json_buf);
    free(json_buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *assets = cJSON_GetObjectItem(root, "assets");
    if (!assets || !cJSON_IsArray(assets)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // Array of remote IDs
    int remote_count = cJSON_GetArraySize(assets);
    char **remote_ids = malloc(remote_count * sizeof(char*));
    for (int i = 0; i < remote_count; i++) {
        cJSON *asset = cJSON_GetArrayItem(assets, i);
        cJSON *id_item = cJSON_GetObjectItem(asset, "id");
        if (id_item && id_item->valuestring) {
            remote_ids[i] = strdup(id_item->valuestring);
        } else {
            remote_ids[i] = NULL;
        }
    }
    cJSON_Delete(root);

    // Scan local directory for .jpg files
    DIR *dir = opendir(PHOTOS_DIR);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".jpg")) {
                // Check if this local file exists in remote
                bool found = false;
                for (int i = 0; i < remote_count; i++) {
                    if (remote_ids[i] && strncmp(ent->d_name, remote_ids[i], strlen(remote_ids[i])) == 0) {
                        found = true;
                        break;
                    }
                }
                
                // If removed from share, delete locally
                if (!found) {
                    char del_path[512];
                    snprintf(del_path, sizeof(del_path), "%s/%s", PHOTOS_DIR, ent->d_name);
                    ESP_LOGI(TAG, "Removing local file not in share: %s", del_path);
                    unlink(del_path);
                }
            }
        }
        closedir(dir);
    }

    // Download missing assets
    for (int i = 0; i < remote_count; i++) {
        if (!remote_ids[i]) continue;

        char jpg_path[512];
        snprintf(jpg_path, sizeof(jpg_path), "%s/%s_local.jpg", PHOTOS_DIR, remote_ids[i]);

        struct stat st;
        if (stat(jpg_path, &st) == 0) {
            // Already downloaded
            continue;
        }

        ESP_LOGI(TAG, "Downloading new asset: %s", remote_ids[i]);
        char asset_url[512];
        snprintf(asset_url, sizeof(asset_url), "%s/api/assets/%s/thumbnail?size=thumbnail&key=%s", IMMICH_BASE_URL, remote_ids[i], IMMICH_SHARE_KEY);
        
        if (download_to_file(asset_url, jpg_path) == ESP_OK) {
            ESP_LOGI(TAG, "Download successful.");
        } else {
            ESP_LOGE(TAG, "Download failed for %s", remote_ids[i]);
        }
    }

    for (int i = 0; i < remote_count; i++) {
        if (remote_ids[i]) free(remote_ids[i]);
    }
    free(remote_ids);

    ESP_LOGI(TAG, "Immich Sync Complete.");
    return ESP_OK;
}
