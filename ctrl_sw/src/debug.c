#include "debug.h"
#include "main.h"
#include <serial_suit_protocol.h>
#include "usbd_cdc_if.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern USBD_HandleTypeDef hUsbDeviceFS;

uint32_t rx_cnt = 0;

#define CON_OUT_BUF_SZ 512
#define CON_IN_BUF_SZ 512

char buffer_rx[CON_IN_BUF_SZ];
int buffer_rx_cnt = 0;

void debug_parse(char *s);

static volatile bool is_tx = false;
static bool usb_enabled = true;

void debug_init(void)
{
    __HAL_UART_ENABLE(&huart1);
}

void debug_disable_usb(void) { usb_enabled = false; }

void debug(char *format, ...)
{
    if(is_tx) return;
    is_tx = true;

    static char buffer[CON_OUT_BUF_SZ + 1];
    va_list ap;

    if(hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && usb_enabled)
    {
        va_start(ap, format);
        vsnprintf(buffer + 1, CON_OUT_BUF_SZ - 1, format, ap);
        va_end(ap);
        buffer[0] = SSP_CMD_DEBUG;
        CDC_Transmit_FS(buffer, strlen(buffer));
    }
    else
    {
        va_start(ap, format);
        vsnprintf(buffer, CON_OUT_BUF_SZ, format, ap);
        va_end(ap);
        HAL_UART_Transmit(&huart1, buffer, strlen(buffer), 200);
    }

    is_tx = false;
}

void debug_rx(char x)
{
    if(x == '\n')
    {
        buffer_rx[buffer_rx_cnt] = x;
        buffer_rx[buffer_rx_cnt + 1] = '\0';
        debug_parse(buffer_rx);
        buffer_rx_cnt = 0;
    }
    else
    {
        buffer_rx[buffer_rx_cnt] = x;
        buffer_rx_cnt++;
    }
}