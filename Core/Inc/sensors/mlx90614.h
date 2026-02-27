#pragma once
#include "main.h"
#include "stm32wbxx_hal_i2c.h"
#include <stdint.h>

#define MLX90614_I2C_ADDR_7BIT   (0x5Au)
#define MLX90614_I2C_ADDR        (MLX90614_I2C_ADDR_7BIT << 1)

typedef struct {
    int32_t ta_c_x100;   // ambient deg C * 100
    int32_t to_c_x100;   // object deg C * 100
} mlx90614_sample_t;

HAL_StatusTypeDef MLX90614_ReadTA_TO1(I2C_HandleTypeDef *hi2c, mlx90614_sample_t *out);
