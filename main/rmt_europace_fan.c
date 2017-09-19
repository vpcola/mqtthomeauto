#include "rmt_europace_fan.h"

static const RMTRiseFall on_off[] = {
	{ 4430, 4600 },
	{ 400,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 }, 
	{ 380,  1620 },
	{ 4430, 4600 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  1620 },
	{ 380,  2560 },
	{ 380,  1620 },
	{ 380,  13800 },
	{ 9010, 2320 },
	{ 400,  50444 },
	{ 9010, 2320 },
	{ 400,  50444 },
	{ 9010, 2320 },
	{ 400,  50444 }
};
#define POWER_ON_OFF_SIZ sizeof(on_off) / sizeof(on_off[0])

static const RMTRiseFall speed[] = {
	{ 4490, 4540 },
	{ 460, 1530 },
	{ 460, 1530 },
	{ 460, 1530 },
	{ 460, 1530 },
	{ 460, 1560 },
	{ 440, 1530 },
	{ 490, 2450 },
	{ 490, 1530 },
	{ 460, 1510 },
	{ 460, 1560 },
	{ 440, 2480 },
	{ 460, 1550 },
	{ 440, 1550 },
	{ 440, 1550 },
	{ 440, 2500 },
	{ 440, 2480 },
	{ 460, 1530 },
	{ 4490, 4540 },
	{ 460, 1530 },
	{ 460, 1530 },
	{ 460, 1530 },
	{ 460, 1530 },
	{ 460, 1560 },
	{ 440, 1530 },
	{ 490, 2450 },
	{ 490, 1530 },
	{ 460, 1510 },
	{ 460, 1560 },
	{ 440, 2480 },
	{ 460, 1550 },
	{ 440, 1550 },
	{ 440, 1550 },
	{ 440, 2500 },
	{ 440, 2480 },
	{ 460, 1530 },
	{ 440, 50444},
	{ 9060, 2270 },
	{ 490, 50444}
};
#define SPEED_SIZ sizeof(speed) / sizeof(speed[0])

static const RMTRiseFall oscillate[] = {
	{ 4490, 4540 },
	{ 400, 1610 },
	{ 440, 1560 },
	{ 440, 1550 },
	{ 440, 1530 },
	{ 460, 1560 },
	{ 440, 1560 },
	{ 440, 2480 },
	{ 430, 1560 },
	{ 460, 1560 },
	{ 440, 1530 },
	{ 460, 1530 },
	{ 430, 2530 },
	{ 440, 2480 },
	{ 460, 2500 },
	{ 440, 1560 },
	{ 440, 1560 },
	{ 410, 1560 },
	{ 4460, 4540 },
	{ 430, 1580 },
	{ 440, 1560 },
	{ 410, 1580 },
	{ 380, 1610 },
	{ 440, 1550 },
	{ 440, 1530 },
	{ 430, 2510 },
	{ 430, 1580 },
	{ 410, 1580 },
	{ 440, 1530 },
	{ 430, 1580 },
	{ 410, 2530 },
	{ 440, 2510 },
	{ 440, 2500 },
	{ 440, 1560 },
	{ 440, 1560 },
	{ 410, 50444 },
	{ 9010, 2330 },
	{ 430, 50444 },
	{ 9010, 2320 },
	{ 400, 50555 }
};

#define OSCILLATE_SIZ sizeof(oscillate)/sizeof(oscillate[0])

void toggleFanOnOff(void)
{
	size_t size = POWER_ON_OFF_SIZ;

	rmt_item32_t * items = (rmt_item32_t *) malloc(size * sizeof(rmt_item32_t));
	if (items)
	{
		rmt_fill_items(items, &on_off[0], size);
		// Sent it to the rmt peripheral
		rmt_send_items(items, size);
		free(items);
	}

}

void toggleFanSpeed(void)
{
	size_t size = SPEED_SIZ;

	rmt_item32_t * items = (rmt_item32_t *) malloc(size * sizeof(rmt_item32_t));
	if (items)
	{
		rmt_fill_items(items, &speed[0], size);
		// Sent it to the rmt peripheral
		rmt_send_items(items, size);
		free(items);
	}
}

void toggleFanOscillate(void)
{
	size_t size = OSCILLATE_SIZ;

	rmt_item32_t * items = (rmt_item32_t *) malloc(size * sizeof(rmt_item32_t));
	if (items)
	{
		rmt_fill_items(items, &oscillate[0], size);
		// Sent it to the rmt peripheral
		rmt_send_items(items, size);
		free(items);
	}
}



