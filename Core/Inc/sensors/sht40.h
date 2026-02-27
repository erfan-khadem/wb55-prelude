#pragma once
#include "main.h"
#include "stm32wbxx_hal_i2c.h"
#include <stdint.h>

#define SHT40_I2C_ADDR_7BIT   (0x44u)
#define SHT40_I2C_ADDR        (SHT40_I2C_ADDR_7BIT << 1)

typedef struct {
    int32_t temp_c_x100;  // deg C * 100
    int32_t rh_x100;      // %RH * 100
} sht40_sample_t;

HAL_StatusTypeDef SHT40_ReadHighPrecision(I2C_HandleTypeDef *hi2c, sht40_sample_t *out);
