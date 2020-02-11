#ifndef POWER_H
#define POWER_H

#include "main.h"

#include <stdbool.h>

inline void pwr_sleep(void) { HAL_PWR_DisableWakeUpPin( PWR_WAKEUP_PIN1 );
                    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
                    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
                    HAL_PWR_EnterSTANDBYMode();}

#endif // POWER_H