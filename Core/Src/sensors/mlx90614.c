#include "mlx90614.h"

// SMBus PEC CRC-8 (poly 0x07, init 0x00)
static uint8_t smbus_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static HAL_StatusTypeDef mlx_read_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *raw_out)
{
    uint8_t rx[3] = {0};

    // Generates: START + (addrW) + reg + REPEATED_START + (addrR) + rx[0..2] + STOP
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
        hi2c,
        MLX90614_I2C_ADDR,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        rx,
        sizeof(rx),
        200
    );
    if (st != HAL_OK) return st;

    // PEC check (SMBus CRC-8 poly 0x07 init 0x00)
    uint8_t pec_data[5] = {
        (uint8_t)(MLX90614_I2C_ADDR_7BIT << 1),           // addr + W
        reg,                                              // command
        (uint8_t)((MLX90614_I2C_ADDR_7BIT << 1) | 0x01),  // addr + R
        rx[0], rx[1]                                      // data low, data high
    };
    uint8_t pec = smbus_crc8(pec_data, sizeof(pec_data));
    if (pec != rx[2]) return HAL_ERROR;

    uint16_t raw = (uint16_t)((rx[1] << 8) | rx[0]);
    if (raw & 0x8000u) return HAL_ERROR; // error flag per datasheet :contentReference[oaicite:3]{index=3}

    *raw_out = raw;
    return HAL_OK;
}

HAL_StatusTypeDef MLX90614_ReadTA_TO1(I2C_HandleTypeDef *hi2c, mlx90614_sample_t *out)
{
    // TA (ambient) in RAM 0x06, TO1 (object) in RAM 0x07
    uint16_t raw_ta = 0, raw_to = 0;

    HAL_StatusTypeDef st = mlx_read_reg(hi2c, 0x06, &raw_ta);
    if (st != HAL_OK) return st;

    st = mlx_read_reg(hi2c, 0x07, &raw_to);
    if (st != HAL_OK) return st;

    // Conversion: Kelvin = raw * 0.02; Celsius = K - 273.15
    // In x100: C*100 = raw*2 - 27315
    out->ta_c_x100 = (int32_t)raw_ta * 2 - 27315;
    out->to_c_x100 = (int32_t)raw_to * 2 - 27315;

    return HAL_OK;
}
