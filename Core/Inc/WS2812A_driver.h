/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef WS2812A_DRIVER_H
#define WS2812A_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "color_conversion.h"
#include "stm32wbxx_hal.h"
#include "app_conf.h"
#include "stm32_seq.h"
#include <stdbool.h>

#define WS2812A_TASK    (1 << CFG_TASK_LIGHT_HANDLER)
#define WS2812A_TASK_PERIOD     40  /* task period in ms */
#define WS2812A_START_ON_LEVEL 30  /* light on level on startup */

typedef struct
{
    uint8_t level_target;   /* the level that the current level must reach; may be either level_on or 0 */
    uint8_t level_on;       /* the target level when the device is swithed on */
    uint32_t transition_time;  /* remaining level transition time [ms] */
    bool set_color_XY;      /* set color from XY space */
    bool set_color_HS;      /* set color from HS space */
    XY_t color_xy;          /* current set color in XY space */
    HS_t color_hs;          /* current set color in HS space */
} Light_Params_t;


void WS2812A_Init(SPI_HandleTypeDef* phSPI);

extern Light_Params_t light_params;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*WS2812A_DRIVER_H */