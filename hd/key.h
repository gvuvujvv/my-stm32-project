#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/**
 * @brief 按键硬件定义（对应新版原理图 v3.0）
 *
 * SW2 -> PB4
 * SW3 -> PB5
 *
 * 按键采用上拉输入，按下为低电平。
 */
#define KEY1_PIN  GPIO_Pin_4
#define KEY2_PIN  GPIO_Pin_5
#define KEY_PORT  GPIOB

void Key_Init(void);
uint8_t Key_GetNum(void);

#endif /* __KEY_H */
