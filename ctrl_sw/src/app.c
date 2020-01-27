#include "adc.h"
#include "debug.h"
#include "hb_tracker.h"
#include "main.h"
#include "trx.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;

extern uint32_t rx_cnt;

#warning "Add WDT"

hbt_node_t *hbt_head;

void init(void)
{
    debug_init();
    adc_init();

    hbt_head = hb_tracker_init(RFM_NET_ID_HEAD, 700 /* 2x HB + 100 ms*/);

    trx_init(RFM_NET_ID_CTRL);
}

void loop(void)
{
    static uint32_t prev_tick = 0;

    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
        LED0_GPIO_Port->ODR ^= LED0_Pin;
        LED1_GPIO_Port->ODR ^= LED1_Pin;
        LED2_GPIO_Port->ODR ^= LED2_Pin;
        LED3_GPIO_Port->ODR ^= LED3_Pin;
        LED4_GPIO_Port->ODR ^= LED4_Pin;
        LED5_GPIO_Port->ODR ^= LED5_Pin;
        LED6_GPIO_Port->ODR ^= LED6_Pin;



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

    trx_poll_rx();
    trx_poll_tx_hb(300, 1, RFM_NET_ID_HEAD);

        static uint32_t prev_ticke = 0;

    if(prev_ticke < HAL_GetTick())
    {
        prev_ticke = HAL_GetTick() + 100;

            if(hb_tracker_is_timeout(hbt_head))
        {LED7_GPIO_Port->ODR |= LED7_Pin;}
        else{ LED7_GPIO_Port->ODR ^= LED7_Pin;}
    }

    static uint32_t ctrl_hb = 0;
    if(ctrl_hb < HAL_GetTick())
    {
        ctrl_hb = HAL_GetTick() + 1500;

        uint8_t r = 200, g = 0, b = 0;
        uint8_t data[] = {RFM_NET_CMD_LIGHT, r, g, b};
        bool sts = trx_send_ack(RFM_NET_ID_HEAD, data, sizeof(data));
        static uint32_t fail_cnt = 0;
        if(sts) fail_cnt++;
        debug("Send Light %d fc %d\n", sts ? 1 : 0, fail_cnt);
    }
}

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
    // debug("RX: %d > Size: %d | %d %d %d %d\n", sender_node_id, data_len, *(data-4), *(data-3),*(data-2),*(data-1));
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

        case RFM_NET_CMD_HB:
        {            
            bool not_found = hb_tracker_update(sender_node_id);
            if(not_found) debug("HB unknown %d\n", sender_node_id);
        }
            break;

        default:
            debug("Unknown cmd %d\n", data[0]);
            break;
        }
    }
}