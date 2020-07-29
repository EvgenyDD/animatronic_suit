#include "adc.h"
#include "debug.h"
#include "fan.h"
#include "led.h"
#include "main.h"
#include "power.h"
#include "servo.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pid.h"

extern pid_ctrl_t srv0;
extern bool start_servo, start_servo2;

static int strcmp_(char *s1, char *s2)
{
    for(int cnt = 0;; cnt++)
    {
        // if(*s1 == '\n' || *s1 == '\0') return cnt == 0 ? 0 : 1;
        if(*s2 == '\n' || *s2 == '\0') return cnt == 0 ? 0 : 1;
        if(*s1 != *s2) return 0;

        s1++;
        s2++;
    }
}

static int find_space(char *s)
{
    char *ss = s;
    for(;; ss++)
    {
        if(*ss == '\n' || *ss == '\0') return 0;
        if(*ss == ' ') return ss - s + 1;
    }
}

void debug_parse(char *s)
{
    if(strcmp_(s, "info"))
    {
        // debug_rf("Popadalovo\n");

        // unsigned int state = 0;

        // int c = sscanf(&s[find_space(s)], "%u", &state);
        // if(c > 0)
        // {
        //     debug_rf("YEAH %d\n", state);
        // }

        // adc_print();
        debug("ADC: %d %d\n", adc_get_raw(0), adc_get_raw(1));
        debug("err: %.3f\n", srv0.prev_error);
        debug_rf("out %.5f\n", srv0.out);
        debug_rf("max %d\n", srv0.max);
    }
    else if(strcmp_(s, "servo"))
    {
        start_servo = true;
        // unsigned int servo = 0, val = 0;

        // int c = sscanf(&s[find_space(s)], "%u %u", &servo, &val);
        // if(c == 2)
        // {
        //     debug_rf("Set Servo %d to %d\n", servo, val);
        //     // servo_set(servo, val);
        // }
        // else
        //     debug_rf("servo: fail Input\n");
    }
    else if(strcmp_(s, "fervo"))
    {
        start_servo2 = true;
        // unsigned int servo = 0, val = 0;

        // int c = sscanf(&s[find_space(s)], "%u %u", &servo, &val);
        // if(c == 2)
        // {
        //     debug_rf("Set Servo %d to %d\n", servo, val);
        //     // servo_set(servo, val);
        // }
        // else
        //     debug_rf("servo: fail Input\n");
    }
    else if(strcmp_(s, "p"))
    {
        unsigned int val = 0;

        int c = sscanf(&s[find_space(s)], "%u", &val);
        if(c == 1)
        {
            srv0.p_term = (float)val / 100.0f;
            debug_rf("Set P %.5f\n", srv0.p_term);
            // servo_set_smooth_and_off(servo, val, delay_ms);
        }
        else
            debug_rf("Servos: fail Input\n");
    }
    else if(strcmp_(s, "i"))
    {
        unsigned int val = 0;

        int c = sscanf(&s[find_space(s)], "%u", &val);
        if(c == 1)
        {
            srv0.i_term = (float)val / 100.0f;
            debug_rf("Set I %.5f\n", srv0.i_term);
            // servo_set_smooth_and_off(servo, val, delay_ms);
        }
        else
            debug_rf("Servos: fail Input\n");
    }
    else if(strcmp_(s, "d"))
    {
        unsigned int val = 0;

        int c = sscanf(&s[find_space(s)], "%u", &val);
        if(c == 1)
        {
            srv0.d_term = (float)val / 100.0f;
            debug_rf("Set I %.5f\n", srv0.d_term);
            // servo_set_smooth_and_off(servo, val, delay_ms);
        }
        else
            debug_rf("Servos: fail Input\n");
    }
    else if(strcmp_(s, "e"))
    {
        static uint8_t sp = 0;

        if(sp == 0)
        {
            srv0.setpoint = 0.4;
            sp++;
        }
        else if(sp == 1)
        {
            srv0.setpoint = 0.6;
            sp++;
        }
        else
        {
            srv0.setpoint = 0.5;
            sp = 0;
        }

        debug_rf("Servo sp : %.2f\n", srv0.setpoint);
    }
    else if(strcmp_(s, "w"))
    {
        unsigned int servo = 0, val = 0, delay_ms = 0;

        int c = sscanf(&s[find_space(s)], "%u %u %u", &servo, &val, &delay_ms);
        if(c == 3)
        {
            debug_rf("Set Servo Smth %d to %d %d ms\n", servo, val, delay_ms);
            servo_set_smooth_and_off(servo, (float)val * 0.0001f, delay_ms);
        }
        else
            debug_rf("Servos: fail Input\n");
    }
    else if(strcmp_(s, "1+"))
    {
        mc_set_pwm_0(1, 200);
        HAL_Delay(400);
        mc_set_pwm_0(1, 0);
    }
    else if(strcmp_(s, "1-"))
    {
        mc_set_pwm_0(0, 200);
        HAL_Delay(400);
        mc_set_pwm_0(0, 0);
    }
    else if(strcmp_(s, "0+"))
    {
        mc_set_pwm_1(1, 200);
        HAL_Delay(400);
        mc_set_pwm_1(1, 0);
    }
    else if(strcmp_(s, "0-"))
    {
        mc_set_pwm_1(0, 200);
        HAL_Delay(400);
        mc_set_pwm_1(0, 0);
    }
    else
    {
        debug_rf("Not Found!\n");
    }
}