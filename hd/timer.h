/**
 * @file timer.h
 * @brief 定时器驱动头文件
 * 
 * 使用TIM2产生500Hz采样中断
 */

#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f10x.h"
#include <stdint.h>

/* 采样频率 */
#define SAMPLE_RATE     500
#define SAMPLE_PERIOD   (1000 / SAMPLE_RATE)  /* ms */

/* 回调函数类型 */
typedef void (*timer_callback_t)(void);

/* 初始化 */
void timer_init(void);

/* 启动定时器 */
void timer_start(void);

/* 停止定时器 */
void timer_stop(void);

/* 设置回调函数 */
void timer_set_callback(timer_callback_t cb);

/* 获取系统运行时间 (ms) */
uint32_t timer_get_ms(void);

// 新增：供 IRQ 调用
void timer_process_sampling(void);

#endif /* __TIMER_H */
