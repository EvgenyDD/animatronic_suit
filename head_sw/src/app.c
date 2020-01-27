#include "adc.h"
#include "debug.h"
#include "hb_tracker.h"
#include "main.h"
#include "trx.h"

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

    trx_init(RFM_NET_ID_HEAD);
}

void loop(void)
{
    static uint32_t prev_tick = 0;
    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
    }

    trx_poll_rx();
    trx_poll_tx_hb(280, 1, RFM_NET_ID_CTRL);

    static uint32_t ctrl_hb = 5000;
    if(ctrl_hb < HAL_GetTick())
    {
        ctrl_hb = HAL_GetTick() + 500;

        uint8_t data[] = "\x28"
                         "Hello FUCKing world\n";
        trx_send_nack(RFM_NET_ID_CTRL, data, sizeof(data));

        LED_GPIO_Port->ODR ^= LED_Pin;
    }
}

void process_data(uint8_t sender_node_id, const volatile uint8_t *data, uint8_t data_len)
{
    // debug("RX: %d > Size: %d\n", sender_node_id, data_len);if(data_len > 0)
    {
        switch(data[0])
        {
        case RFM_NET_CMD_HB:
        {
            bool not_found = hb_tracker_update(sender_node_id);
            if(not_found) debug("HB unknown %d\n", sender_node_id);
        }
        break;

        default:
            // debug("Unknown cmd %d\n", data[0]);
            break;
        }
    }
}