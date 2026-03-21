/**
 * @file ecg_detector.h
 * @brief ECG R波检测头文件
 * 
 * 使用Pan-Tompkins算法
 */

#ifndef __ECG_DETECTOR_H
#define __ECG_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

/* 参数 */
#define SAMPLE_RATE         500
#define INTEGRAL_WINDOW     75      /* 150ms @500Hz */
#define MIN_RR_INTERVAL     150     /* 300ms @500Hz */
#define SEARCH_WINDOW       40      /* 80ms @500Hz */
#define MAX_R_PEAKS         100

/* R波信息 */
typedef struct {
    uint32_t position;      /* 位置 (样本索引) */
    float amplitude;        /* 幅度 */
} r_peak_info_t;

/* 检测器状态 */
typedef struct {
    /* 微分器 */
    float diff_buf[5];
    
    /* 积分器 */
    float intg_buf[INTEGRAL_WINDOW];
    float intg_sum;
    uint16_t intg_idx;
    
    /* 峰值检测 */
    float threshold;
    float signal_energy;
    int32_t last_r_pos;
    
    /* 延迟 */
    float prev_val;
    float prev_prev_val;
    
    /* 结果 */
    r_peak_info_t peaks[MAX_R_PEAKS];
    uint16_t peak_count;
    
    /* 样本计数 */
    uint32_t sample_count;
} ecg_detector_t;

/* 初始化 */
void ecg_detector_init(ecg_detector_t *det);

/* 处理单样本,返回是否检测到R波 */
bool ecg_detector_process(ecg_detector_t *det, float filtered_sample);

/* 获取R波数量 */
uint16_t ecg_detector_get_peak_count(ecg_detector_t *det);

/* 获取最后一个R波 */
const r_peak_info_t* ecg_detector_get_last_peak(ecg_detector_t *det);

/* 重置 */
void ecg_detector_reset(ecg_detector_t *det);

#endif /* __ECG_DETECTOR_H */
