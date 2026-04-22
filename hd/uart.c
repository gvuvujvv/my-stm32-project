/**
 * @file uart.c
 * @brief UART驱动实现
 */

#include "uart.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 初始化UART
 */
void uart_init(void) {
    GPIO_InitTypeDef gpio_cfg;
    USART_InitTypeDef usart_cfg;
    
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
                           RCC_APB2Periph_AFIO | 
                           RCC_APB2Periph_USART1, ENABLE);
    
    /* PA9 - TX, 复用推挽输出 */
    gpio_cfg.GPIO_Pin = GPIO_Pin_9;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_cfg);
    
    /* PA10 - RX, 浮空输入 */
    gpio_cfg.GPIO_Pin = GPIO_Pin_10;
    gpio_cfg.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_cfg);
    
    /* 配置USART1 */
    usart_cfg.USART_BaudRate = UART_BAUDRATE;
    usart_cfg.USART_WordLength = USART_WordLength_8b;
    usart_cfg.USART_StopBits = USART_StopBits_1;
    usart_cfg.USART_Parity = USART_Parity_No;
    usart_cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_cfg.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &usart_cfg);
    
    /* 使能USART */
    USART_Cmd(USART1, ENABLE);
}

void uart_set_baudrate(uint32_t baudrate) {
    USART_InitTypeDef usart_cfg;
    usart_cfg.USART_BaudRate = baudrate;
    usart_cfg.USART_WordLength = USART_WordLength_8b;
    usart_cfg.USART_StopBits = USART_StopBits_1;
    usart_cfg.USART_Parity = USART_Parity_No;
    usart_cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_cfg.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Cmd(USART1, DISABLE);
    USART_Init(USART1, &usart_cfg);
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief 发送单字节
 */
void uart_send_byte(uint8_t data) {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, data);
}

/**
 * @brief 发送字符串
 */
void uart_send_string(const char *str) {
    while (*str) {
        uart_send_byte(*str++);
    }
}

void uart_send_bytes(const uint8_t *data, uint16_t len) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        uart_send_byte(data[i]);
    }
}

void UART_SendBLEPacket(uint16_t data) {
    uint8_t pkt[5];
    uint16_t v = (uint16_t)(data & 0x0FFF);

    pkt[0] = 0xAA;
    pkt[1] = 0x01;
    pkt[2] = (uint8_t)((v >> 8) & 0x0F);
    pkt[3] = (uint8_t)(v & 0xFF);
    pkt[4] = (uint8_t)(pkt[0] ^ pkt[1] ^ pkt[2] ^ pkt[3]);

    uart_send_bytes(pkt, 5);
}

void uart_send_ble_packet(uint16_t data) {
    UART_SendBLEPacket(data);
}

static uint8_t clamp_u8(uint16_t value) {
    return value > 255 ? 255 : (uint8_t)value;
}

void uart_send_hrv_packet(const hr_result_t *hr) {
    uint8_t pkt[9];
    uint8_t status;
    uint8_t i;
    uint8_t checksum = 0;

    if (hr->status == HR_NO_SIGNAL) {
        status = 3;
    } else if (!hr->hrv_ready) {
        status = 4;
    } else {
        status = (uint8_t)hr->status;
    }

    pkt[0] = 0xAA;
    pkt[1] = 0x02;
    pkt[2] = clamp_u8(hr->average);
    pkt[3] = clamp_u8(hr->sdnn);
    pkt[4] = clamp_u8(hr->rmssd);
    pkt[5] = hr->pnn50 > 100 ? 100 : hr->pnn50;
    pkt[6] = hr->hrv_count;
    pkt[7] = status;

    for (i = 0; i < 8; i++) {
        checksum ^= pkt[i];
    }
    pkt[8] = checksum;

    uart_send_bytes(pkt, 9);
}

/**
 * @brief 发送数字 (带换行，int32_t)
 */
void uart_send_number(int32_t num) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", num);
    uart_send_string(buf);
}

/**
 * @brief 发送无符号16位数字 (不带换行)
 */
void uart_send_number_u16(uint16_t num) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", num);
    uart_send_string(buf);
}

/**
 * @brief 发送ADC值 (格式: "value\r\n")
 */
void uart_send_adc(uint16_t adc_val) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d\r\n", adc_val);
    uart_send_string(buf);
}

/**
 * @brief 发送浮点数 (格式: "value\r\n", 保留3位小数)
 * @param value 浮点数值 (单位: mV)
 */
void uart_send_float(float value) {
    char buf[16];
    int32_t integer_part;
    int32_t decimal_part;
    float abs_val;
    char *p = buf;
    
    /* 处理负数 */
    if (value < 0) {
        *p++ = '-';
        abs_val = -value;
    } else {
        abs_val = value;
    }
    
    /* 分离整数和小数部分 (保留3位小数) */
    integer_part = (int32_t)abs_val;
    decimal_part = (int32_t)((abs_val - integer_part) * 1000 + 0.5f);
    if (decimal_part >= 1000) {
        integer_part += 1;
        decimal_part -= 1000;
    }
    
    /* 格式化输出 */
    snprintf(p, sizeof(buf) - (p - buf), "%ld.%03ld\r\n", integer_part, decimal_part);
    uart_send_string(buf);
}
