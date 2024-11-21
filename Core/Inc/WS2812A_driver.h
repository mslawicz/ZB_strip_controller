/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef WS2812A_DRIVER_H
#define WS2812A_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32wbxx_hal.h"
#include "app_conf.h"
#include "stm32_seq.h"

#define WS2812A_TASK    (1 << CFG_TASK_LIGHT_HANDLER)
#define WS2812A_TASK_PERIOD     40  /* task period in ms */

void WS2812A_Init(TIM_HandleTypeDef* phTIM, uint32_t channel);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*WS2812A_DRIVER_H */