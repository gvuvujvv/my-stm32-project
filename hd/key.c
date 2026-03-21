#include "key.h"
#include "system.h"

void Key_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // Pull-up for buttons
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

uint8_t Key_GetNum(void) {
    uint8_t key_num = 0;
    if (GPIO_ReadInputDataBit(KEY_PORT, KEY1_PIN) == 0) {
        sys_delay_ms(20);
        while (GPIO_ReadInputDataBit(KEY_PORT, KEY1_PIN) == 0);
        sys_delay_ms(20);
        key_num = 1;
    }
    if (GPIO_ReadInputDataBit(KEY_PORT, KEY2_PIN) == 0) {
        sys_delay_ms(20);
        while (GPIO_ReadInputDataBit(KEY_PORT, KEY2_PIN) == 0);
        sys_delay_ms(20);
        key_num = 2;
    }
    return key_num;
}
