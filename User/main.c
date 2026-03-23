/**
 * @file main.c
 * @brief 心电信号采集主程序
 * 
 * 功能: 500Hz采样AD8232输出,通过串口发送滤波后数据及心率信息
 * 
 * 硬件连接:
 * - PA5: AD8232 OUTPUT (ADC输入)
 * - PA9: USART1 TX (USB转TTL RX)
 * - PA10: USART1 RX (USB转TTL TX)
 */

#include "stm32f10x.h"
#include "system.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "ecg_filter.h"
#include "ecg_detector.h"
#include "ecg_hr.h"
#include "oled.h"
#include "key.h"

/* 全局变量 */
volatile uint8_t g_peak_flag = 0; // 新增：R波检测标志
ecg_filter_t g_filter; 
ecg_detector_t g_detector;
ecg_hr_t g_hr;
static uint32_t g_refresh_counter = 0;

/**
 * @brief 发送启动信息
 */
static void send_startup_info(void) {
    uart_send_string("\r\n");
    uart_send_string("================================\r\n");
    uart_send_string("  ECG Data Acquisition System   \r\n");
    uart_send_string("================================\r\n");
    uart_send_string("Sample Rate: 500Hz\r\n");
    uart_send_string("ADC Channel: PA5 (AD8232)\r\n");
    uart_send_string("Filter: HP(0.5Hz)->LP(40Hz)->Notch(50Hz)\r\n");
    uart_send_string("Detector: Pan-Tompkins Algorithm\r\n");
    uart_send_string("UART: 115200 baud\r\n");
    uart_send_string("Data Format: ECG(mV),[HR Info]\r\n");
    uart_send_string("================================\r\n\r\n");
}

/**
 * @brief 发送心率信息
 */
static void send_hr_info(const hr_result_t* hr) {
    uart_send_string("[HR] ");
    uart_send_string("Instant:");
    uart_send_number(hr->instant);
    uart_send_string(" Avg:");
    uart_send_number(hr->average);
    uart_send_string(" Min:");
    uart_send_number(hr->min_hr);
    uart_send_string(" Max:");
    uart_send_number(hr->max_hr);
    uart_send_string(" Status:");
    uart_send_string(ecg_hr_status_string(hr->status));
    uart_send_string("\r\n");
}

/**
 * @brief 主函数
 */
int main(void) {
    /* 系统初始化 */
    sys_init();
    
    /* 外设初始化 */
    uart_init();
    adc_init();
    timer_init();
    OLED_Init();
    Key_Init();
    
    /* 初始化信号处理模块 */
    ecg_filter_init(&g_filter);
    ecg_detector_init(&g_detector);
    ecg_hr_init(&g_hr);
    
    /* OLED 初始显示 */
    OLED_Clear();
    OLED_Refresh();
    
    /* 发送启动信息 */
    send_startup_info();
    
    /* 启动定时器 */
    timer_start();
    
    /* 主循环 */
    while (1) {
        /* BLE 二进制透传发送：主循环中发送，避免阻塞 500Hz 采样中断
         *
         * 当前发送的是“滤波后信号”的 12bit 定点映射值（不是原始 ADC）：
         * - 单片机端：ecg12 = 2048 + round((filtered-baseline)*1024)，并裁剪到 0..4095
         * - 小程序端：signed = ecg12 - 2048；normalized(V) ≈ signed / 1024
         *
         * 微信小程序端解析示例（ArrayBuffer -> 12位 ecg12）：
         * wx.onBLECharacteristicValueChange(res => {
         *   const u8 = new Uint8Array(res.value)
         *   for (let i = 0; i + 4 < u8.length; i += 5) {
         *     if (u8[i] !== 0xAA) continue
         *     const type = u8[i + 1]
         *     const dataH = u8[i + 2] & 0x0F
         *     const dataL = u8[i + 3]
         *     const cs = u8[i + 4]
         *     const calc = (u8[i] ^ u8[i + 1] ^ u8[i + 2] ^ u8[i + 3]) & 0xFF
         *     if (cs !== calc) continue
         *     if (type !== 0x01) continue
         *     const ecg12 = (dataH << 8) | dataL
         *     const signed = ecg12 - 2048
         *     const v = signed / 1024.0
         *   }
         * })
         */
        if (g_data_ready) {
            uint16_t sample = g_ble_ecg_sample;
            g_data_ready = 0;
            UART_SendBLEPacket(sample);
        }

        /* 检查是否有新的心率数据需要通过串口发送 */
        if (g_peak_flag) {
            g_peak_flag = 0;
            hr_result_t hr;
            ecg_hr_get_result_snapshot(&g_hr, &hr);
            send_hr_info(&hr);
        }
        
        /* 处理按键 */
        uint8_t key = Key_GetNum();
        if (key == 1) {
            // 按键 1: 切换显示模式
            g_display_mode = (OLED_DisplayMode_t)((g_display_mode + 1) % OLED_MODE_MAX);
            OLED_Clear();
            OLED_ResetWaveform(); // 切换模式时重置波形
            OLED_Refresh();
        } else if (key == 2) {
            // 按键 2: 重置心率统计
            ecg_hr_init(&g_hr);
            OLED_Clear();
            OLED_ResetWaveform(); // 重置绘制
            OLED_ShowString(2, 1, "HR Reset");
            OLED_Refresh();
            sys_delay_ms(500);
            OLED_Clear();
            OLED_Refresh();
        }
        
        /* 定期刷新显示 (20Hz) */
        if (++g_refresh_counter >= 25) {
            g_refresh_counter = 0;
            hr_result_t hr;
            ecg_hr_get_result_snapshot(&g_hr, &hr);
            
            if (g_display_mode == OLED_MODE_FULL_WAVE) {
                if (hr.status == HR_NO_SIGNAL) {
                    OLED_Clear();
                    OLED_ResetWaveform();
                    OLED_ShowString(2, 5, "NO SIG");
                    OLED_ShowString(3, 2, "Check Electrode");
                    OLED_Refresh();
                } else {
                    OLED_Refresh();
                }
            } 
            else if (g_display_mode == OLED_MODE_WAVE_HR) {
                // 波形 + 右上角实时心率 + 状态
                OLED_ShowString(1, 1, "HR:");
                if (hr.status == HR_NO_SIGNAL) {
                    OLED_ShowString(1, 4, "---");
                } else {
                    OLED_ShowNum(1, 4, hr.average, 3);
                }
                OLED_ShowString(1, 8, ecg_hr_status_string(hr.status));
                OLED_Refresh();
            }
            else if (g_display_mode == OLED_MODE_LARGE_HR) {
                // 大字心率显示模式
                OLED_ShowString(2, 4, "Heart Rate");
                if (hr.status == HR_NO_SIGNAL) {
                    OLED_ShowString(3, 6, "---");
                } else {
                    OLED_ShowNum(3, 6, hr.average, 3);
                }
                OLED_ShowString(3, 10, "bpm");
                OLED_ShowString(4, 5, ecg_hr_status_string(hr.status));
                OLED_Refresh();
            }
        }
        
        sys_delay_ms(10); 
    }
}
