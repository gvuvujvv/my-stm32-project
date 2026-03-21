#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define KEY1_PIN  GPIO_Pin_11
#define KEY2_PIN  GPIO_Pin_12
#define KEY_PORT  GPIOA

void Key_Init(void);
uint8_t Key_GetNum(void);

#endif /* __KEY_H */
