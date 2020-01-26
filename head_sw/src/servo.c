#include "servo.h"
#include "debug.h"
#include "main.h"

#include <string.h>

enum
{
    SERVO_R_B,
    SERVO_R_F,
    SERVO_EAR_R,
    SERVO_EAR_L,
    SERVO_L_F,
    SERVO_L_B,
};

#define MAX_POINTS 64
move_point_t points[SERVO_COUNT][MAX_POINTS];
uint32_t points_push[SERVO_COUNT];
uint32_t points_pop[SERVO_COUNT];
uint32_t points_timer[SERVO_COUNT];

uint32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    if(x < in_min)
    {
        x = in_min;
    }
    if(x > in_max)
    {
        x = in_max;
    }
    return (uint32_t)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

void servo_init(void)
{
    memset(points, 0, sizeof(points));
    memset(points_push, 0, sizeof(points_push));
    memset(points_pop, 0, sizeof(points_pop));
    memset(points_timer, 0, sizeof(points_timer));
}

bool servo_set(uint8_t servo, uint32_t value)
{
    uint32_t v = map((int32_t)value, 0, 1000, 400, 5600);
    if(value == 0) v = 0;
    __IO uint32_t *ptr[] = {
        &TIM1->CCR1,
        &TIM1->CCR2,
        &TIM1->CCR3,
        &TIM1->CCR4,
        &TIM4->CCR1,
        &TIM4->CCR2,
        &TIM4->CCR3};
    if(servo > 6) return true;
    *ptr[servo] = v;
    return false;
    // switch(servo)
    // {
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // // case: break;
    // default: return true;
    // }
    // if(servo == 0)
    //     TIM1->CCR1 = v;
    // else if(servo == 1)
    // {
    //     TIM1->CCR2 = v;
    //     return false;
    // }
    // else if(servo == 2)
    // {
    //     TIM1->CCR3 = v;
    //     return false;
    // }
    // else if(servo == 3)
    // {
    //     TIM1->CCR4 = v;
    //     return false;
    // }
    // else if(servo == 4)
    // {
    //     TIM4->CCR1 = v;
    //     return false;
    // }
    // else if(servo == 5)
    // {
    //     TIM4->CCR2 = v;
    //     return false;
    // }
    // else if(servo == 6)
    // {
    //     TIM4->CCR3 = v;
    //     return false;
    // }
    // return true;
}

void servo_print(void)
{
    debug("Servo: \n\t 0\t%d\n\t 1\t%d\n\t 2\t%d\n\t 3\t%d\n\t 4\t%d\n\t 5\t%d\n\t 6\t%d\n",
          TIM1->CCR1 / 2,
          TIM1->CCR2 / 2,
          TIM1->CCR3 / 2,
          TIM1->CCR4 / 2,
          TIM4->CCR1 / 2,
          TIM4->CCR2 / 2,
          TIM4->CCR3 / 2);
}

void servo_cb(uint32_t diff_ms)
{
    if(diff_ms > 0)
    {
    }
}