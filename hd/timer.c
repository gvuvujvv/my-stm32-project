/**
 * @file timer.c
 * @brief 定时器驱动实现
 */

#include "timer.h"
#include "stm32f10x.h"
#include "adc.h"
#include "uart.h"
#include "ecg_filter.h"
#include "oled.h"
#include <stddef.h>
#include <math.h>

static volatile uint32_t g_sys_ms = 0;
static timer_callback_t g_timer_cb = NULL;

volatile uint16_t g_ble_ecg_sample = 0;
volatile uint8_t g_data_ready = 0;

// 引用主程序的滤波器
extern ecg_filter_t g_filter;

#include "ecg_detector.h"
#include "ecg_hr.h"

// 引用主程序的结构体
extern ecg_detector_t g_detector;
extern ecg_hr_t g_hr;
extern volatile uint8_t g_peak_flag;

static void buzzer_write(uint8_t on) {
    GPIO_WriteBit(GPIOB, GPIO_Pin_13, on ? Bit_SET : Bit_RESET);
}

static void buzzer_update(uint8_t alarm_mode) {
    static uint8_t mode = 0;
    static uint8_t state = 0;
    static uint16_t phase_ms = 0;

    uint16_t on_ms = 0;
    uint16_t off_ms = 0;

    if (g_sys_ms < 3000) {
        buzzer_write(0);
        mode = 0;
        state = 0;
        phase_ms = 0;
        return;
    }

    if (alarm_mode == 0) {
        mode = 0;
        state = 0;
        phase_ms = 0;
        buzzer_write(0);
        return;
    }

    if (alarm_mode != mode) {
        mode = alarm_mode;
        state = 1;
        phase_ms = 0;
    }

    if (mode == 1) {
        on_ms = 200;
        off_ms = 800;
    } else {
        on_ms = 100;
        off_ms = 300;
    }

    phase_ms += SAMPLE_PERIOD;
    if (state) {
        if (phase_ms >= on_ms) {
            state = 0;
            phase_ms = 0;
        }
    } else {
        if (phase_ms >= off_ms) {
            state = 1;
            phase_ms = 0;
        }
    }
    buzzer_write(state);
}

/**
 * @brief 定时中断处理 (500Hz)
 */
void timer_process_sampling(void) {
    static uint32_t last_hr_update_ms = 0;
    static uint32_t prev_last_rr = 0;
    static uint16_t rail_cnt = 0;
    static float ble_baseline = 0.0f;
    static uint16_t ble_settle = 0;

    /* 1. 读取ADC原始值 */
    uint16_t adc_val = adc_read();
    
    /* 2. 转换为电压值 (0-3.3V -> 0-4095) */
    float voltage = (float)adc_val * 3.3f / 4095.0f;
    
    /* 3. 滤波处理 (唯一的处理点，避免重入) */
    float filtered = ecg_filter_process(&g_filter, voltage);

    /* BLE 发送数据选择：发送“滤波后信号”的定点映射（方案B）
     * 目的：小程序端无需再做复杂滤波，直接绘图就更像 OLED 上的心电波形。
     *
     * 编码方式：
     * - 对滤波信号做慢速基线跟踪，得到近似零均值的 normalized = filtered - baseline
     * - 映射为 12bit 无符号：ecg12 = 2048 + round(normalized * BLE_SCALE)
     * - 结果裁剪到 0..4095
     *
     * 小程序端解码：
     * - signed = ecg12 - 2048
     * - normalized(V) ≈ signed / BLE_SCALE
     */
#define BLE_ECG_OFFSET   2048
#define BLE_ECG_SCALE    1024.0f
    if (ble_settle < 500) {
        ble_baseline = filtered;
        ble_settle++;
        g_ble_ecg_sample = BLE_ECG_OFFSET;
    } else {
        ble_baseline = 0.995f * ble_baseline + 0.005f * filtered;
        float norm = filtered - ble_baseline;
        float qf = norm * BLE_ECG_SCALE;
        int32_t q = (int32_t)(qf + (qf >= 0.0f ? 0.5f : -0.5f));
        int32_t enc = (int32_t)BLE_ECG_OFFSET + q;
        if (enc < 0) enc = 0;
        if (enc > 4095) enc = 4095;
        g_ble_ecg_sample = (uint16_t)enc;
    }
    g_data_ready = 1;
    
    /* 4. 串口发送移至主循环，避免阻塞采样中断 */
    
    /* 5. R波检测与心率更新 */
    static uint8_t peak_blink = 0;
    if (ecg_detector_process(&g_detector, filtered)) {
        const r_peak_info_t* peak = ecg_detector_get_last_peak(&g_detector);
        if (peak != NULL) {
            if (fabsf(peak->amplitude) >= 0.02f) {
                uint8_t updated = ecg_hr_add_peak(&g_hr, peak);
                if (updated && g_hr.result.last_rr != prev_last_rr) {
                    prev_last_rr = g_hr.result.last_rr;
                    g_peak_flag = 1;
                    last_hr_update_ms = g_sys_ms;
                    peak_blink = 10;
                }
            }
        }
    }

    if (adc_val < 30 || adc_val > 4065) {
        if (rail_cnt < 5000) rail_cnt++;
    } else {
        if (rail_cnt > 0) rail_cnt--;
    }

    if (g_sys_ms >= 3000) {
        if (last_hr_update_ms == 0) {
            if (g_sys_ms >= 6500) {
                g_hr.result.status = HR_NO_SIGNAL;
                g_hr.result.average = 0;
                g_hr.result.instant = 0;
            }
        } else {
            if ((uint32_t)(g_sys_ms - last_hr_update_ms) >= 2500) {
                g_hr.result.status = HR_NO_SIGNAL;
                g_hr.result.average = 0;
                g_hr.result.instant = 0;
            }
        }
        if (rail_cnt >= 200) {
            g_hr.result.status = HR_NO_SIGNAL;
            g_hr.result.average = 0;
            g_hr.result.instant = 0;
        }
    }

    if (g_hr.result.status == HR_NO_SIGNAL) {
        buzzer_update(1);
    } else if (g_hr.result.status == HR_TACHYCARDIA || g_hr.result.status == HR_BRADYCARDIA) {
        buzzer_update(2);
    } else {
        buzzer_update(0);
    }
    
    /* 6. 更新 OLED 显存 (降采样以匹配 128 像素宽度显示 1 秒数据) */
    static uint8_t draw_div = 0;
    
    if (g_hr.result.status != HR_NO_SIGNAL && g_display_mode != OLED_MODE_LARGE_HR) {
        if (++draw_div >= 4) { // 每 4 个采样点绘制一次 (125Hz)
            draw_div = 0;
            OLED_DrawECGFull(filtered);
            
            // 在右上角画一个闪烁的点作为检测指示器
            if (peak_blink > 0) {
                OLED_DrawPoint(127, 0, 1);
                peak_blink--;
            }
        }
    }
}

/**
 * @brief TIM2中断处理
 */
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        
        g_sys_ms += SAMPLE_PERIOD;
        
        /* 执行核心采样逻辑 */
        timer_process_sampling();
        
        /* 执行其它回调 */
        if (g_timer_cb != NULL) {
            g_timer_cb();
        }
    }
}

/**
 * @brief 初始化定时器
 * 
 * 配置为500Hz (2ms周期)
 * 72MHz / 144 / 1000 = 500Hz
 */
void timer_init(void) {
    TIM_TimeBaseInitTypeDef tim_cfg;
    NVIC_InitTypeDef nvic_cfg;
    GPIO_InitTypeDef gpio_cfg;
    
    /* 使能时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio_cfg.GPIO_Pin = GPIO_Pin_13;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio_cfg);
    buzzer_write(0);
    
    /* 配置TIM2 */
    tim_cfg.TIM_Period = 1000 - 1;      /* 计数周期 */
    tim_cfg.TIM_Prescaler = 144 - 1;    /* 预分频 */
    tim_cfg.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_cfg.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tim_cfg);
    
    /* 使能中断 */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    /* 配置NVIC */
    nvic_cfg.NVIC_IRQChannel = TIM2_IRQn;
    nvic_cfg.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_cfg.NVIC_IRQChannelSubPriority = 0;
    nvic_cfg.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_cfg);
}

/**
 * @brief 启动定时器
 */
void timer_start(void) {
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief 停止定时器
 */
void timer_stop(void) {
    TIM_Cmd(TIM2, DISABLE);
}

/**
 * @brief 设置回调函数
 */
void timer_set_callback(timer_callback_t cb) {
    g_timer_cb = cb;
}

/**
 * @brief 获取系统运行时间
 */
uint32_t timer_get_ms(void) {
    return g_sys_ms;
}
