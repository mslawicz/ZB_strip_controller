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
#include <math.h>

#define WS2812A_NUMB_DEV  8     /* number of WS2812A devices in the strip */
#define WS2812A_DEV_SIZE  15    /* number of bytes per WS2812A device (24 RGB bits * 5 pulse bits = 120 bits) */
#define WS2812A_PULSE_BUF_SIZE  (WS2812A_NUMB_DEV * WS2812A_DEV_SIZE)   /* size of WS2812A pulse buffer */
#define WS2812A_PULSE_ZERO  0x10    /* 5-bit SPI pulse creating device bit 0 */
#define WS2812A_PULSE_ONE   0x1C    /* 5-bit SPI pulse creating device bit 1 */
#define WS2812A_RGB_WHITE 0x7F  /* RGB value for the white color */
#define MAX_SAT 0xFF

static SPI_HandleTypeDef* pWS2812A_SPI;
static uint8_t WS2812A_pulse_buffer[WS2812A_PULSE_BUF_SIZE];
static RGB_t WS2812A_RGB_data[WS2812A_NUMB_DEV];
static float level_current = 0.0f;   /* current light level <0.0,255.0> */
static uint16_t group_size[WS2812A_NUMB_DEV]; /* number of devices in groups, number of groups <= number of devices */
static uint16_t number_of_groups = 0;   /* number of groups */
static WS2812A_ColorLoopTypeDef last_color_loop_mode = COLOR_LOOP_NUMB_MODES;   /* used for mode change detection */

Light_Params_t light_params =
{
    .level_target = 0,
    .level_on = WS2812A_START_ON_LEVEL,
    .transition_time = 0,
    .set_color_XY = false,
    .set_color_HS = false,
    .set_color_temp = false,
    .color_restore = false,
    .color_mode = COLOR_STATIC,
    .color_loop_mode = COLOR_LOOP_CYCLIC_ALL_FAST,
    .color_rgb = {WS2812A_RGB_WHITE, WS2812A_RGB_WHITE, WS2812A_RGB_WHITE}
};

void WS2812A_handler(void);
void color_loop_cycling(float period, bool use_groups);
void color_loop_random(float period, bool use_groups);

void WS2812A_Init(SPI_HandleTypeDef* phSPI)
{
  pWS2812A_SPI = phSPI;

  /* initialize RGB buffer with white color values */
  memset(WS2812A_RGB_data, WS2812A_RGB_WHITE, sizeof(WS2812A_RGB_data));

  /* define groups */
  uint16_t group;
  number_of_groups = WS2812A_NUMB_DEV;
  for(group = 0; group < number_of_groups; group++)
  {
    group_size[group] = 1;
  }

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
  //XXX test
  light_params.color_mode = COLOR_LOOP;
  light_params.color_loop_mode = COLOR_LOOP_RANDOM_GROUPS_FAST;
  light_params.level_target = 100;


  /* check if global color must be set */
  if(light_params.set_color_XY | light_params.set_color_HS | light_params.set_color_temp | light_params.color_restore)
  {
    /* set a global color */

    if(light_params.set_color_XY)
    {
      /* set a global color from XY space */
      light_params.color_rgb = convert_XY_to_RGB(light_params.color_xy);
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
    else if(light_params.set_color_temp)
    {
      /* convert color temperature to XY */
      light_params.color_xy = convert_temp_to_XY(light_params.color_temp);
      /* set a global color from XY space */
      light_params.color_rgb = convert_XY_to_RGB(light_params.color_xy);
      /* mark as done */
      light_params.set_color_temp = false;        
    }
    else if(light_params.color_restore)
    {
      light_params.color_restore = false;
      /* do nothing else - just restore static RGB color */
    }

    uint16_t dev_index;
    for(dev_index = 0; dev_index < WS2812A_NUMB_DEV; dev_index++)
    {
      WS2812A_RGB_data[dev_index] = light_params.color_rgb;
    }       

    /* global color sets color mode to static */
    light_params.color_mode = COLOR_STATIC;
    /* apply change by transmission to devices */
    transmit_request = true;
  }

  /* check if color mode loop is active */
  if(light_params.color_mode == COLOR_LOOP)
  {
    switch(light_params.color_loop_mode)
    {
      case COLOR_LOOP_CYCLIC_GROUPS_FAST:
      color_loop_cycling(5.0f, true);
      break;
      
      case COLOR_LOOP_CYCLIC_GROUPS_SLOW:
      color_loop_cycling(30.0f, true);
      break;

      case COLOR_LOOP_CYCLIC_ALL_FAST:
      color_loop_cycling(5.0f, false);
      break;

      case COLOR_LOOP_CYCLIC_ALL_SLOW:
      color_loop_cycling(30.0f, false);
      break;

      case COLOR_LOOP_RANDOM_GROUPS_FAST:
      color_loop_random(1.0f, true);
      break;      

      default:
      break;
    }
    last_color_loop_mode = light_params.color_loop_mode;
    /* apply change by transmission to devices */
    transmit_request = true; 
  }

  /* check if the current level must be changed */
  if((uint8_t)level_current != light_params.level_target)
  {
    uint8_t level_stored = (uint8_t)level_current;
    uint32_t numb_of_steps = light_params.transition_time / WS2812A_TASK_INTERVAL;
    if(numb_of_steps > 0)
    {
      /* at least one transitional step */
      float level_change = (light_params.level_target - (float)level_current) / (numb_of_steps + 1.0f);
      level_current += level_change;
      light_params.transition_time -= WS2812A_TASK_INTERVAL;
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

void color_loop_cycling(float period, bool use_groups)
{
  static float phase0 = 0.0f;  // phase of the first device in the hue circle <0,1>
  uint16_t group;
  uint16_t dev_index = 0;  /* index of the device to be set */
  float phase_delta = 0.001f * WS2812A_TASK_INTERVAL / period;
  HS_t color_hs;
  RGB_t color_rgb;
  float phase;
  float direction = (light_params.loop_direction == 0) ? 1.0f : -1.0f;

  phase0 += phase_delta;
  phase0 = fmodf(phase0 + 1.0f, 1.0f);   // the phase is again in the range <0,1>
  
  /* set all groups */
   for(group = 0; group < number_of_groups; group++)
  {
    uint16_t device;
    phase = phase0;
    if(use_groups)
    {
      phase += direction * (float)group / (float)number_of_groups;
      phase = fmodf(phase + 1.0f, 1.0f);   // the phase is in the range <0,1>
    }
    color_hs.hue = (uint8_t)(phase * 0x100) % 0x100;
    color_hs.sat = MAX_SAT;
    color_rgb = convert_HS_to_RGB(color_hs);
    /* set all devices in a group */
    for(device = 0; device < group_size[group]; device++)
    {
      if(dev_index < WS2812A_NUMB_DEV)
      {
        WS2812A_RGB_data[dev_index++] = color_rgb;
      }
    }
  }
}

void color_loop_random(float period, bool use_groups)
{
  HS_t color_hs;
  static RGB_t color_rgb_current = {WS2812A_RGB_WHITE, WS2812A_RGB_WHITE, WS2812A_RGB_WHITE};
  static RGB_t color_rgb_target = {WS2812A_RGB_WHITE, WS2812A_RGB_WHITE, WS2812A_RGB_WHITE};
  static uint16_t group_active;
  uint16_t group_index;
  static float remaining_time;
  uint16_t device, group;
  static const float Task_interval = 0.001f * WS2812A_TASK_INTERVAL;

  /* initialize groups if it is the first pass after the loop mode has been changed */
  if(light_params.color_loop_mode != last_color_loop_mode)
  {
    group_index = 0;
    RGB_t color_rgb_init;
    for(group = 0; group < number_of_groups; group++)
    {
      if((group == 0) || (use_groups))
      {
        /* set new init color */
        color_hs.hue = rand() % 0x100;  /* random hue */
        color_hs.sat = MAX_SAT;
        color_rgb_init = convert_HS_to_RGB(color_hs);  
      }

      for(device = 0; device < group_size[group]; device++)
      {
        WS2812A_RGB_data[group_index + device] = color_rgb_init;
      }
      group_index += group_size[group];
    }
  }

  /* check if the current color equals the target color */
  if((color_rgb_current.R == color_rgb_target.R) &&
     (color_rgb_current.G == color_rgb_target.G) &&
     (color_rgb_current.B == color_rgb_target.B))
  {
    /* set new active group; 0 if groups not used */
    group_active = use_groups ? rand() % number_of_groups : 0;
    /* set new target color */
    color_hs.hue = rand() % 0x100;  /* random hue */
    color_hs.sat = MAX_SAT;
    color_rgb_target = convert_HS_to_RGB(color_hs);
    /* set current color from acitve group data */
    group_index = 0;
    for(group = 0; group < group_active; group++)
    {
      group_index += group_size[group];
    }
    color_rgb_current = WS2812A_RGB_data[group_index];
    /* reset remaining time */
    remaining_time = period;
  }

  /* single step current color change */
  if(remaining_time <= Task_interval)
  {
    /* transfer time elapsed */
    color_rgb_current = color_rgb_target;
  }
  else
  {
    /* next color changing step */
    color_rgb_current.R += (int8_t)(((float)color_rgb_target.R - (float)color_rgb_current.R) * Task_interval / remaining_time);
    color_rgb_current.G += (int8_t)(((float)color_rgb_target.G - (float)color_rgb_current.G) * Task_interval / remaining_time);
    color_rgb_current.B += (int8_t)(((float)color_rgb_target.B - (float)color_rgb_current.B) * Task_interval / remaining_time);
    remaining_time -= Task_interval;
  }

  group_index = 0;
  for(group = 0; group < number_of_groups; group++)
  {
    if((!use_groups) || (group == group_active))
    {
      for(device = 0; device < group_size[group]; device++)
      {
        WS2812A_RGB_data[group_index + device] = color_rgb_current;
      }
    }
    group_index += group_size[group];
  }
}