#include "rmt_utils.h"

static const char * RMTTAG = "RMT_UTIL";
static xSemaphoreHandle rmt_mux;

/*
 * @brief Build register value of waveform for NEC one data bit
 */
void rmt_fill_item_level(rmt_item32_t* item, int high_us, int low_us)
{
   item->level0 = 1;
   item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
   item->level1 = 0;
   item->duration1 = (low_us) / 10 * RMT_TICK_10_US;
}

void rmt_fill_items(rmt_item32_t * items, const RMTRiseFall * data, size_t numitems)
{
    uint32_t i = 0;
    rmt_item32_t * curItem = items;


    if(items == NULL) 
        return;

    for(i = 0; i < numitems; i++)
    {
        rmt_fill_item_level(curItem, data[i].high, data[i].low);
        curItem++;
    }
}

void rmt_dump_items(rmt_item32_t * items, size_t numitems)
{
    uint32_t i = 0;
    rmt_item32_t * curItem = items;

    for (i = 0; i < numitems; i++)
    {
        ESP_LOGI(RMTTAG, "Item[%d] Level 0 [%d : %d] Level 1 [%d : %d]",
                i, curItem->level0, curItem->duration0,
                curItem->level1, curItem->duration1);
        curItem++;
    }
}

/*
 * @brief RMT transmitter initialization
 *
 * Set carrier frequency to 38 KHz, rmt_mode to transmit, 
 * duty cycle = 50% etc.
 */
void rmt_tx_init()
{
   rmt_config_t rmt_tx;
   rmt_tx.channel = RMT_TX_CHANNEL;
   rmt_tx.gpio_num = RMT_TX_GPIO_NUM;
   rmt_tx.mem_block_num = 1;
   rmt_tx.clk_div = RMT_CLK_DIV;
   rmt_tx.tx_config.loop_en = false;
   rmt_tx.tx_config.carrier_duty_percent = 50;
   rmt_tx.tx_config.carrier_freq_hz = 38000;
   rmt_tx.tx_config.carrier_level = 1;
   rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
   rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
   rmt_tx.tx_config.idle_output_en = true;
   rmt_tx.rmt_mode = RMT_MODE_TX;
   rmt_config(&rmt_tx);
   rmt_driver_install(rmt_tx.channel, 0, 0);

   // Create a mutex for tasks/threads 
   // to share the resource
   //
   rmt_mux = xSemaphoreCreateMutex();
}

void rmt_send_items(rmt_item32_t * items, size_t numitems)
{

    xSemaphoreTake(rmt_mux, portMAX_DELAY);

    //To send data according to the waveform items.
    rmt_write_items(RMT_TX_CHANNEL, items, numitems, true);
    //Wait until sending is done.
    rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);
    
    xSemaphoreGive(rmt_mux);

}


