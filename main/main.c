#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_ili9341.h"
#include "lvgl.h"
#include "ui.h"
#include "wifi_sntp.h"
#include "auto_dimming.h"
#include "auto_dimming.h"
#include "sd_card.h"
#include "immich_sync.h"
#include "met_eireann_weather.h"
#include "ha_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include <dirent.h>

static const char *TAG = "cyd_clock";

// Global weather data
met_weather_t current_weather = {0};
bool weather_valid = false;

static void weather_task(void *arg) {
    // Wait for WiFi to connect (give it 10 seconds)
    vTaskDelay(pdMS_TO_TICKS(10000));

    int loop_count = 0;

    while (1) {
        ESP_LOGI(TAG, "Fetching Weather...");
        if (met_weather_fetch(53.217, -6.663, &current_weather) == ESP_OK) {
            weather_valid = true;
            ui_set_weather((int)current_weather.temperature, current_weather.symbol_id, current_weather.symbol_name);
        } else {
            ESP_LOGE(TAG, "Failed to fetch weather");
        }

        ESP_LOGI(TAG, "Fetching Room Temperature...");
        float room_temp = 0;
        if (ha_sensor_fetch_temp(&room_temp) == ESP_OK) {
            char temp_str[32];
            snprintf(temp_str, sizeof(temp_str), "In: %.1f°C", room_temp);
            ui_set_room_temp(temp_str);
        } else {
            ESP_LOGE(TAG, "Failed to fetch room temperature");
        }

        // Photo sync and change only every 2 hours (120 mins).
        // Since loop runs every 15 mins, 8 * 15 = 120 mins.
        if (loop_count % 8 == 0) {
#if 0
            ESP_LOGI(TAG, "Syncing photos...");
            immich_sync_run();
            
            // Pick a random photo from the SD card and set as background
            DIR *dir = opendir("/sdcard/photos");
            if (dir) {
                char *jpgs[64];
                int count = 0;
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL && count < 64) {
                    if (strstr(ent->d_name, "_local.jpg") || strstr(ent->d_name, "_local.JPG")) {
                        jpgs[count++] = strdup(ent->d_name);
                    }
                }
                closedir(dir);

                if (count > 0) {
                    int idx = esp_random() % count;
                    char bg_path[256];
                    // LVGL FS uses 'A:/' for STDIO
                    snprintf(bg_path, sizeof(bg_path), "A:/sdcard/photos/%s", jpgs[idx]);
                    ui_set_background(bg_path);
                    ESP_LOGI(TAG, "Set background to %s", bg_path);
                }
                
                for (int i = 0; i < count; i++) free(jpgs[i]);
            }
#endif
        }

        static bool first_run = true;
        if (first_run) {
            ui_hide_splash();
            first_run = false;
        }

        loop_count++;
        // Wait 15 minutes before refreshing
        vTaskDelay(pdMS_TO_TICKS(15 * 60 * 1000));
    }
}

// --- PINS FOR STANDARD CYD (Cheap Yellow Display) ESP32-2432S028R ---
#define LCD_HOST       SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_PIN_NUM_SCLK      14
#define LCD_PIN_NUM_MOSI      13
#define LCD_PIN_NUM_MISO      12
#define LCD_PIN_NUM_LCD_DC    2
#define LCD_PIN_NUM_LCD_RST   -1 // Usually connected to EN
#define LCD_PIN_NUM_LCD_CS    15
#define LCD_PIN_NUM_BK_LIGHT  21
#define LCD_H_RES             320
#define LCD_V_RES             240
#define LCD_CMD_BITS          8
#define LCD_PARAM_BITS        8

static esp_lcd_panel_handle_t panel_handle = NULL;
SemaphoreHandle_t lvgl_mux = NULL;


// LVGL flush callback
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    uint16_t *buf16 = (uint16_t *)px_map;
    uint32_t size = (offsetx2 - offsetx1 + 1) * (offsety2 - offsety1 + 1);
    for (uint32_t i = 0; i < size; i++) {
        buf16[i] = (buf16[i] >> 8) | (buf16[i] << 8); // Swap bytes
    }
    
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
    // lv_display_flush_ready(disp) is now called by the DMA completion callback!
}

// Increase LVGL tick
static void increase_lvgl_tick(void *arg)
{
    // Tell LVGL how many milliseconds has elapsed
    lv_tick_inc(2);
}

static void gui_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (lvgl_mux && xSemaphoreTake(lvgl_mux, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(lvgl_mux);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_NUM_SCLK,
        .mosi_io_num = LCD_PIN_NUM_MOSI,
        .miso_io_num = LCD_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_NUM_LCD_DC,
        .cs_gpio_num = LCD_PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB, // Switching to RGB to fix Cyan appearing as Yellow
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Keep hardware in native portrait mode, but mirror columns as suggested by AliExpress comments
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Turn on auto-dimming LCD backlight");
    // LCD Backlight is now controlled by PWM auto-dimming
    auto_dimming_init();

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // Create a display with native resolution (240x320)
    // We pass V_RES as width and H_RES as height because it's portrait
    lv_display_t * disp = lv_display_create(LCD_V_RES, LCD_H_RES);
    lv_display_set_user_data(disp, panel_handle);

    // Register DMA callback so LVGL waits for DMA before reusing the buffer
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp);

    // Tell LVGL to rotate 90 degrees to landscape
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    // Initialize SD Card
    sd_card_init();

    // Alloc draw buffers used by LVGL
    size_t draw_buffer_sz = LCD_H_RES * 40 * sizeof(lv_color16_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);
    void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);
    lv_display_set_buffers(disp, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));

    lvgl_mux = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "Initialize LVGL UI");
    ui_init();

    // Force LVGL to redraw the entire screen to overwrite old data (fixes white block artifacts)
    lv_obj_invalidate(lv_screen_active());

    // Start the GUI task to handle LVGL updates in the background
    // Increased stack size from 4096 to 16384 to prevent stack overflow during image decoding
    xTaskCreatePinnedToCore(gui_task, "gui_task", 16384, NULL, 5, NULL, 1);

    // Start Weather and Photo sync task
    xTaskCreate(weather_task, "weather_task", 8192, NULL, 4, NULL);

    ESP_LOGI(TAG, "Starting WiFi and SNTP");
    // This will block until WiFi is connected, but the GUI task will keep running!
    wifi_sntp_init();
}
