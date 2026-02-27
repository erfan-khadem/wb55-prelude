#include "scd41.h"
#include "sensirion_crc.h"
#include "usb_vcp.h"

static HAL_StatusTypeDef scd41_send_cmd(I2C_HandleTypeDef *hi2c, uint16_t cmd)
{
    uint8_t tx[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return HAL_I2C_Master_Transmit(hi2c, SCD41_I2C_ADDR, tx, sizeof(tx), 100);
}

static HAL_StatusTypeDef scd41_write_data(I2C_HandleTypeDef *hi2c, uint16_t cmd, uint16_t value)
{
    uint8_t val[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    uint8_t tx[5] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF),
                    val[0], val[1],
                    (uint8_t)(sensirion_crc8(val, 2))};
    return HAL_I2C_Master_Transmit(hi2c, SCD41_I2C_ADDR, tx, sizeof(tx), 100);
}

HAL_StatusTypeDef SCD41_StartPeriodic(I2C_HandleTypeDef *hi2c)
{
    // start_periodic_measurement: 0x21B1, update interval ~5s
    return scd41_send_cmd(hi2c, 0x21B1);
}

HAL_StatusTypeDef SCD41_StopPeriodic(I2C_HandleTypeDef *hi2c)
{
    // stop_periodic_measurement: 0x3F86, then wait 500ms before other commands
    HAL_StatusTypeDef st = scd41_send_cmd(hi2c, 0x3F86);
    HAL_Delay(500);
    return st;
}

HAL_StatusTypeDef SCD41_DataReady(I2C_HandleTypeDef *hi2c, uint8_t *ready)
{
    // get_data_ready_status: 0xE4B8, returns 1 word + CRC
    *ready = 0;

    HAL_StatusTypeDef st = scd41_send_cmd(hi2c, 0xE4B8);
    if (st != HAL_OK) return st;

    HAL_Delay(1);

    uint8_t rx[3] = {0};
    st = HAL_I2C_Master_Receive(hi2c, SCD41_I2C_ADDR, rx, sizeof(rx), 100);
    if (st != HAL_OK) return st;

    if (sensirion_crc8(rx, 2) != rx[2]) return HAL_ERROR;

    uint16_t word0 = (uint16_t)((rx[0] << 8) | rx[1]);
    // data ready if least significant 11 bits != 0
    *ready = ((word0 & 0x07FFu) != 0u) ? 1u : 0u;
    return HAL_OK;
}

HAL_StatusTypeDef SCD41_ReadMeasurement(I2C_HandleTypeDef *hi2c, scd41_sample_t *out)
{
    // read_measurement: 0xEC05, returns 3 words (CO2, T, RH) each with CRC
    HAL_StatusTypeDef st = scd41_send_cmd(hi2c, 0xEC05);
    if (st != HAL_OK) return st;

    HAL_Delay(1);

    uint8_t rx[9] = {0};
    st = HAL_I2C_Master_Receive(hi2c, SCD41_I2C_ADDR, rx, sizeof(rx), 100);
    if (st != HAL_OK) return st;

    // CRC check each word
    for (int i = 0; i < 3; i++) {
        uint8_t *w = &rx[i * 3];
        if (sensirion_crc8(w, 2) != w[2]) return HAL_ERROR;
    }

    uint16_t co2  = (uint16_t)((rx[0] << 8) | rx[1]);
    uint16_t traw = (uint16_t)((rx[3] << 8) | rx[4]);
    uint16_t hraw = (uint16_t)((rx[6] << 8) | rx[7]);

    // T(deg C) = -45 + 175 * raw / (2^16 - 1)
    // RH(%) = 100 * raw / (2^16 - 1)
    const int32_t denom = 65535;

    out->co2_ppm = co2;
    out->temp_c_x100 = -4500 + (int32_t)((17500LL * traw) / denom);
    out->rh_x100     = (int32_t)((10000LL * hraw) / denom);

    return HAL_OK;
}

HAL_StatusTypeDef SCD41_PerformSelfTest(I2C_HandleTypeDef *hi2c) {
    HAL_StatusTypeDef st = scd41_send_cmd(hi2c, 0x3639);
    if (st != HAL_OK) return st;

    HAL_Delay(10000);
    uint8_t rx[3] = {0};
    st = HAL_I2C_Master_Receive(hi2c, SCD41_I2C_ADDR, rx, sizeof(rx), 100);
    if (st != HAL_OK) return st;
    // CRC check each word
    for (int i = 0; i < 1; i++) {
        uint8_t *w = &rx[i * 3];
        if (sensirion_crc8(w, 2) != w[2]) return HAL_ERROR;
    }

    uint16_t status = (uint16_t)((rx[0] << 8) | rx[1]);

    if(status != 0) {
        //USBVCP_Printf("Got Status Code %hu", status);
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef SCD41_SetAltitudePressure(I2C_HandleTypeDef *hi2c, uint16_t altitude_m, uint16_t pressure_hPa){
    // 0x2427 is set_sensor_altitude
    HAL_StatusTypeDef st = scd41_write_data(hi2c, 0x2427, altitude_m);
    HAL_Delay(1);
    if(st != HAL_OK) return st;

    // 0xe000 is set_ambient_pressure
    st = scd41_write_data(hi2c, 0x2427, pressure_hPa);
    HAL_Delay(1);
    if(st != HAL_OK) return st;

    return HAL_OK;
}