#ifndef SERVO_H
#define SERVO_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

void servo_init(void);

void servo_poll(uint32_t diff_ms);
bool servo_set_smooth_and_off(uint8_t servo, float pos_end, uint32_t delay_ms);

typedef struct
{
    uint32_t start_time;
    uint32_t stop_time;

    uint32_t start_pos;
    uint32_t end_pos;
} move_point_t;

inline void mc_set_pwm(int sel, bool dir, uint32_t value)
{
    if(sel == 0)
    {
        if(value == 0)
        {
            TIM1->CCR1 = 0;
            SRV0_1_GPIO_Port->ODR &= (uint32_t)~SRV0_1_Pin;
            TIM1->CCR2 = 0;
            SRV0_0_GPIO_Port->ODR &= (uint32_t)~SRV0_0_Pin;
        }
        else if(dir)
        {
            SRV0_0_GPIO_Port->ODR &= (uint32_t)~SRV0_0_Pin;
            TIM1->CCR2 = 0;

            TIM1->CCR1 = value;
            SRV0_1_GPIO_Port->ODR |= SRV0_1_Pin;
        }
        else
        {
            SRV0_1_GPIO_Port->ODR &= (uint32_t)~SRV0_1_Pin;
            TIM1->CCR1 = 0;

            TIM1->CCR2 = value;
            SRV0_0_GPIO_Port->ODR |= SRV0_0_Pin;
        }
    }
    else if(sel == 1)
    {
        if(value == 0)
        {
            TIM1->CCR3 = 0;
            SRV1_1_GPIO_Port->ODR &= (uint32_t)~SRV1_1_Pin;
            TIM1->CCR4 = 0;
            SRV1_0_GPIO_Port->ODR &= (uint32_t)~SRV1_0_Pin;
        }
        else if(dir)
        {
            SRV1_0_GPIO_Port->ODR &= (uint32_t)~SRV1_0_Pin;
            TIM1->CCR3 = 0;

            TIM1->CCR4 = value;
            SRV1_1_GPIO_Port->ODR |= SRV1_1_Pin;
        }
        else
        {
            SRV1_1_GPIO_Port->ODR &= (uint32_t)~SRV1_1_Pin;
            TIM1->CCR3 = 0;

            TIM1->CCR4 = value;
            SRV1_0_GPIO_Port->ODR |= SRV1_0_Pin;
        }
    }
}

#endif // SERVO_H