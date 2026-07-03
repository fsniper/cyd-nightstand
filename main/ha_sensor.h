#ifndef HA_SENSOR_H
#define HA_SENSOR_H

#include "esp_err.h"

esp_err_t ha_sensor_fetch_temp(float *out_temp);

#endif // HA_SENSOR_H
