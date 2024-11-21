/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    WS2812A_driver.c
  * @author  Marcin Slawicz
  * @brief   driver for the WS2812A addressable LEDs
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 Marcin Slawicz.
  * All rights reserved.
  *
  ******************************************************************************
  */


#include "WS2812A_driver.h"
#include "main.h"
#include <string.h>

#define WS2812A_NUMB_DEV  8     /* number of WS2812A devices in the strip */
#define WS2812A_DEV_SIZE  24    /* number of bytes per WS2812A device */
#define WS2812A_PWM_SIZE  (WS2812A_NUMB_DEV * WS2812A_DEV_SIZE + 1)   /* size of WS2812A PWM buffer */
#define WS2812A_PWM_ZERO  26    /* PWM value for bit 0 */
#define WS2812A_PWM_ONE   53    /* PWM value for bit 1 */
#define WS2812A_RGB_WHITE 0x7F  /* RGB value for the white color */

typedef struct
{
  uint8_t R;
  uint8_t G;
  uint8_t B;
} RGB_t;

static TIM_HandleTypeDef* pWS2812A_TIM;
static uint32_t WS2812A_channel;
static uint8_t WS2812A_PWM_data[WS2812A_PWM_SIZE];
static RGB_t WS2812A_RGB_data[WS2812A_NUMB_DEV];

void WS2812A_handler(void);

void WS2812A_Init(TIM_HandleTypeDef* phTIM, uint32_t channel)
{
  pWS2812A_TIM = phTIM;
  WS2812A_channel = channel;

  /*  initialize buffer with bits 0 */
  memset(WS2812A_PWM_data, WS2812A_PWM_ZERO, WS2812A_PWM_SIZE);
  /* trailing PWM pulse with duty 0 */
  WS2812A_PWM_data[WS2812A_PWM_SIZE - 1] = 0;
  /* initialize RGB buffer with white color values */
  memset(WS2812A_RGB_data, WS2812A_RGB_WHITE, sizeof(WS2812A_RGB_data));

  /* register WS2812A handler task */
  UTIL_SEQ_RegTask(WS2812A_TASK, 0, WS2812A_handler);
}

void WS2812A_handler(void)
{
  uint8_t transmit_request = 1;

  if(transmit_request)
  {
    /* generate PWM values and place them in the PWM buffer */
    size_t dev_index;
    uint32_t bit_buffer;
    for(dev_index = 0; dev_index < WS2812A_NUMB_DEV; dev_index++)
    {
      /* place data in order G R B */
      bit_buffer = (WS2812A_RGB_data[dev_index].G << 16) |
                   (WS2812A_RGB_data[dev_index].R << 8) |
                   WS2812A_RGB_data[dev_index].B;
      uint8_t bit_index;
      uint32_t bit_mask = 0x00800000;
      /* fill buffer with PWM values for all bits of one device */
      for(bit_index = 0; bit_index < WS2812A_DEV_SIZE; bit_index++)
      {
        WS2812A_PWM_data[dev_index * WS2812A_DEV_SIZE + bit_index] = ((bit_buffer & bit_mask) == 0 ? WS2812A_PWM_ZERO : WS2812A_PWM_ONE);
        bit_mask >>= 1;
      }
    }

    /* transmit data to all WS2812A devices */
    HAL_TIM_PWM_Start_DMA(pWS2812A_TIM, WS2812A_channel, (const uint32_t*)WS2812A_PWM_data, WS2812A_PWM_SIZE);
  }
}