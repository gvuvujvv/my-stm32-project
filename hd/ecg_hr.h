/**
 * @file ecg_hr.h
 * @brief 心率计算头文件
 */

#ifndef __ECG_HR_H
#define __ECG_HR_H

#include <stdint.h>
#include "ecg_detector.h"

/* 参数 */
#define HR_RR_BUF_SIZE      8
#define HRV_RR_BUF_SIZE     32
#define HRV_MIN_RR_COUNT    8
#define HR_MIN_VALID_RR     300     /* ms */
#define HR_MAX_VALID_RR     2000    /* ms */

/* 心率状态 */
typedef enum {
    HR_NORMAL = 0,
    HR_BRADYCARDIA,     /* 心动过缓 <60 */
    HR_TACHYCARDIA,     /* 心动过速 >100 */
    HR_NO_SIGNAL
} hr_status_t;

/* 心率结果 */
typedef struct {
    uint16_t instant;       /* 即时心率 */
    uint16_t average;       /* 平均心率 */
    uint16_t min_hr;        /* 最小心率 */
    uint16_t max_hr;        /* 最大心率 */
    uint32_t last_rr;       /* 上一个RR间期 (ms) */
    uint16_t sdnn;          /* RR间期标准差 (ms) */
    uint16_t rmssd;         /* 相邻RR差值均方根 (ms) */
    uint8_t pnn50;          /* 相邻RR差值>50ms比例 (%) */
    uint8_t hrv_count;      /* HRV有效RR数量 */
    uint8_t hrv_ready;      /* HRV指标是否可用 */
    hr_status_t status;     /* 状态 */
} hr_result_t;

/* 心率计算器 */
typedef struct {
    uint32_t rr_buf[HR_RR_BUF_SIZE];
    uint8_t rr_count;
    uint8_t rr_idx;
    uint32_t last_r_pos;
    uint8_t has_last_r;
    uint16_t min_hr;
    uint16_t max_hr;
    uint32_t hrv_rr_buf[HRV_RR_BUF_SIZE];
    uint8_t hrv_rr_count;
    uint8_t hrv_rr_idx;
    hr_result_t result;
} ecg_hr_t;

/* 初始化 */
void ecg_hr_init(ecg_hr_t *hr);

/* 添加R波 */
uint8_t ecg_hr_add_peak(ecg_hr_t *hr, const r_peak_info_t *peak);

/* 获取结果 */
const hr_result_t* ecg_hr_get_result(ecg_hr_t *hr);
void ecg_hr_get_result_snapshot(ecg_hr_t *hr, hr_result_t *out);

/* 获取状态字符串 */
const char* ecg_hr_status_string(hr_status_t status);

/* 重置 */
void ecg_hr_reset(ecg_hr_t *hr);

#endif /* __ECG_HR_H */
