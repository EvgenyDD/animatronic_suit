#include "adc.h"
#include "debug.h"
#include "fan.h"
#include "led.h"
#include "main.h"
#include "servo.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int strcmp_(char *s1, char *s2)
{
    for(;;)
    {
        if(*s1 == '\n' || *s1 == '\0') return 1;
        if(*s2 == '\n' || *s2 == '\0') return 1;
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
        if(*ss == '\n' || ss == '\0') return 0;
        if(*ss == ' ') return ss - s + 1;
    }
}

void debug_parse(char *s)
{
    if(strcmp_(s, "info"))
    {
        // debug("Popadalovo\n");

        // unsigned int state = 0;

        // int c = sscanf(&s[find_space(s)], "%u", &state);
        // if(c > 0)
        // {
        //     debug("YEAH %d\n", state);
        // }
        servo_print();
        adc_print();
        debug("Btn: %d\n", BTN_SNS_GPIO_Port->IDR & BTN_SNS_Pin ? 1 : 0);
    }
    else if(strcmp_(s, "servo"))
    {
        unsigned int servo = 0, val = 0;

        int c = sscanf(&s[find_space(s)], "%u %u", &servo, &val);
        if(c == 2)
        {
            debug("Set Servo %d to %d\n", servo, val);
            servo_set(servo, val);
        }
        else
            debug("servo: fail Input\n");
    }
    else if(strcmp_(s, "fan"))
    {
        unsigned int fan = 0, val = 0;

        int c = sscanf(&s[find_space(s)], "%u %u", &fan, &val);
        if(c == 2)
        {
            debug("Set Fan %d to %d\n", fan, val);
            fan_set(fan, val);
        }
        else
            debug("fan: fail Input\n");
    }
    else if(strcmp_(s, "led"))
    {
        unsigned int led = 0, val = 0;

        int c = sscanf(&s[find_space(s)], "%u %u", &led, &val);
        if(c == 2)
        {
            debug("Set LED %d to %d\n", led, val);
            led_set(led, val);
        }
        else
            debug("LED: fail Input\n");
    }
    else
    {
        debug("Not Found!\n");
    }
}