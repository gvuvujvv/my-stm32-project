/**
 * @file adc.h
 * @brief ADC驱动头文件
 * 
 * 使用ADC1通道5 (PA5)
 * 12位分辨率
 */

#ifndef __ADC_H
#define __ADC_H

#include "stm32f10x.h"
#include <stdint.h>

/* ADC通道 */
#define ADC_CHANNEL     ADC_Channel_5
#define ADC_GPIO_PIN    GPIO_Pin_5

/* 初始化 */
void adc_init(void);

/* 启动一次转换 */
void adc_start(void);

/* 获取转换结果 (阻塞方式) */
uint16_t adc_read(void);

/* 检查转换是否完成 */
uint8_t adc_is_ready(void);

/* 获取结果 (非阻塞) */
uint16_t adc_get_value(void);

#endif /* __ADC_H */
