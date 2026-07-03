#ifndef MET_EIREANN_WEATHER_H
#define MET_EIREANN_WEATHER_H

#include "esp_err.h"

typedef struct {
    float temperature;
    int symbol_id;
    char symbol_name[32];
    char time_str[16]; // e.g. "12:00"
} met_forecast_t;

typedef struct {
    float temperature;
    int symbol_id;
    char symbol_name[32];
    float precipitation;
    char created_time[32]; // Forecast generation time
    met_forecast_t forecast[3];
} met_weather_t;

/**
 * @brief Fetch the current weather from Met Eireann API.
 * 
 * @param lat Latitude
 * @param lon Longitude
 * @param out_weather Pointer to store the result
 * @return esp_err_t 
 */
esp_err_t met_weather_fetch(float lat, float lon, met_weather_t *out_weather);

#endif
