#include "key.h"
#include "system.h"

/**
 * @brief 按键初始化
 *
 * - 使用 GPIOB
 * - PB4 / PB5 上拉输入
 * - 关闭 JTAG，释放 PB4 引脚
 */
void Key_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    /* 关闭 JTAG，保留 SWD（否则 PB4 无法作为普通 IO 使用） */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

/**
 * @brief 按键扫描（带消抖）
 * @return 0=无按键，1=KEY1，2=KEY2
 */
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
