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
#define WS2812A_DEV_SIZE  15    /* number of bytes per WS2812A device (24 RGB bits * 5 pulse bits = 120 bits) */
#define WS2812A_PULSE_BUF_SIZE  (WS2812A_NUMB_DEV * WS2812A_DEV_SIZE)   /* size of WS2812A pulse buffer */
#define WS2812A_PULSE_ZERO  0x10    /* 5-bit SPI pulse creating device bit 0 */
#define WS2812A_PULSE_ONE   0x1C    /* 5-bit SPI pulse creating device bit 1 */
#define WS2812A_RGB_WHITE 0x7F  /* RGB value for the white color */

static SPI_HandleTypeDef* pWS2812A_SPI;
static uint8_t WS2812A_pulse_buffer[WS2812A_PULSE_BUF_SIZE];
static RGB_t WS2812A_RGB_data[WS2812A_NUMB_DEV];
static float level_current = 0.0f;   /* current light level <0.0,255.0> */

Light_Params_t light_params =
{
    .level_target = 0,
    .level_on = WS2812A_START_ON_LEVEL,
    .transition_time = 0,
    .set_color_XY = false,
    .set_color_HS = false
};

void WS2812A_handler(void);

void WS2812A_Init(SPI_HandleTypeDef* phSPI)
{
  pWS2812A_SPI = phSPI;

  /* initialize RGB buffer with white color values */
  memset(WS2812A_RGB_data, WS2812A_RGB_WHITE, sizeof(WS2812A_RGB_data));

  /* register WS2812A handler task */
  UTIL_SEQ_RegTask(WS2812A_TASK, 0, WS2812A_handler);
}

/* takes 8 bits and generates 8 5-bit pulses in the buffer */
void bits_to_pulses(uint8_t color_value, uint8_t** ppBuffer)
{
  uint64_t pulse_buffer = 0;
  uint8_t bit_index;

  for(bit_index = 0; bit_index < 8; bit_index++)
  {
    pulse_buffer <<= 5;
    pulse_buffer |= ((color_value & 0x80) != 0 ? WS2812A_PULSE_ONE : WS2812A_PULSE_ZERO);
    color_value <<= 1;
  }

  **ppBuffer = (pulse_buffer >> 32) & 0xFF;
  (*ppBuffer)++;
  **ppBuffer = (pulse_buffer >> 24) & 0xFF;
  (*ppBuffer)++;
  **ppBuffer = (pulse_buffer >> 16) & 0xFF;
  (*ppBuffer)++;
  **ppBuffer = (pulse_buffer >> 8) & 0xFF;
  (*ppBuffer)++;
  **ppBuffer = pulse_buffer & 0xFF;
  (*ppBuffer)++;
}

void WS2812A_handler(void)
{
  bool transmit_request = false;

  /* check if global color must be set */
  if(light_params.set_color_XY | light_params.set_color_HS)
  {
    /* set a global color */
    RGB_t color_rgb;

    if(light_params.set_color_XY)
    {
      /* set a global color from XY space */
      color_rgb = convert_xy_to_RGB(light_params.color_xy);
      /* mark as done */
      light_params.set_color_XY = false;          
    }
    else if(light_params.set_color_HS)
    {
      /* set a global color from HS space */
      //TODO implement HS to RGB
      /* mark as done */
      light_params.set_color_HS = false;        
    }

    uint16_t dev_index;
    for(dev_index = 0; dev_index < WS2812A_NUMB_DEV; dev_index++)
    {
      WS2812A_RGB_data[dev_index] = color_rgb;
    }

    //XXX test of HS to RGB conversion
    HS_t test_color;
    test_color.hue = 0;
    test_color.sat = 0xFF;
    WS2812A_RGB_data[0] = convert_HS_to_RGB(test_color);
    test_color.hue = 85;
    test_color.sat = 0xFF;
    WS2812A_RGB_data[1] = convert_HS_to_RGB(test_color);
    test_color.hue = 212;
    test_color.sat = 0xFF;
    WS2812A_RGB_data[2] = convert_HS_to_RGB(test_color);
    test_color.hue = 212;
    test_color.sat = 0x00;
    WS2812A_RGB_data[3] = convert_HS_to_RGB(test_color);        

    transmit_request = true;
  }

  /* check if the current level must be changed */
  if((uint8_t)level_current != light_params.level_target)
  {
    uint8_t level_stored = (uint8_t)level_current;
    uint32_t numb_of_steps = light_params.transition_time / WS2812A_TASK_PERIOD;
    if(numb_of_steps > 0)
    {
      /* at least one transitional step */
      float level_change = (light_params.level_target - (float)level_current) / (numb_of_steps + 1.0f);
      level_current += level_change;
      light_params.transition_time -= WS2812A_TASK_PERIOD;
    }
    else
    {
      /* the final level changing step */
      level_current = light_params.level_target;
      light_params.transition_time = 0;
    }

    if((uint8_t)level_current != level_stored)
    {
      /* transmit only if current level has been effectively changed */
      transmit_request = true;
    }
  }

  /* transmit pulses to WS2812A devices */
  if(transmit_request)
  {
    /* generate WS2812A pulses and place them in the pulse buffer */
    size_t dev_index;
    uint8_t* pBuffer = WS2812A_pulse_buffer;
    
    /* calculate the corrected level */
    uint8_t level_corrected = (uint8_t)(level_current * (level_current + 64.0f) / 320.0f);
    /* the corrected level must not be 0 when the level_current != 0 */
    if((level_current >= 1.0f) && (level_corrected < 0xFF))
    {
      ++level_corrected;
    }

    for(dev_index = 0; dev_index < WS2812A_NUMB_DEV; dev_index++)
    {
      /* generate pulses for color green */
      bits_to_pulses((WS2812A_RGB_data[dev_index].G * level_corrected / 0xFF) & 0xFF, &pBuffer);
      
      /* generate pulses for color red */
      bits_to_pulses((WS2812A_RGB_data[dev_index].R * level_corrected / 0xFF) & 0xFF, &pBuffer);

      /* generate pulses for color blue */
      bits_to_pulses((WS2812A_RGB_data[dev_index].B * level_corrected / 0xFF) & 0xFF, &pBuffer);
    }

    /* transmit data to all WS2812A devices */
    HAL_SPI_Transmit_DMA(pWS2812A_SPI, WS2812A_pulse_buffer, WS2812A_PULSE_BUF_SIZE);
  }
}