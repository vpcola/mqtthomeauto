#ifndef _RMT_UTILS_H_
#define _RMT_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#define RMT_TX_CARRIER_EN    1   /*!< Enable carrier for IR transmitter test with IR led */

#define RMT_TX_CHANNEL    1     /*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM  16     /*!< GPIO number for transmitter signal */
#define RMT_CLK_DIV      100    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */

typedef struct _rmt_rise_fall_data {
    int high;
    int low;
} RMTRiseFall;
    
void rmt_fill_item_level(rmt_item32_t* item, int high_us, int low_us);
void rmt_fill_items(rmt_item32_t * items, const RMTRiseFall * data, size_t numitems);
void rmt_dump_items(rmt_item32_t * items, size_t numitems);
void rmt_send_items(rmt_item32_t * items, size_t numitems);
void rmt_tx_init();


#ifdef __cplusplus
}
#endif

#endif
