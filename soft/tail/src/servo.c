#include "servo.h"
#include "debug.h"
#include "main.h"

#include "pid.h"

#include <string.h>

#include "adc.h" // for debug
extern pid_ctrl_t srv0;

#define NUM_SERVOS 2
#define MAX_POINTS 64
move_point_t points[NUM_SERVOS][MAX_POINTS];
uint32_t points_push[NUM_SERVOS];
uint32_t points_pop[NUM_SERVOS];
uint32_t points_timer[NUM_SERVOS];

typedef enum
{
    MODE_OFF = 0,
    MODE_MOVE,
    MODE_DELAY_BEFORE_OFF,
} Servo_mode;

typedef struct
{
    Servo_mode mode;
    float pos_end;
    float pos_start;
    uint32_t time_start;
    uint32_t time_end;
} servo_t;

servo_t servo_move[1];

// uint32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
// {
//     if(x < in_min)
//     {
//         x = in_min;
//     }
//     if(x > in_max)
//     {
//         x = in_max;
//     }
//     return (uint32_t)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
// }

void servo_init(void)
{
    memset(points, 0, sizeof(points));
    memset(points_push, 0, sizeof(points_push));
    memset(points_pop, 0, sizeof(points_pop));
    memset(points_timer, 0, sizeof(points_timer));

    memset(servo_move, 0, sizeof(servo_move));
}

bool servo_set_smooth_and_off(uint8_t servo, float pos_end, uint32_t delay_ms)
{
    servo_move[servo].pos_start = (float)adc_get_raw(0) * 0.00024420024420024420024420024420024f;
    servo_move[servo].pos_end = pos_end;
    servo_move[servo].time_start = HAL_GetTick();
    servo_move[servo].time_end = servo_move[servo].time_start + delay_ms;
    servo_move[servo].mode = MODE_MOVE;

    return false;
}

void servo_print(void)
{
}

void servo_poll(uint32_t diff_ms)
{
    if(diff_ms > 0)
    {
        uint32_t time_now = HAL_GetTick();
        for(uint32_t i = 0; i < 1; i++)
        {
            if(servo_move[i].mode == MODE_MOVE)
            {
                if(servo_move[i].time_end <= time_now)
                {
                    srv0.setpoint = servo_move[i].pos_end; // servo_set(i, servo_move[i].pos_end);

                    servo_move[i].time_end += 50; // delay before off
                    servo_move[i].mode = MODE_DELAY_BEFORE_OFF;
                }
                else
                {
                    float pos = servo_move[i].pos_start + (servo_move[i].pos_end - servo_move[i].pos_start) *
                                                              (float)(time_now - servo_move[i].time_start) /
                                                              (float)(servo_move[i].time_end - servo_move[i].time_start);
                    srv0.setpoint = pos; // servo_set(i, pos);
                    // if(servo_move[i].time_print_state < time_now)
                    // {
                    //     debug_rf("%d>%d|%d|%d|%d|%d|%d", pos,
                    //              adc_get_raw(ADC_SAIN_0),
                    //              adc_get_raw(ADC_SAIN_1),
                    //              adc_get_raw(ADC_SAIN_2),
                    //              adc_get_raw(ADC_SAIN_3),
                    //              adc_get_raw(ADC_SAIN_4),
                    //              adc_get_raw(ADC_SAIN_5));
                    //     servo_move[i].time_print_state = time_now + 180;
                    // }
                }
            }
            else if(servo_move[i].mode == MODE_DELAY_BEFORE_OFF)
            {
                if(servo_move[i].time_end < time_now)
                {
                    srv0.setpoint = 0.0; // servo_set(i, 0);

                    servo_move[i].mode = MODE_OFF;
                }
            }
        }
    }
}
