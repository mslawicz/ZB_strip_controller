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

#define WS2812A_NUMB_DEV  8     /* number of WS2812A devices in the strip */
#define WS2812A_DEV_SIZE  24    /* number of bytes per WS2812A device */
#define WS2812A_PWM_SIZE  (WS2812A_NUMB_DEV * WS2812A_DEV_SIZE + 1)   /* size of WS2812A PWM buffer */
#define WS2812A_PWM_ZERO  26    /* PWM value for bit 0 */
#define WS2812A_PWM_ONE   53    /* PWM value for bit 1 */

static TIM_HandleTypeDef* pWS2812A_TIM;
static uint32_t WS2812A_channel;
static uint8_t WS2812A_PWM_data[WS2812A_PWM_SIZE] = {0};

void WS2812A_Init(TIM_HandleTypeDef* phTIM, uint32_t channel)
{
  pWS2812A_TIM = phTIM;
  WS2812A_channel = channel;

  //XXX PWM test
  uint8_t k;
  for(k=0; k<24; k++)
  {
    WS2812A_PWM_data[k] = WS2812A_PWM_ZERO;
  }
  WS2812A_PWM_data[3] = WS2812A_PWM_ONE;
  WS2812A_PWM_data[12] = WS2812A_PWM_ONE;
  WS2812A_PWM_data[22] = WS2812A_PWM_ONE;
}

void WS2812A_test(void)
{
  HAL_TIM_PWM_Start_DMA(pWS2812A_TIM, WS2812A_channel, (const uint32_t*)WS2812A_PWM_data, 25);
}