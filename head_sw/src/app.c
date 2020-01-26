#include "adc.h"
#include "debug.h"
#include "main.h"
#include "rfm12b.h"
#include "rfm_net.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim12;

extern UART_HandleTypeDef huart3;

extern CRC_HandleTypeDef hcrc;
extern RNG_HandleTypeDef hrng;

extern uint32_t rx_cnt;

#warning "Add WDT"

uint8_t KEY[] = "ABCDABCDABCDABCD";
char payload[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890~!@#$%^&*(){}[]`|<>?+=:;,.";

// wait a few milliseconds for proper ACK, return true if received
static bool waitForAck(uint8_t dest_node_id)
{
    uint32_t now = HAL_GetTick();
    while(HAL_GetTick() - now <= 50 /* ms */)
        if(rfm12b_is_ack_received(dest_node_id))
            return true;
    return false;
}

void init(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_3);
    HAL_TIM_IC_Start(&htim8, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);

    __HAL_UART_ENABLE(&huart3);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    adc_init();

    PWR_EN_GPIO_Port->ODR |= PWR_EN_Pin;

    rfm12b_init(RFM_NET_GATEWAY, RFM_NET_ID_HEAD);
    rfm12b_encrypt(KEY, 16);
    rfm12b_sleep();
}

void loop(void)
{
    static uint32_t prev_tick = 0;
    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
        LED_GPIO_Port->ODR ^= LED_Pin;

        if(1)
        {
            static uint8_t sendSize = 0;
            static bool request_ack = false;

            debug("Sending[%d]\n", sendSize + 1);
            // for(uint32_t i = 0; i < sendSize + 1U; i++)
            //     debug(" > %c\n", (char)payload[i]);

            request_ack = !(sendSize % 3); //request ACK every 3rd xmission

            rfm12b_wakeup();

            uint8_t dest_node_id = RFM_NET_ID_CTRL;

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
        if(0)
        {
            if(rfm12b_rx_complete())
            {
                if(rfm12b_is_crc_pass())
                {
                    debug("[%d]\n", rfm12b_get_sender());
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

        static float i = 0;
        // debug("Fuckyou %.2f %d\n", i, rx_cnt);
        i += 0.5f;
    }
}