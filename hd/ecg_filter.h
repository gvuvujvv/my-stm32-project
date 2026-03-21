/**
 * @file ecg_filter.h
 * @brief ECG数字滤波器头文件
 * 
 * 三级级联滤波:
 * - 高通滤波器: 0.5Hz, 去除基线漂移
 * - 低通滤波器: 40Hz, 去除高频噪声
 * - 陷波滤波器: 50Hz, 去除工频干扰
 */

#ifndef __ECG_FILTER_H
#define __ECG_FILTER_H

#include <stdint.h>

/* 滤波器阶数 */
#define FILTER_ORDER    2
#define FILTER_STATES   2

/* 滤波器结构体 (直接II型) */
typedef struct {
    float b[FILTER_ORDER + 1];  /* 分子系数 */
    float a[FILTER_ORDER + 1];  /* 分母系数 */
    float w[FILTER_STATES];     /* 状态变量 */
} iir_filter_t;

/* 滤波器组 */
typedef struct {
    iir_filter_t hp;      /* 高通 */
    iir_filter_t lp;      /* 低通 */
    iir_filter_t notch;   /* 陷波 */
} ecg_filter_t;

/* 初始化滤波器组 */
void ecg_filter_init(ecg_filter_t *filt);

/* 处理单样本 */
float ecg_filter_process(ecg_filter_t *filt, float input);

/* 重置滤波器状态 */
void ecg_filter_reset(ecg_filter_t *filt);

#endif /* __ECG_FILTER_H */
