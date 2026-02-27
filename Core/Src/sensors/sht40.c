#include "sht40.h"
#include "sensirion_crc.h"
#include "cmsis_os2.h"

static void sht40_delay_ms(uint32_t delay_ms)
{
    if (osKernelGetState() == osKernelRunning) {
        osDelay(delay_ms);
    } else {
        HAL_Delay(delay_ms);
    }
}

HAL_StatusTypeDef SHT40_ReadHighPrecision(I2C_HandleTypeDef *hi2c, sht40_sample_t *out)
{
    // measure T&RH high precision command = 0xFD
    uint8_t cmd = 0xFD;
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(hi2c, SHT40_I2C_ADDR, &cmd, 1, 100);
    if (st != HAL_OK) return st;

    // typical wait ~10ms from datasheet pseudo code
    sht40_delay_ms(10);

    uint8_t rx[6] = {0};
    st = HAL_I2C_Master_Receive(hi2c, SHT40_I2C_ADDR, rx, sizeof(rx), 100);
    if (st != HAL_OK) return st;

    // CRC for T word and RH word (poly 0x31 init 0xFF)
    if (sensirion_crc8(&rx[0], 2) != rx[2]) return HAL_ERROR;
    if (sensirion_crc8(&rx[3], 2) != rx[5]) return HAL_ERROR;

    uint16_t traw = (uint16_t)((rx[0] << 8) | rx[1]);
    uint16_t hraw = (uint16_t)((rx[3] << 8) | rx[4]);

    // t = -45 + 175 * raw / 65535
    // rh = -6 + 125 * raw / 65535
    const int32_t denom = 65535;

    int32_t t_x100  = -4500 + (int32_t)((17500LL * traw) / denom);
    int32_t rh_x100 =  -600 + (int32_t)((12500LL * hraw) / denom);

    // clamp RH to 0..100%
    if (rh_x100 < 0) rh_x100 = 0;
    if (rh_x100 > 10000) rh_x100 = 10000;

    out->temp_c_x100 = t_x100;
    out->rh_x100 = rh_x100;

    return HAL_OK;
}
