/**
 * @file ecg_detector.c
 * @brief ECG R波检测实现 (Pan-Tompkins算法)
 */

#include "ecg_detector.h"
#include <string.h>
#include <math.h>

/* 微分器系数 (5点) */
static const float DIFF_COEF[5] = {-62.5f, -125.0f, 0.0f, 125.0f, 62.5f};

/**
 * @brief 初始化检测器
 */
void ecg_detector_init(ecg_detector_t *det) {
    memset(det, 0, sizeof(ecg_detector_t));
    det->last_r_pos = -MIN_RR_INTERVAL;
}

/**
 * @brief 处理单样本
 */
bool ecg_detector_process(ecg_detector_t *det, float sample) {
    float diff_out, sqr_out, intg_out;
    bool r_detected = false;
    
    det->sample_count++;
    
    /* 1. 微分 */
    memmove(&det->diff_buf[1], &det->diff_buf[0], 4 * sizeof(float));
    det->diff_buf[0] = sample;
    
    diff_out = det->diff_buf[0] * DIFF_COEF[0] +
               det->diff_buf[1] * DIFF_COEF[1] +
               det->diff_buf[2] * DIFF_COEF[2] +
               det->diff_buf[3] * DIFF_COEF[3] +
               det->diff_buf[4] * DIFF_COEF[4];
    
    /* 2. 平方 */
    sqr_out = diff_out * diff_out;
    
    /* 3. 移动窗口积分 */
    det->intg_sum = det->intg_sum - det->intg_buf[det->intg_idx] + sqr_out;
    det->intg_buf[det->intg_idx] = sqr_out;
    det->intg_idx = (det->intg_idx + 1) % INTEGRAL_WINDOW;
    intg_out = det->intg_sum / INTEGRAL_WINDOW;
    
    /* 4. 信号能量估计 */
    det->signal_energy = 0.99f * det->signal_energy + 0.01f * intg_out;
    
    /* 5. 阈值更新 */
    if (det->threshold > 0) {
        det->threshold *= 0.998f;
    }
    float min_thresh = 1.5f * det->signal_energy;
    if (det->threshold < min_thresh) {
        det->threshold = min_thresh;
    }
    
    /* 6. 峰值检测 */
    if (det->sample_count >= 2) {
        bool is_peak = (det->prev_val > det->prev_prev_val) && 
                       (det->prev_val > intg_out);
        bool interval_ok = ((int32_t)det->sample_count - 1 - det->last_r_pos) >= MIN_RR_INTERVAL;
        
        if (is_peak && det->prev_val > det->threshold && interval_ok) {
            /* 在原始信号中精确定位 */
            int32_t search_start = (int32_t)det->sample_count - 1 - SEARCH_WINDOW;
            if (search_start < 0) search_start = 0;
            
            /* 存储R波 (循环使用缓存，仅保留最新 R 波) */
            uint16_t idx = det->peak_count % MAX_R_PEAKS;
            det->peaks[idx].position = det->sample_count - 1;
            det->peaks[idx].amplitude = sample;
            det->peak_count++;
            
            det->last_r_pos = det->sample_count - 1;
            det->threshold = 0.4f * det->prev_val;
            r_detected = true;
        }
    }
    
    det->prev_prev_val = det->prev_val;
    det->prev_val = intg_out;
    
    return r_detected;
}

/**
 * @brief 获取R波数量
 */
uint16_t ecg_detector_get_peak_count(ecg_detector_t *det) {
    return det->peak_count;
}

/**
 * @brief 获取最后一个R波
 */
const r_peak_info_t* ecg_detector_get_last_peak(ecg_detector_t *det) {
    if (det->peak_count == 0) {
        return NULL;
    }
    uint16_t last_idx = (det->peak_count - 1) % MAX_R_PEAKS;
    return &det->peaks[last_idx];
}

/**
 * @brief 重置
 */
void ecg_detector_reset(ecg_detector_t *det) {
    memset(det->diff_buf, 0, sizeof(det->diff_buf));
    memset(det->intg_buf, 0, sizeof(det->intg_buf));
    
    det->intg_sum = 0;
    det->intg_idx = 0;
    det->threshold = 0;
    det->signal_energy = 0;
    det->last_r_pos = -MIN_RR_INTERVAL;
    det->prev_val = 0;
    det->prev_prev_val = 0;
    det->peak_count = 0;
}
