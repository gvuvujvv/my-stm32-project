/**
 * @file system.h
 * @brief 系统配置头文件
 */

#ifndef __SYSTEM_H
#define __SYSTEM_H

#include "stm32f10x.h"

/* 系统时钟频率 */
#define SYS_CLK_FREQ    72000000UL

/* 公共延时函数 */
void sys_delay_ms(uint32_t ms);
void sys_delay_us(uint32_t us);

/* 系统初始化 */
void sys_init(void);

#endif /* __SYSTEM_H */
