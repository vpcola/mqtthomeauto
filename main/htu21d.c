#include "htu21d.h"

/**
* @brief Reads temperature data from HTU21d sensor
**/
esp_err_t htu21d_temperature(float * temperature)
{
       uint8_t data_h, data_l;

       if (temperature == NULL) return ESP_FAIL;

       if (i2c_master_sensor_test( I2C_MASTER_NUM, 
			       HTU21D_I2C_ADDRESS,
			       TRIGGER_TEMP_MEASURE, 
			       &data_h,
			       &data_l) == ESP_OK)
       {
	       // Algorithm from datasheet to compute temperature.
	       unsigned int rawTemperature = ((unsigned int) data_h << 8) | (unsigned int) data_l;
		       rawTemperature &= 0xFFFC;

	       float temp = rawTemperature / (float)65536; //2^16 = 65536
	       *temperature = -46.85 + (175.72 * temp); //From page 14

	       return ESP_OK;
       }

       return ESP_FAIL;
}

/**
* @brief Reads humidity data from HTU21d sensor
**/
esp_err_t htu21d_humidity(float * humidity)
{
      uint8_t data_h, data_l;

      if (humidity == NULL) return ESP_FAIL;

      if (i2c_master_sensor_test( I2C_MASTER_NUM, 
			      HTU21D_I2C_ADDRESS,
			      TRIGGER_HUMD_MEASURE, 
			      &data_h,
			      &data_l) == ESP_OK)
      {
	       // Algorithm from datasheet to compute temperature.
	       unsigned int rawHumidity = ((unsigned int) data_h << 8) | (unsigned int) data_l;
		       rawHumidity &= 0xFFFC; //Zero out the status bits but keep them in place
   
	       //Given the raw humidity data, calculate the actual relative humidity
	       float temp = rawHumidity / (float)65536; //2^16 = 65536
	       *humidity = -6 + (125 * temp); //From page 14	       

	       return ESP_OK;
      }

      return ESP_FAIL;
}


