/**
 * @file ecg_hr.c
 * @brief 心率计算实现
 */

#include "ecg_hr.h"
#include <string.h>
#include "stm32f10x.h"
#include "timer.h"  



/**
 * @brief 初始化
 */
void ecg_hr_init(ecg_hr_t *hr) {
    memset(hr, 0, sizeof(ecg_hr_t));
    hr->min_hr = 200;
    hr->result.status = HR_NO_SIGNAL;
    hr->result.average = 0;
    hr->result.instant = 0;
}

/**
 * @brief 添加R波
 */
uint8_t ecg_hr_add_peak(ecg_hr_t *hr, const r_peak_info_t *peak) {
    uint32_t rr_ms;
    uint16_t instant_hr;
    uint32_t sum;
    uint8_t i;
    
    if (!hr->has_last_r) {
        hr->last_r_pos = peak->position;
        hr->has_last_r = 1;
        return 0;
    }
    
    /* 计算RR间期 (ms) */
    rr_ms = (peak->position - hr->last_r_pos) * 1000 / SAMPLE_RATE;
    
    /* 验证有效性 */
    if (rr_ms < HR_MIN_VALID_RR || rr_ms > HR_MAX_VALID_RR) {
        hr->last_r_pos = peak->position;
        return 0;
    }
    
    /* 存储RR间期 */
    hr->rr_buf[hr->rr_idx] = rr_ms;
    hr->rr_idx = (hr->rr_idx + 1) % HR_RR_BUF_SIZE;
    if (hr->rr_count < HR_RR_BUF_SIZE) {
        hr->rr_count++;
    }
    
    hr->last_r_pos = peak->position;
    hr->result.last_rr = rr_ms;
    
    /* 即时心率 */
    instant_hr = (uint16_t)(60000 / rr_ms);
    hr->result.instant = instant_hr;
    
    /* 更新最小心率 */
    if (instant_hr < hr->min_hr) {
        hr->min_hr = instant_hr;
    }
    if (instant_hr > hr->max_hr) {
        hr->max_hr = instant_hr;
    }
    hr->result.min_hr = hr->min_hr;
    hr->result.max_hr = hr->max_hr;
    
    /* 平均心率 */
    if (hr->rr_count > 0) {
        sum = 0;
        for (i = 0; i < hr->rr_count; i++) {
            sum += hr->rr_buf[i];
        }
        hr->result.average = (uint16_t)(60000 / (sum / hr->rr_count));
    }
    
    /* 状态判断 */
    if (hr->result.average < 60) {
        hr->result.status = HR_BRADYCARDIA;
    } else if (hr->result.average > 100) {
        hr->result.status = HR_TACHYCARDIA;
    } else {
        hr->result.status = HR_NORMAL;
    }
    return 1;
}

/**
 * @brief 获取结果
 */
const hr_result_t* ecg_hr_get_result(ecg_hr_t *hr) {
    return &hr->result;
}

void ecg_hr_get_result_snapshot(ecg_hr_t *hr, hr_result_t *out) {
    __disable_irq();
    *out = hr->result;
    __enable_irq();
}

/**
 * @brief 获取状态字符串
 */
const char* ecg_hr_status_string(hr_status_t status) {
    switch (status) {
        case HR_NORMAL: return "Normal";
        case HR_BRADYCARDIA: return "Brady";
        case HR_TACHYCARDIA: return "Tachy";
        case HR_NO_SIGNAL: return "NoSig";
        default: return "Unknown";
    }
}

/**
 * @brief 重置
 */
void ecg_hr_reset(ecg_hr_t *hr) {
    memset(hr->rr_buf, 0, sizeof(hr->rr_buf));
    hr->rr_count = 0;
    hr->rr_idx = 0;
    hr->has_last_r = 0;
    hr->min_hr = 200;
    hr->max_hr = 0;
    hr->result.status = HR_NO_SIGNAL;
    hr->result.average = 0;
    hr->result.instant = 0;
}
