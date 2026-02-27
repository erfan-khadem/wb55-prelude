#include "usb_vcp.h"
#include "usbd_cdc_if.h"
#include "cmsis_os2.h"
#include <stdarg.h>
#include <stdio.h>

volatile uint8_t g_usb_cdc_tx_ready = 1;

static void usb_vcp_delay_ms(uint32_t delay_ms)
{
    if (osKernelGetState() == osKernelRunning) {
        osDelay(delay_ms);
    } else {
        HAL_Delay(delay_ms);
    }
}

void USBVCP_InitFlag(void)
{
    g_usb_cdc_tx_ready = 1;
}

void USBVCP_Printf(const char *fmt, ...)
{
    static uint8_t buf[256];

    // Wait until previous transfer completed
    while (!g_usb_cdc_tx_ready) {
        usb_vcp_delay_ms(1);
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf((char*)buf, sizeof(buf), fmt, args);
    va_end(args);

    if (n <= 0) return;
    if (n > (int)sizeof(buf)) n = sizeof(buf);

    uint8_t ret;
    do {
        ret = CDC_Transmit_FS(buf, (uint16_t)n);
        if (ret == USBD_OK) {
            g_usb_cdc_tx_ready = 0; // will be set to 1 in CDC_TransmitCplt_FS
        } else {
            usb_vcp_delay_ms(1);
        }
    } while (ret != USBD_OK);
}
