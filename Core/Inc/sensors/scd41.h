#pragma once
#include "main.h"
#include "stm32wbxx_hal_i2c.h"
#include <stdint.h>

#define SCD41_I2C_ADDR_7BIT   (0x62u)
#define SCD41_I2C_ADDR        (SCD41_I2C_ADDR_7BIT << 1)

typedef struct {
    uint16_t co2_ppm;
    int32_t  temp_c_x100;   // deg C * 100
    int32_t  rh_x100;       // %RH * 100
} scd41_sample_t;

HAL_StatusTypeDef SCD41_StartPeriodic(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef SCD41_StopPeriodic(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef SCD41_SetAltitudePressure(I2C_HandleTypeDef *hi2c, uint16_t altitude_m, uint16_t pressure_hPa);
HAL_StatusTypeDef SCD41_DataReady(I2C_HandleTypeDef *hi2c, uint8_t *ready);
HAL_StatusTypeDef SCD41_PerformSelfTest(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef SCD41_ReadMeasurement(I2C_HandleTypeDef *hi2c, scd41_sample_t *out);
