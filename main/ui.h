#ifndef UI_H
#define UI_H

#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t lvgl_mux;

void ui_init(void);
void ui_set_time(const char *time_str);
void ui_set_date(const char *date_str);
void ui_set_weather(int temp, int symbol_id, const char *desc);
void ui_set_room_temp(const char *temp_str);
void ui_set_background(const char *path);
void ui_hide_splash(void);

#endif // UI_H
