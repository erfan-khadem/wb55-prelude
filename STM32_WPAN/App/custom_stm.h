/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    App/custom_stm.h
  * @author  MCD Application Team
  * @brief   Header for custom_stm.c module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef CUSTOM_STM_H
#define CUSTOM_STM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ble.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  CUSTOM_STM_LEDWRITE,
  CUSTOM_STM_SHT40_DATA,
  CUSTOM_STM_SCD41_DATA,
  CUSTOM_STM_MLX90614_DATA,
} Custom_STM_Char_Opcode_t;

/* Exported constants --------------------------------------------------------*/
extern uint16_t SizeLedwrite;
extern uint16_t SizeSht40Data;
extern uint16_t SizeScd41Data;
extern uint16_t SizeMlx90614Data;

/* Exported functions ------------------------------------------------------- */
void SVCCTL_InitCustomSvc(void);
tBleStatus Custom_STM_App_Update_Char(Custom_STM_Char_Opcode_t CharOpcode, uint8_t *pPayload);
tBleStatus Custom_STM_UpdateSHT40(int32_t temp_c_x100, int32_t rh_x100);
tBleStatus Custom_STM_UpdateSCD41(uint16_t co2_ppm, int32_t temp_c_x100, int32_t rh_x100);
tBleStatus Custom_STM_UpdateMLX90614(int32_t ta_c_x100, int32_t to_c_x100);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_STM_H */
