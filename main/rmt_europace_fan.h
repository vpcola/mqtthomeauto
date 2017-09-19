#ifndef _RMT_EUROPACE_FAN_H_
#define _RMT_EUROPACE_FAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rmt_utils.h"

void toggleFanOnOff(void);
void toggleFanSpeed(void);
void toggleFanOscillate(void);

#ifdef __cplusplus
}
#endif

#endif
