/**
 * @file system.c
 * @brief 系统配置实现
 */

#include "system.h"

/**
 * @brief 毫秒延时
 */
void sys_delay_ms(uint32_t ms) {
    volatile uint32_t n;
    while (ms--) {
        for (n = 0; n < 7200; n++);
    }
}

/**
 * @brief 微秒延时
 */
void sys_delay_us(uint32_t us) {
    volatile uint32_t n;
    while (us--) {
        for (n = 0; n < 8; n++);
    }
}

/**
 * @brief 配置系统时钟为72MHz
 */
static void clock_config(void) {
    /* 使能外部晶振 */
    RCC_HSEConfig(RCC_HSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
    
    /* 配置Flash延迟 */
    FLASH_SetLatency(FLASH_Latency_2);
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    
    /* 配置PLL: 8MHz * 9 = 72MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
    
    /* 切换系统时钟到PLL */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08);
    
    /* 配置AHB、APB1、APB2分频 */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
}

/**
 * @brief 系统初始化
 */
void sys_init(void) {
    clock_config();
}
