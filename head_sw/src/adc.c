#include "adc.h"
#include "debug.h"
#include "main.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;

static const float v_ref = 3.3f;
static const float adc_max_cnt = 4095.0f;

#define NTC_COEF 3434.0f
//NTC -> lo side
#define NTC_RES_LO_SIDE(adc_val) ((adc_val * 10000.0f) / (adc_max_cnt - adc_val))
#define NTC_TEMP_LO_SIDE(adc_raw, coef) (1.0f / ((logf(NTC_RES_LO_SIDE(adc_raw) / 47000.0f) / coef) + (1.0 / 298.15f)) - 273.15f)

enum
{
    ADC_SAIN_0,
    ADC_SAIN_1,
    ADC_SAIN_2,
    ADC_SAIN_3,
    ADC_SAIN_4,
    ADC_SAIN_5,
    ADC_SAIN_VBAT,
    ADC_SAIN_NTC0,
    ADC_SAIN_RSSI,

    ADC_CH_NUM
};

float adc_logic_get_temp(void)
{
    return NTC_TEMP_LO_SIDE(adc_get_raw(ADC_SAIN_NTC0), NTC_COEF);
}

float adc_logic_get_vbat(void)
{
    return (float)adc_get_raw(ADC_SAIN_VBAT) / adc_max_cnt * v_ref * (1.0f + 47.0f / 10.0f);
}

uint32_t adc_sel = 0; // offset, values: 0 or ADC_CH_NUM
static uint16_t adc_raw[ADC_CH_NUM * 2];

void adc_init(void)
{
    HAL_ADC_Start(&hadc1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_raw, ADC_CH_NUM * 2);
#pragma GCC diagnostic pop
    __HAL_TIM_ENABLE(&htim3);
}

void adc_print(void)
{
    for(uint32_t i = 0; i < ADC_CH_NUM; i++)
    {
        debug("ADC: %d %d\n", i, adc_get_raw(i));
    }
    debug("Temp: %.1f\n", adc_logic_get_temp());
}

uint16_t adc_get_raw(uint32_t channel) { return adc_raw[adc_sel + channel]; }

void adc_drv_conv_complete_half(void)
{
    adc_sel = 0;
    // filter_values();
}

void adc_drv_conv_complete_full(void)
{
    adc_sel = ADC_CH_NUM;
    // filter_values();
}
