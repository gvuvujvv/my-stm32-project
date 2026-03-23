/**
 * @file uart.h
 * @brief UART驱动头文件
 * 
 * 使用USART1, PA9(TX), PA10(RX)
 * 波特率: 115200
 */

#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
#include <stdint.h>

/* 波特率 */
#ifndef UART_BAUDRATE
#define UART_BAUDRATE   115200
#endif

/* 初始化 */
void uart_init(void);
void uart_set_baudrate(uint32_t baudrate);

/* 发送单字节 */
void uart_send_byte(uint8_t data);

/* 发送字符串 */
void uart_send_string(const char *str);
void uart_send_bytes(const uint8_t *data, uint16_t len);

void UART_SendBLEPacket(uint16_t data);

/* 发送数字 (带换行) */
void uart_send_number(int32_t num);

/* 发送无符号16位数字 (不带换行，用于连续输出) */
void uart_send_number_u16(uint16_t num);

/* 发送ADC值 (格式: "value\r\n") */
void uart_send_adc(uint16_t adc_val);

/* 发送浮点数 (格式: "value\r\n", 保留3位小数) */
void uart_send_float(float value);

#endif /* __UART_H */
