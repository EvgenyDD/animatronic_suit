#include "adc.h"
#include "debug.h"
#include "hb_tracker.h"
#include "led.h"
#include "main.h"
#include "trx.h"

#include <string.h>

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

hbt_node_t *hbt_ctrl;

uint32_t get_random(void){return HAL_RNG_GetRandomNumber(&hrng);}

const uint8_t iterator_ctrl = 1;


    

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

    hbt_ctrl = hb_tracker_init(RFM_NET_ID_CTRL, 700 /* 2x HB + 100 ms*/);

    trx_init();
    trx_init_node(iterator_ctrl, RFM_NET_ID_CTRL);
}

void loop(void)
{
    static uint32_t prev_tick = 0;
    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
    }

    trx_poll();

    static uint32_t hbtt = 0;
    if(hbtt < HAL_GetTick())
    {
        hbtt = HAL_GetTick() + 100;

        if(hb_tracker_is_timeout(hbt_ctrl))
        {
            LED_GPIO_Port->ODR |= LED_Pin;
        }
        else
        {
            LED_GPIO_Port->ODR ^= LED_Pin;
        }
    }

    static uint32_t ctr_sts_head = 0;
    if(ctr_sts_head < HAL_GetTick())
    {
        ctr_sts_head = HAL_GetTick() + 2000;

        uint8_t data[1 + 4 + 4] = {RFM_NET_CMD_STS_HEAD};
        float vbat = adc_logic_get_vbat();
        float temp = adc_logic_get_temp();
        memcpy(data + 1, &vbat, 4);
        memcpy(data + 1 + 4, &temp, 4);
        trx_send_async(iterator_ctrl, data, sizeof(data));
        // debug_rf("*");
    }
}

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
    // debug("RX: %d > Size: %d\n", sender_node_id, data_len);if(data_len > 0)
    {
        switch(data[0])
        {
        // case RFM_NET_CMD_HB:
        // {
        //     if(data_len == 2)
        //     {
        //         bool not_found = hb_tracker_update(sender_node_id);
        //         // if(not_found) debug("HB unknown %d\n", sender_node_id);
        //     }
        // }
        // break;

        case RFM_NET_CMD_LIGHT:
        {
            if(data_len == 4)
            {
                led_set_gamma(0, data[1]);
                led_set_gamma(1, data[2]);
                led_set_gamma(2, data[3]);
            }
        }

        default:
            // debug("Unknown cmd %d\n", data[0]);
            break;
        }
    }
}