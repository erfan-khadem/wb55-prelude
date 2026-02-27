#include "usb_vcp.h"
#include "usbd_cdc_if.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

volatile uint8_t g_usb_cdc_tx_ready = 1;
volatile uint8_t g_usb_cdc_terminal_ready = 0;

static uint8_t g_usb_vcp_tx_active[256];
static uint8_t g_usb_vcp_tx_pending[256];
static uint8_t g_usb_vcp_format_buf[256];
static uint16_t g_usb_vcp_tx_pending_len = 0;

static void USBVCP_TryFlushPending(void)
{
    uint8_t ret;

    if ((g_usb_cdc_terminal_ready == 0U) ||
        (g_usb_cdc_tx_ready == 0U) ||
        (g_usb_vcp_tx_pending_len == 0U)) {
        return;
    }

    memcpy(g_usb_vcp_tx_active, g_usb_vcp_tx_pending, g_usb_vcp_tx_pending_len);
    ret = CDC_Transmit_FS(g_usb_vcp_tx_active, g_usb_vcp_tx_pending_len);
    if (ret == USBD_OK) {
        g_usb_cdc_tx_ready = 0U;
        g_usb_vcp_tx_pending_len = 0U;
    }
}

void USBVCP_InitFlag(void)
{
    g_usb_cdc_tx_ready = 1U;
    g_usb_cdc_terminal_ready = 0U;
    g_usb_vcp_tx_pending_len = 0U;
}

void USBVCP_Printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf((char*)g_usb_vcp_format_buf, sizeof(g_usb_vcp_format_buf), fmt, args);
    va_end(args);

    if (n <= 0) {
        return;
    }

    if (n >= (int)sizeof(g_usb_vcp_format_buf)) {
        n = (int)sizeof(g_usb_vcp_format_buf) - 1;
    }

    memcpy(g_usb_vcp_tx_pending, g_usb_vcp_format_buf, (uint16_t)n);
    g_usb_vcp_tx_pending_len = (uint16_t)n;

    USBVCP_TryFlushPending();
}

void USBVCP_SetTerminalReady(uint8_t ready)
{
    g_usb_cdc_terminal_ready = (ready != 0U) ? 1U : 0U;
}

void USBVCP_OnTxComplete(void)
{
    g_usb_cdc_tx_ready = 1U;
    USBVCP_TryFlushPending();
}

uint8_t USBVCP_IsTerminalReady(void)
{
    return g_usb_cdc_terminal_ready;
}
