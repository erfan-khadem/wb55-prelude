#pragma once
#include "main.h"
#include <stdint.h>

extern volatile uint8_t g_usb_cdc_tx_ready;

void USBVCP_InitFlag(void);
void USBVCP_Printf(const char *fmt, ...);