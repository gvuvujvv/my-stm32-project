/**
 * @file ecg_filter.c
 * @brief ECG数字滤波器实现
 * 
 * 使用MATLAB生成的系数:
 * 高通: butter(2, 0.5/250, 'high')
 * 低通: butter(2, 40/250, 'low')
 * 陷波: iirnotch(50/250, 35)
 */

#include "ecg_filter.h"
#include <string.h>

/* 高通滤波器系数 (0.5Hz) */
static const float HP_B[3] = {0.995567f, -1.991134f, 0.995567f};
static const float HP_A[3] = {1.0f, -1.991114f, 0.991154f};

/* 低通滤波器系数 (40Hz) */
static const float LP_B[3] = {0.046132f, 0.092264f, 0.046132f};
static const float LP_A[3] = {1.0f, -1.307285f, 0.491812f};

/* 陷波滤波器系数 (50Hz) */
static const float NOTCH_B[3] = {0.991104f, -1.603639f, 0.991104f};
static const float NOTCH_A[3] = {1.0f, -1.603639f, 0.982207f};

/**
 * @brief 初始化单个IIR滤波器
 */
static void iir_init(iir_filter_t *filt, const float *b, const float *a) {
    uint8_t i;
    for (i = 0; i <= FILTER_ORDER; i++) {
        filt->b[i] = b[i];
        filt->a[i] = a[i];
    }
    filt->w[0] = 0.0f;
    filt->w[1] = 0.0f;
}

/**
 * @brief IIR滤波 (直接II型)
 */
static float iir_process(iir_filter_t *filt, float x) {
    float w, y;
    
    /* w[n] = x[n] - a1*w[n-1] - a2*w[n-2] */
    w = x - filt->a[1] * filt->w[0] - filt->a[2] * filt->w[1];
    
    /* y[n] = b0*w[n] + b1*w[n-1] + b2*w[n-2] */
    y = filt->b[0] * w + filt->b[1] * filt->w[0] + filt->b[2] * filt->w[1];
    
    /* 更新状态 */
    filt->w[1] = filt->w[0];
    filt->w[0] = w;
    
    return y;
}

/**
 * @brief 初始化滤波器组
 */
void ecg_filter_init(ecg_filter_t *filt) {
    iir_init(&filt->hp, HP_B, HP_A);
    iir_init(&filt->lp, LP_B, LP_A);
    iir_init(&filt->notch, NOTCH_B, NOTCH_A);
}

/**
 * @brief 处理单样本
 * 
 * 处理流程: 高通 -> 低通 -> 陷波
 */
float ecg_filter_process(ecg_filter_t *filt, float input) {
    float output;
    
    output = iir_process(&filt->hp, input);
    output = iir_process(&filt->lp, output);
    output = iir_process(&filt->notch, output);
    
    return output;
}

/**
 * @brief 重置滤波器状态
 */
void ecg_filter_reset(ecg_filter_t *filt) {
    filt->hp.w[0] = 0.0f;
    filt->hp.w[1] = 0.0f;
    filt->lp.w[0] = 0.0f;
    filt->lp.w[1] = 0.0f;
    filt->notch.w[0] = 0.0f;
    filt->notch.w[1] = 0.0f;
}
