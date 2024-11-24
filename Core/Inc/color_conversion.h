/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COLOR_CONVERSION_H
#define COLOR_CONVERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32wbxx_hal.h"

typedef struct
{
  uint8_t R;
  uint8_t G;
  uint8_t B;
} RGB_t;

typedef struct
{
  uint16_t X;
  uint16_t Y;
} XY_t;

typedef struct
{
    uint8_t hue; /**< Hue */
    uint8_t sat; /**< Saturation */
} HS_t;

RGB_t convert_xy_to_RGB(XY_t color_xy);
RGB_t convert_HS_to_RGB(HS_t color_hs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*COLOR_CONVERSION_H */