#include "debug.h"
#include "main.h"
#include "rfm_net.h"
#include "serial_suit_protocol.h"
#include "usbd_cdc_if.h"
#include "flasher_hal.h"
#include "flash_regions.h"

#include <stdbool.h>
#include <stdint.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t tx_buf[1024];

void tx(uint8_t *buf, uint32_t len)
{
    CDC_Transmit_FS(buf, len);
}


void usb_if_rx(uint8_t *buf, uint32_t len)
{
    if(len)
    {
        switch(buf[0])
        {
        case SSP_CMD_DEBUG:
            if(len >= 3) // at least 1 symbol
            {
                switch(buf[1])
                {
                case RFM_NET_ID_CTRL:
                    for(uint32_t i = 2; i < len; i++)
                        debug_rx(buf[i]);
                    break;

                case RFM_NET_ID_HEAD:
                    // TODO
                    break;

                case RFM_NET_ID_TAIL:
                    // TODO
                    break;

                default: break;
                }
            }
            break;

        case SSP_CMD_FLASH:
            debug_disable_usb();
            // Format: [1b] SSP_CMD_FLASH [1b] node_id [4b] address [xx] data
            if(len >= 7) // at least 1 symbol
            {
                if(((len - 6) % 4) == 0)
                {
                    tx_buf[0] = SSP_CMD_FLASH;
                    tx_buf[1] = buf[1];
                    uint32_t addr;
                    memcpy(&addr, buf+2, 4);
                    uint32_t len_flash = len - 6;
                    switch(buf[1])
                    {
                    case RFM_NET_ID_CTRL:
                    {
                        if(addr < ADDR_APP_IMAGE || addr >= ADDR_APP_IMAGE + LEN_APP_IMAGE)
                        {
                            tx_buf[2] = SSP_FLASH_WRONG_ADDRESS;
                        }
                        else
                        {
                            tx_buf[2] = SSP_FLASH_OK;
                            for(uint32_t i = 0; i < len_flash; i++)
                            {
                                if(FLASH_ProgramByte(addr + i, buf[6+i]) != FLASH_COMPLETE)
                                {
                                    tx_buf[2] = SSP_FLASH_FAIL;
                                    break;
                                }
                                if(*(uint8_t*)(addr+i) != buf[6+i])
                                {
                                    tx_buf[2] = SSP_FLASH_VERIFY_FAIL;
                                    break;
                                }
                            }
                            memcpy(tx_buf + 3, &addr, 4);
                            tx(tx_buf, 3+4);
                            break;
                        }
                        
                        tx(tx_buf, 3);
                    }
                    break;

                    case RFM_NET_ID_HEAD:
                    {
                        tx_buf[2] = SSP_FLASH_UNEXIST;
                        tx(tx_buf, 3);
                    }
                    break;

                    case RFM_NET_ID_TAIL:
                    {
                        tx_buf[2] = SSP_FLASH_UNEXIST;
                        tx(tx_buf, 3);
                    }
                    break;

                    default:                   
                    {
                        tx_buf[2] = SSP_FLASH_UNEXIST;
                        tx(tx_buf, 3);
                    }
                    break;
                    }
                }
                else
                {
                    tx_buf[2] = SSP_FLASH_WRONG_PARAM;
                    tx_buf[3] = len;
                    tx(tx_buf, 4);
                }
            }
            break;

            case SSP_CMD_REBOOT:
            if(len >= 2) 
            {
                switch(buf[1])
                {
                case RFM_NET_ID_CTRL:
                {
                    // PCD_HandleTypeDef *hpcd = hUsbDeviceFS.pData;
                    // USB_DevDisconnect(hpcd->Instance);
                    HAL_NVIC_SystemReset();
                }
                    break;

                case RFM_NET_ID_HEAD:
                    // TODO
                    break;

                case RFM_NET_ID_TAIL:
                    // TODO
                    break;

                default: break;
                }
            }
            break;

            default:
                break;
        }
    }
}