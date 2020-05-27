#include <stdint.h>

typedef struct
{
    float setpoint;
    float feedback;

    float p_term, i_term, d_term, i_acc_limit; // config

    float i_acc;
    float out;
    float prev_error;

    uint32_t max;
} pid_ctrl_t;

void pid_init(pid_ctrl_t *ctrl);
void pid_reset(pid_ctrl_t *ctrl);

void pid_poll(pid_ctrl_t *ctrl, float ts);