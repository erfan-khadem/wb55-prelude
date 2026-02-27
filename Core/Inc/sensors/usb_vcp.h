#pragma once
#include "main.h"
#include <stdint.h>

extern volatile uint8_t g_usb_cdc_tx_ready;
extern volatile uint8_t g_usb_cdc_terminal_ready;

void USBVCP_InitFlag(void);
void USBVCP_Printf(const char *fmt, ...);
void USBVCP_SetTerminalReady(uint8_t ready);
void USBVCP_OnTxComplete(void);
uint8_t USBVCP_IsTerminalReady(void);
