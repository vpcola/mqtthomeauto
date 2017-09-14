#ifndef _HTU21D_H_
#define _HTU21D_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c_routines.h"

#define HTU21D_I2C_ADDRESS  0x40
#define TRIGGER_TEMP_MEASURE  0xE3
#define TRIGGER_HUMD_MEASURE  0xE5

esp_err_t htu21d_temperature(float * temperature);
esp_err_t htu21d_humidity(float * humidity);



#ifdef __cplusplus
}
#endif

#endif
