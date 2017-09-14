#ifndef _I2C_ROUTINES_H_
#define _I2C_ROUTINES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO    22    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    21    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_1   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    400000     /*!< I2C master clock frequency */

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

void i2c_master_init();
esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t addr, uint8_t* data_rd, size_t size);
esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t addr, uint8_t regaddr, uint8_t* data_rd, size_t size);
esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t addr, uint8_t* data_wr, size_t size);
esp_err_t i2c_master_sensor_test(i2c_port_t i2c_num, uint8_t addr, uint8_t reg, uint8_t* data_h, uint8_t* data_l);

#ifdef __cplusplus
}
#endif

#endif
