#ifndef POWER_H
#define POWER_H

#include "main.h"
#include "rfm12b.h"

#include <stdbool.h>

inline void pwr_sleep(void) {
                    rfm12b_sleep();
                    CHRG_EN_GPIO_Port->ODR &= ~CHRG_EN_Pin;
                    HAL_PWR_DisableWakeUpPin( PWR_WAKEUP_PIN1 );
                    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
                    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
                    HAL_PWR_EnterSTANDBYMode();}

#endif // POWER_H