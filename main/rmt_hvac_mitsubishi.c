#include "rmt_hvac_mitsubishi.h"

static const uint8_t cmddata[18] = { 0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06, 0x30, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F };

// static const char * RMT_M = "RMT_MITSUBISHI";

static rmt_item32_t * generateHvacCommand(size_t * itemsiz, 
                HvacMode_t mode, 
                int  temp,
                HvacFanMode_t     fan_mode,
                HvacVanneMode_t   vanne_mode,
                int               on_off)
{
    int i, j;
    uint8_t mask = 1;
    uint8_t data[18];

    // Copy command data
    memcpy(data, cmddata, 18);
    // Modify some bytes based on modes passed
    
    // Byte 6 - On/Off
    data[5] = (on_off == 0) ? 0x0 : 0x20; // Set 0 to OFF, 0x20 to turn ON 

    // Byte 7 - Mode
    switch(mode)
    {
        case HVAC_HOT  : data[6] = (uint8_t) 0x08; break;
        case HVAC_COLD : data[6] = (uint8_t) 0x18; break;
        case HVAC_DRY  : data[6] = (uint8_t) 0x10; break;
        case HVAC_AUTO : data[6] = (uint8_t) 0x20; break;
        default: break;
    }

    // Byte 8 - Temperature
    int realTemp;
    if (temp > 31) 
        realTemp = 31; // Max out at 31
    if (temp < 16)
        realTemp = 16; // Min at 16
    else
        realTemp = temp;
    // Actual temp is offset from 16
    data[7] = realTemp - 16;


    // Byte 10 - Fan/Vanne mode
    switch(fan_mode)
    {
        case FAN_SPEED_1:       data[9] = (uint8_t) 0x01; break;
        case FAN_SPEED_2:       data[9] = (uint8_t) 0x02; break;
        case FAN_SPEED_3:       data[9] = (uint8_t) 0x03; break;
        case FAN_SPEED_4:       data[9] = (uint8_t) 0x04; break;
        case FAN_SPEED_5:       data[9] = (uint8_t) 0x04; break; //No FAN speed 5 for MITSUBISHI so it is consider as Speed 4
        case FAN_SPEED_AUTO:    data[9] = (uint8_t) 0x80; break;
        case FAN_SPEED_SILENT:  data[9] = (uint8_t) 0x05; break;
        default: break;                           
    }

    switch (vanne_mode)
    {
        case VANNE_AUTO:        data[9] = (uint8_t) data[9] | 0x40; break;
        case VANNE_H1:          data[9] = (uint8_t) data[9] | 0x48; break;
        case VANNE_H2:          data[9] = (uint8_t) data[9] | 0x50; break;
        case VANNE_H3:          data[9] = (uint8_t) data[9] | 0x58; break;
        case VANNE_H4:          data[9] = (uint8_t) data[9] | 0x60; break;
        case VANNE_H5:          data[9] = (uint8_t) data[9] | 0x68; break;
        case VANNE_AUTO_MOVE:   data[9] = (uint8_t) data[9] | 0x78; break;
        default: break;
    }

    // Byte 18 - CRC
    data[17] = 0;
    for (i = 0; i < 17; i++) {
        data[17] = (uint8_t) data[i] + data[17];  // CRC is a simple bits addition
    }


    // Total number of items = ((1 header item) + (18 bytes * 8) data items + (1 footer item)) * 2 (twice transmitted)
    //                       =  292 items
    rmt_item32_t * items = malloc( 292 * sizeof(rmt_item32_t));
    if (items)
    {
        int curItem = 0;
        // Fillup all 76 items ..
        for (i = 0; i < 2; i++)
        {
            // Fill header
            rmt_fill_item_level(&items[curItem], HVAC_MITSUBISHI_HDR_MARK, HVAC_MITSUBISHI_HDR_SPACE);
            curItem++;
            // Form all items from the 18 bytes data
            for(j = 0; j < 18; j++)
            {
                // Set bits in the byte (reverse order)
                for(mask = 00000001 ; mask > 0; mask <<=1 )
                {
                    // Test if 0 or 1
                    if (data[j] & mask) { // bit is set ONE
                        rmt_fill_item_level(&items[curItem], HVAC_MITSUBISHI_BIT_MARK, HVAC_MITSUBISHI_ONE_SPACE);
                    }else                 // bit is set ZERO
                    {
                        rmt_fill_item_level(&items[curItem], HVAC_MITSUBISHI_BIT_MARK, HVAC_MITSUBISHI_ZERO_SPACE);
                    }
                    curItem++;
                }
            }
            // Fill the footer/end
            rmt_fill_item_level(&items[curItem], HVAC_MITSUBISHI_RPT_MARK, HVAC_MITSUBISHI_RPT_SPACE);
            curItem++;
        }
        *itemsiz = curItem;
    }else
    {
        *itemsiz = 0;
    }

    return items;
}

int sendHvacCommand(HvacMode_t      mode,
    int             temp,
    HvacFanMode_t   fan_mode,
    HvacVanneMode_t vanne_mode,
    int             on_off)
{
    size_t numitems;

    rmt_item32_t * items = generateHvacCommand(&numitems, 
          mode, temp, fan_mode, vanne_mode, on_off);

    if (items)
    {
        // Send commands to rmt perepheral
        rmt_send_items(items, numitems);
        // Free the data returned by generateHvacCommand
        free(items);

        return ESP_OK;
    }

    return ESP_FAIL;
}


