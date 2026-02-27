#include "app_sensors.h"
#include "scd41.h"
#include "sht40.h"
#include "mlx90614.h"
#include "usb_vcp.h"
#include "stm32wbxx_hal_i2c.h"
#include <string.h>

static uint32_t t_last_fast = 0;
static uint32_t t_last_scd_check = 0;

static void print_x100(const char *label, int32_t v_x100)
{
    int32_t ip = v_x100 / 100;
    int32_t fp = v_x100 % 100;
    if (fp < 0) fp = -fp;
    USBVCP_Printf("%s%ld.%02ld", label, (long)ip, (long)fp);
}

void Sensors_Init(void)
{
    USBVCP_InitFlag();

    // Start SCD41 periodic measurement (CO2 + T/RH)
    // If the sensor was already running, stopping first is safe.
    (void)SCD41_StopPeriodic(&hi2c1);
    if(SCD41_PerformSelfTest(&hi2c1) != HAL_OK) {
        for(int i = 0; i < 10; i++) {
            HAL_GPIO_TogglePin(GP1_GPIO_Port, GP1_Pin);
            HAL_Delay(500);
        }
    } else {
        HAL_GPIO_TogglePin(GP0_GPIO_Port, GP0_Pin);
    }
    (void)SCD41_SetAltitudePressure(&hi2c1, 1250, 870); // for my place at Tehran
    (void)SCD41_StartPeriodic(&hi2c1);

    USBVCP_Printf("Sensors init done.\r\n");
}

void Sensors_Task(void)
{
    const uint32_t now = HAL_GetTick();

    // Read SHT40 + MLX90614 every 1000 ms
    if ((now - t_last_fast) >= 1000) {
        t_last_fast = now;

        sht40_sample_t sht = {0};
        mlx90614_sample_t mlx = {0};

        HAL_StatusTypeDef st_sht = SHT40_ReadHighPrecision(&hi2c1, &sht);
        HAL_StatusTypeDef st_mlx = MLX90614_ReadTA_TO1(&hi2c1, &mlx);

        USBVCP_Printf("[FAST] ");

        if (st_sht == HAL_OK) {
            print_x100("SHT_T=", sht.temp_c_x100);
            USBVCP_Printf("C ");
            print_x100("SHT_RH=", sht.rh_x100);
            USBVCP_Printf("%% ");
        } else {
            USBVCP_Printf("SHT_ERR ");
        }

        if (st_mlx == HAL_OK) {
            print_x100("MLX_Ta=", mlx.ta_c_x100);
            USBVCP_Printf("C ");
            print_x100("MLX_To=", mlx.to_c_x100);
            USBVCP_Printf("C ");
        } else {
            USBVCP_Printf("MLX_ERR ");
        }

        USBVCP_Printf("\r\n");
    }

    // Check SCD41 data-ready ~ every 500 ms; read when ready (~5 s update)
    if ((now - t_last_scd_check) >= 500) {
        t_last_scd_check = now;

        uint8_t ready = 0;
        if (SCD41_DataReady(&hi2c1, &ready) == HAL_OK && ready) {
            scd41_sample_t scd = {0};
            if (SCD41_ReadMeasurement(&hi2c1, &scd) == HAL_OK) {
                USBVCP_Printf("[SCD] CO2=%u ppm ", scd.co2_ppm);
                print_x100("T=", scd.temp_c_x100);
                USBVCP_Printf("C ");
                print_x100("RH=", scd.rh_x100);
                USBVCP_Printf("%%\r\n");
            } else {
                USBVCP_Printf("[SCD] READ_ERR\r\n");
            }
        }
    }
}