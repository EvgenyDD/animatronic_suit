#include "adc.h"
#include "debug.h"
#include "main.h"
#include "rfm12b.h"
#include "rfm_net.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;

extern UART_HandleTypeDef huart1;

extern uint32_t rx_cnt;

#warning "Add WDT"

uint8_t KEY[] = "ABCDABCDABCDABCD";
char payload[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890~!@#$%^&*(){}[]`|<>?+=:;,.";


void init(void)
{
    __HAL_UART_ENABLE(&huart1);

    adc_init();

    rfm12b_init(RFM_NET_GATEWAY, RFM_NET_ID_CTRL);
    rfm12b_encrypt(KEY, 16);
    rfm12b_sleep();
}

uint8_t buff[200];

// wait a few milliseconds for proper ACK, return true if received
static bool waitForAck(uint8_t dest_node_id)
{
    uint32_t now = HAL_GetTick();
    while(HAL_GetTick() - now <= 50 /* ms */)
        if(rfm12b_is_ack_received(dest_node_id))
            return true;
    return false;
}

void loop(void)
{
    static uint32_t prev_tick = 0;

    static uint8_t prev_sts = 0xff;
    uint8_t stsx = 0; //rfm12b_trx_1b(0);
    if(stsx != prev_sts)
    {
        prev_sts = stsx;
        // debug("STS Change: %d @%d\n", prev_sts, HAL_GetTick());
    }

    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
        LED0_GPIO_Port->ODR ^= LED0_Pin;
        LED1_GPIO_Port->ODR ^= LED1_Pin;
        //     LED2_GPIO_Port->ODR ^= LED2_Pin;
        //     LED3_GPIO_Port->ODR ^= LED3_Pin;
        //     LED4_GPIO_Port->ODR ^= LED4_Pin;
        //     LED5_GPIO_Port->ODR ^= LED5_Pin;
        //     LED6_GPIO_Port->ODR ^= LED6_Pin;
        //     LED7_GPIO_Port->ODR ^= LED7_Pin;
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

            rfm12b_wakeup();

            uint8_t dest_node_id = RFM_NET_ID_HEAD;

            rfm12b_send(dest_node_id, payload, sendSize + 1, request_ack, true);

            if(request_ack)
            {
                debug(" - waiting for ACK...");
                if(waitForAck(dest_node_id))
                    debug("ok!\n");
                else
                    debug("nothing...\n");
            }

            rfm12b_sleep();

            sendSize = (sendSize + 1) % 88;

            debug("\n");
        }
    }

    if(1)
    {
        if(rfm12b_rx_complete())
        {
            if(rfm12b_is_crc_pass())
            {
                debug("[%d] Size: %d\n", rfm12b_get_sender(), rfm12b_get_data_len());
                // for(uint32_t i = 0; i < rfm12b_get_data_len(); i++) //can also use radio.GetDataLen() if you don't like pointers
                //     debug(" > %c\n", (char)rfm12b_get_data()[i]);

                if(rfm12b_is_ack_requested())
                {
                    rfm12b_sendACK("", 0, false);
                    debug(" - ACK sent\n");
                }
            }
            else
                debug("BAD-CRC\n");

            debug("\n");
        }
    }

    // rx();
}