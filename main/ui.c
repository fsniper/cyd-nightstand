#include "ui.h"

static lv_obj_t *time_label;
static lv_obj_t *date_label;
static lv_obj_t *bg_img;
static lv_obj_t *weather_temp_label;
static lv_obj_t *weather_desc_label;
static lv_obj_t *weather_icon_img;
static lv_obj_t *room_temp_label;
static lv_obj_t *splash_scr;

extern const uint8_t icon_0_png_start[] asm("_binary_icon_0_png_start");
extern const uint8_t icon_1_png_start[] asm("_binary_icon_1_png_start");
extern const uint8_t icon_2_png_start[] asm("_binary_icon_2_png_start");
extern const uint8_t icon_3_png_start[] asm("_binary_icon_3_png_start");
extern const uint8_t icon_4_png_start[] asm("_binary_icon_4_png_start");
extern const uint8_t icon_5_png_start[] asm("_binary_icon_5_png_start");
extern const uint8_t icon_6_png_start[] asm("_binary_icon_6_png_start");
extern const uint8_t icon_7_png_start[] asm("_binary_icon_7_png_start");
extern const uint8_t icon_8_png_start[] asm("_binary_icon_8_png_start");
extern const uint8_t icon_9_png_start[] asm("_binary_icon_9_png_start");
extern const uint8_t icon_10_png_start[] asm("_binary_icon_10_png_start");
extern const uint8_t icon_11_png_start[] asm("_binary_icon_11_png_start");

extern const uint8_t icon_0_png_end[] asm("_binary_icon_0_png_end");
extern const uint8_t icon_1_png_end[] asm("_binary_icon_1_png_end");
extern const uint8_t icon_2_png_end[] asm("_binary_icon_2_png_end");
extern const uint8_t icon_3_png_end[] asm("_binary_icon_3_png_end");
extern const uint8_t icon_4_png_end[] asm("_binary_icon_4_png_end");
extern const uint8_t icon_5_png_end[] asm("_binary_icon_5_png_end");
extern const uint8_t icon_6_png_end[] asm("_binary_icon_6_png_end");
extern const uint8_t icon_7_png_end[] asm("_binary_icon_7_png_end");
extern const uint8_t icon_8_png_end[] asm("_binary_icon_8_png_end");
extern const uint8_t icon_9_png_end[] asm("_binary_icon_9_png_end");
extern const uint8_t icon_10_png_end[] asm("_binary_icon_10_png_end");
extern const uint8_t icon_11_png_end[] asm("_binary_icon_11_png_end");

static const void* get_icon_src(int symbol_id) {
    int col = 0, row = 0;
    switch (symbol_id) {
        case 1: col = 3; row = 1; break; // Sun
        case 2:
        case 3: col = 1; row = 0; break; // SunCloud
        case 4: col = 0; row = 0; break; // Cloudy
        case 5: col = 2; row = 1; break; // Rain Showers
        case 6:
        case 11: col = 3; row = 0; break; // Thunder
        case 9: col = 0; row = 1; break; // Rain
        case 12:
        case 13:
        case 14: col = 1; row = 1; break; // Snow
        case 15: col = 1; row = 2; break; // Fog
        default: col = 0; row = 0; break;
    }
    int idx = row * 4 + col;
    return NULL; // Not used directly anymore
}


void ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN); // Pure black

    // Background Image
    bg_img = lv_image_create(scr);
    lv_obj_set_size(bg_img, 320, 240);
    lv_obj_align(bg_img, LV_ALIGN_CENTER, 0, 0);

    // Weather Panel (Glassmorphism)
    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_set_size(panel, 120, 240);
    lv_obj_align(panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(panel, 80, 0); // Much lighter transparency
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 0, 0);

    // Weather Icon
    weather_icon_img = lv_image_create(panel);
    lv_obj_align(weather_icon_img, LV_ALIGN_TOP_MID, 0, 10);
    
    // Weather Temp
    weather_temp_label = lv_label_create(panel);
    lv_obj_set_style_text_font(weather_temp_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(weather_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(weather_temp_label, "-- °C");
    lv_obj_align(weather_temp_label, LV_ALIGN_TOP_MID, 0, 80);

    // Weather Desc
    weather_desc_label = lv_label_create(panel);
    lv_obj_set_style_text_font(weather_desc_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(weather_desc_label, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(weather_desc_label, "Loading...");
    lv_obj_align(weather_desc_label, LV_ALIGN_TOP_MID, 0, 110);

    // Room Temp
    room_temp_label = lv_label_create(panel);
    lv_obj_set_style_text_font(room_temp_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(room_temp_label, lv_color_hex(0x00FF00), 0); // Green to stand out
    lv_label_set_text(room_temp_label, "In: -- °C");
    lv_obj_align(room_temp_label, LV_ALIGN_TOP_MID, 0, 135);

    // Time Label (Move left)
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "12:00");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00E5FF), LV_PART_MAIN); 
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 20, -20); 

    // Date Label (Move left)
    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "Mon, Jan 01");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN); 
    lv_obj_align(date_label, LV_ALIGN_LEFT_MID, 20, 40); 

    // Splash Screen (on top of everything)
    splash_scr = lv_obj_create(scr);
    lv_obj_set_size(splash_scr, 320, 240);
    lv_obj_align(splash_scr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(splash_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(splash_scr, 255, 0);
    lv_obj_set_style_border_width(splash_scr, 0, 0);
    lv_obj_set_style_radius(splash_scr, 0, 0);

    lv_obj_t *splash_label = lv_label_create(splash_scr);
    lv_label_set_text(splash_label, "Loading...");
    lv_obj_set_style_text_font(splash_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(splash_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(splash_label, LV_ALIGN_CENTER, 0, 0);
}

void ui_set_time(const char *time_str)
{
    if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        if(time_label) {
            lv_label_set_text(time_label, time_str);
        }
        xSemaphoreGive(lvgl_mux);
    }
}

void ui_set_date(const char *date_str)
{
    if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        if(date_label) {
            lv_label_set_text(date_label, date_str);
        }
        xSemaphoreGive(lvgl_mux);
    }
}

void ui_set_room_temp(const char *temp_str)
{
    if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        if(room_temp_label) {
            lv_label_set_text(room_temp_label, temp_str);
        }
        xSemaphoreGive(lvgl_mux);
    }
}

void ui_hide_splash(void) {
    if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        if (splash_scr) {
            lv_obj_delete(splash_scr);
            splash_scr = NULL;
        }
        xSemaphoreGive(lvgl_mux);
    }
}

static lv_image_dsc_t current_icon_dsc;

void ui_set_weather(int temp, int symbol_id, const char *desc) {
    if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        if (weather_temp_label) {
            lv_label_set_text_fmt(weather_temp_label, "%d °C", temp);
        }
        if (weather_desc_label) {
            lv_label_set_text(weather_desc_label, desc);
        }
        if (weather_icon_img) {
            const uint8_t* src = NULL;
            uint32_t size = 0;
            
            int col = 0, row = 0;
            switch (symbol_id) {
                case 1: col = 3; row = 1; break; // Sun
                case 2:
                case 3: col = 1; row = 0; break; // SunCloud
                case 4: col = 0; row = 0; break; // Cloudy
                case 5: col = 2; row = 1; break; // Rain Showers
                case 6:
                case 11: col = 3; row = 0; break; // Thunder
                case 9: col = 0; row = 1; break; // Rain
                case 12:
                case 13:
                case 14: col = 1; row = 1; break; // Snow
                case 15: col = 1; row = 2; break; // Fog
                default: col = 0; row = 0; break;
            }
            int idx = row * 4 + col;
            
            switch(idx) {
                case 0: src = icon_0_png_start; size = (uint32_t)(icon_0_png_end - icon_0_png_start); break;
                case 1: src = icon_1_png_start; size = (uint32_t)(icon_1_png_end - icon_1_png_start); break;
                case 2: src = icon_2_png_start; size = (uint32_t)(icon_2_png_end - icon_2_png_start); break;
                case 3: src = icon_3_png_start; size = (uint32_t)(icon_3_png_end - icon_3_png_start); break;
                case 4: src = icon_4_png_start; size = (uint32_t)(icon_4_png_end - icon_4_png_start); break;
                case 5: src = icon_5_png_start; size = (uint32_t)(icon_5_png_end - icon_5_png_start); break;
                case 6: src = icon_6_png_start; size = (uint32_t)(icon_6_png_end - icon_6_png_start); break;
                case 7: src = icon_7_png_start; size = (uint32_t)(icon_7_png_end - icon_7_png_start); break;
                case 8: src = icon_8_png_start; size = (uint32_t)(icon_8_png_end - icon_8_png_start); break;
                case 9: src = icon_9_png_start; size = (uint32_t)(icon_9_png_end - icon_9_png_start); break;
                case 10: src = icon_10_png_start; size = (uint32_t)(icon_10_png_end - icon_10_png_start); break;
                case 11: src = icon_11_png_start; size = (uint32_t)(icon_11_png_end - icon_11_png_start); break;
            }
            
            if (src) {
                current_icon_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
                current_icon_dsc.header.cf = LV_COLOR_FORMAT_NATIVE; // Let lodepng handle it
                current_icon_dsc.header.w = 64;
                current_icon_dsc.header.h = 64;
                current_icon_dsc.data_size = size;
                current_icon_dsc.data = src;
                lv_image_set_src(weather_icon_img, &current_icon_dsc);
            }
        }
        xSemaphoreGive(lvgl_mux);
    }
}

void ui_set_background(const char *path) {
    if (lvgl_mux && xSemaphoreTake(lvgl_mux, portMAX_DELAY) == pdTRUE) {
        if (bg_img) {
            lv_image_set_src(bg_img, path);
        }
        xSemaphoreGive(lvgl_mux);
    }
}
