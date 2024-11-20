/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef WS2812A_DRIVER_H
#define WS2812A_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32wbxx_hal.h"

void WS2812A_Init(TIM_HandleTypeDef* phTIM, uint32_t channel);
void WS2812A_test(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*WS2812A_DRIVER_H */