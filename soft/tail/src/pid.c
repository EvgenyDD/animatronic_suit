#include "pid.h"
#include <string.h>

void pid_init(pid_ctrl_t *ctrl)
{
    memset(ctrl, 0, sizeof(pid_ctrl_t));
}

void pid_reset(pid_ctrl_t *ctrl)
{
    ctrl->prev_error = 0;
    ctrl->i_acc = 0;
}

void pid_poll(pid_ctrl_t *ctrl, float ts)
{
    float error = ctrl->setpoint - ctrl->feedback;

    ctrl->i_acc += error * ts;

    float i_val = ctrl->i_term * ctrl->i_acc;
    if(i_val > ctrl->i_acc_limit)
    {
        i_val = ctrl->i_acc_limit;
        ctrl->i_acc = ctrl->i_acc_limit / ctrl->i_term;
    }
    if(i_val < -ctrl->i_acc_limit)
    {
        i_val = -ctrl->i_acc_limit;
        ctrl->i_acc = -ctrl->i_acc_limit / ctrl->i_term;
    }

    ctrl->out = ctrl->p_term * error +
                ctrl->d_term * ctrl->prev_error / ts +
                i_val;

#define LIMIT 1.0f
    if(ctrl->out > LIMIT) ctrl->out = LIMIT;
    if(ctrl->out < -LIMIT) ctrl->out = -LIMIT;

    ctrl->prev_error = error;
}