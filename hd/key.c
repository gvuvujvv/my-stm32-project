#include "key.h"
#include "system.h"

void Key_Init(void) {
    /* 新版按键在 GPIOB */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* 关闭 JTAG，保留 SWD，释放 PB4 */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
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
