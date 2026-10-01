#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Home Assistant background polling service.
 * Periodically polls the outside temperature sensor via the Home Assistant REST API.
 */
void ha_client_init(void);

/**
 * @brief Get the latest cached outside temperature in Celsius.
 * 
 * @param out_temp_c Pointer to store temperature in °C
 * @return true if valid temperature has been received, false otherwise
 */
bool ha_client_get_outdoor_temp(float *out_temp_c);

#ifdef __cplusplus
}
#endif
