#include "adc.h"
#include "debug.h"
#include "flasher_hal.h"
#include "hb_tracker.h"
#include "main.h"
#include "trx.h"

#include <string.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;

extern uint32_t rx_cnt;

#warning "Add WDT"

hbt_node_t *hbt_head;

bool to_from_head = true;

uint32_t get_random(void){return HAL_RNG_GetRandomNumber(&hrng);}

const uint8_t iterator_head = 0;
const uint8_t iterator_tail = 1;

void init(void)
{
    debug_init();
    adc_init();

    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    hbt_head = hb_tracker_init(RFM_NET_ID_HEAD, 700 /* 2x HB + 100 ms*/);

    trx_init();
    trx_init_node(iterator_head, RFM_NET_ID_HEAD);
    trx_init_node(iterator_tail, RFM_NET_ID_TAIL);
}

void loop(void)
{
    static uint32_t prev_tick = 0;

    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
        LED0_GPIO_Port->ODR ^= LED0_Pin;
        // LED1_GPIO_Port->ODR ^= LED1_Pin;
        // LED2_GPIO_Port->ODR ^= LED2_Pin;
        // LED3_GPIO_Port->ODR ^= LED3_Pin;
        // LED4_GPIO_Port->ODR ^= LED4_Pin;
        // LED5_GPIO_Port->ODR ^= LED5_Pin;

        static uint32_t cnt = 0;
        // debug(">>> %d\n", cnt++);
        if(0)
        {
            static uint8_t sendSize = 0;
            static bool request_ack = false;

            debug("Sending[%d]\n", sendSize + 1);
            // for(uint32_t i = 0; i < sendSize + 1U; i++)
            //     debug(" > %c\n", (char)payload[i]);

            request_ack = !(sendSize % 3); //request ACK every 3rd xmission
        }
    }

    trx_poll();


    static uint32_t prev_ticke = 0;

    if(prev_ticke < HAL_GetTick())
    {
        prev_ticke = HAL_GetTick() + 100;

        if(hb_tracker_is_timeout(hbt_head))
        {
            LED7_GPIO_Port->ODR |= LED7_Pin;
        }
        else
        {
            LED7_GPIO_Port->ODR ^= LED7_Pin;
        }

        if(to_from_head)
        {
            LED6_GPIO_Port->ODR |= LED6_Pin;
        }
        else
        {
            LED6_GPIO_Port->ODR ^= LED6_Pin;
        }
    }

    static uint32_t ctrl_hb = 0;
    if(ctrl_hb < HAL_GetTick())
    {
        ctrl_hb = HAL_GetTick() + 200;

        uint8_t r = 0, g = 200, b = 0;
        uint8_t data[4] = {RFM_NET_CMD_LIGHT, 0, 0, 0};
        static uint8_t ptr = 1;
        ptr++;
        if(ptr >= 4) ptr = 1;
        data[ptr] = 200;
        trx_send_async(iterator_head, data, sizeof(data));

        // static uint32_t fail_cnt = 0;
        // if(sts) fail_cnt++;
        // if(sts)
        //     debug("Light %d fail %d\n", sts ? 1 : 0, fail_cnt);
    }

    static uint32_t prev = 0;
    static uint32_t cnt = 0;
    if(prev < HAL_GetTick())
    {
        prev = HAL_GetTick() + 1000;
        debug("STAT: %d\n", cnt);
        cnt = 0;
    }
    cnt++;
}

static void memcpy_volatile(void *src, const volatile void *dst, size_t size)
{
    for(uint32_t i = 0; i < size; i++)
    {
        *((uint8_t *)src + i) = *((const volatile uint8_t *)dst + i);
    }
}

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
    // debug("RX: %d > Size: %d | %d %d %d %d\n", sender_node_id, data_len, *(data-4), *(data-3),*(data-2),*(data-1));
    debug("R %d > %d\n", sender_node_id, data[0]);
    if(data_len > 0)
    {
        switch(data[0])
        {
        case RFM_NET_CMD_DEBUG:
            debug("# ");
            for(uint8_t i = 1; i < data_len; i++)
                debug("%c", data[i]);
            if(data[data_len - 1] != '\n') debug("\n");
            break;

        // case RFM_NET_CMD_HB:
        // {
        //     if(data_len == 2)
        //     {
        //         bool not_found = hb_tracker_update(sender_node_id);
        //         if(not_found) debug("HB unknown %d\n", sender_node_id);
        //         if(sender_node_id == RFM_NET_ID_HEAD)
        //         {
        //             to_from_head = data[1];
        //         }
        //     }
        // }
        // break;

        case RFM_NET_CMD_STS_HEAD:
            if(data_len == 1 + 4 + 4)
            {
                float vbat, temp;
                memcpy_volatile(&vbat, (data + 1), 4);
                memcpy_volatile(&temp, (data + 1 + 4), 4);
                debug("HD STS: v %.2f | t %.1f\n", vbat, temp);
            }
            else
                debug("Wrong size RFM_NET_CMD_STS_HEAD %d", data_len);
            break;

        default:
            debug("Unknown cmd %d\n", data[0]);
            break;
        }
    }
}