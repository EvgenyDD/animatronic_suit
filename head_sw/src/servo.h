#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>
#include <stdint.h>

#define SERVO_COUNT 7

bool servo_set(uint8_t servo, uint32_t value);
void servo_print(void);

typedef struct
{
    uint32_t start_time;
    uint32_t stop_time;

    uint32_t start_pos;
    uint32_t end_pos;
} move_point_t;

#endif // SERVO_H