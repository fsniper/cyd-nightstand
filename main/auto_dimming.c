#include "auto_dimming.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "auto_dimming";

#define LDR_ADC_UNIT        ADC_UNIT_1
#define LDR_ADC_CHANNEL     ADC_CHANNEL_6 // GPIO 34
#define LCD_BK_LIGHT_PIN    21

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO      LCD_BK_LIGHT_PIN
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT // 8191 max
#define LEDC_FREQUENCY      (4000)

static adc_oneshot_unit_handle_t adc1_handle;

static void auto_dimming_task(void *arg)
{
    int ldr_raw = 0;
    
    // We keep a moving average of the light to prevent flickering
    int filtered_ldr = 2000; 
    
    int log_counter = 0;
    
    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LDR_ADC_CHANNEL, &ldr_raw));
        
        // Simple low-pass filter
        filtered_ldr = (filtered_ldr * 9 + ldr_raw) / 10;
        
        // Make the minimum brightness a bit higher so it's not too dark
        int min_duty = 300;   // Increased from 50 to 300 so it remains visible
        int max_duty = 8191; // Full brightness
        
        // The user reported: Bright/Torch = 0, Dark/Covered = ~320.
        // This means the LDR pulls the pin to ground when exposed to light!
        int dark_threshold = 320; 
        
        int ldr_cap = filtered_ldr;
        if (ldr_cap > dark_threshold) ldr_cap = dark_threshold;
        
        // Inverted Linear Mapping:
        // When ldr_cap is 0 (brightest), we want max_duty.
        // When ldr_cap is dark_threshold (darkest), we subtract the full range, leaving min_duty.
        int duty = max_duty - (ldr_cap * (max_duty - min_duty)) / dark_threshold;
        
        // Log every 1 second (10 * 100ms) to avoid flooding the console
        if (++log_counter >= 10) {
            // ESP_LOGI(TAG, "LDR Raw: %4d | Filtered: %4d | PWM Duty: %4d", ldr_raw, filtered_ldr, duty);
            log_counter = 0;
        }
        
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        
        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz update
    }
}

void auto_dimming_init(void)
{
    ESP_LOGI(TAG, "Initializing auto-dimming (LDR on GPIO34, LEDC on GPIO21)");

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 8191, // Start full brightness
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = LDR_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LDR_ADC_CHANNEL, &config));

    xTaskCreate(auto_dimming_task, "auto_dimming_task", 4096, NULL, 4, NULL);
}
