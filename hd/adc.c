/**
 * @file adc.c
 * @brief ADC驱动实现
 */

#include "adc.h"

/* 全局变量: 存储ADC结果 */
static volatile uint16_t g_adc_result = 0;
static volatile uint8_t g_adc_ready = 0;

/**
 * @brief 初始化ADC
 */
void adc_init(void) {
    GPIO_InitTypeDef gpio_cfg;
    ADC_InitTypeDef adc_cfg;
    
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
                           RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  /* 72MHz / 6 = 12MHz */
    
    /* PA5 - 模拟输入 */
    gpio_cfg.GPIO_Pin = ADC_GPIO_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio_cfg);
    
    /* 配置ADC */
    adc_cfg.ADC_Mode = ADC_Mode_Independent;
    adc_cfg.ADC_ScanConvMode = DISABLE;
    adc_cfg.ADC_ContinuousConvMode = DISABLE;
    adc_cfg.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc_cfg.ADC_DataAlign = ADC_DataAlign_Right;
    adc_cfg.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc_cfg);
    
    /* 配置通道 */
    ADC_RegularChannelConfig(ADC1, ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);
    
    /* 使能ADC */
    ADC_Cmd(ADC1, ENABLE);
    
    /* 校准 */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

/**
 * @brief 启动一次转换
 */
void adc_start(void) {
    g_adc_ready = 0;
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/**
 * @brief 获取转换结果 (阻塞方式)
 */
uint16_t adc_read(void) {
    adc_start();
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

/**
 * @brief 检查转换是否完成
 */
uint8_t adc_is_ready(void) {
    if (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == SET) {
        g_adc_result = ADC_GetConversionValue(ADC1);
        g_adc_ready = 1;
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
        return 1;
    }
    return 0;
}

/**
 * @brief 获取结果 (非阻塞)
 */
uint16_t adc_get_value(void) {
    return g_adc_result;
}
