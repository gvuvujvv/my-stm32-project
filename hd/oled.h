#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

#define OLED_WIDTH   128 
#define OLED_HEIGHT  64 
#define OLED_PAGES   8 

extern uint8_t OLED_GRAM[OLED_PAGES][OLED_WIDTH];

// I2C基础 
void OLED_I2C_Init(void);
void OLED_I2C_Start(void); 
void OLED_I2C_Stop(void); 
void OLED_I2C_SendByte(uint8_t Byte); 
 
// OLED操作 
void OLED_Init(void);
void OLED_WriteCommand(uint8_t Command); 
void OLED_WriteData(uint8_t Data); 
void OLED_SetCursor(uint8_t Y, uint8_t X);  // Y:页0-7, X:列0-127 
void OLED_Refresh(void);  // 将OLED_GRAM刷新到屏幕 
void OLED_RefreshWaveform(void); // 只刷新波形区域 (Pages 4-7)
void OLED_RefreshText(void); // 只刷新文字区域 (Pages 0-3)
void OLED_ResetWaveform(void); // 重置波形绘制状态
 
// 绘图 
void OLED_Clear(void);  // 清屏 
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color);  // 0=灭, 1=亮 
 
// 字符显示（行号1-4，列号1-16，基于8x16字体） 
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char); 
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String); 
 
// 6x8小字体（行号1-8） 
void OLED_ShowChar6x8(uint8_t Line, uint8_t Column, char Char); 
void OLED_ShowString6x8(uint8_t Line, uint8_t Column, const char *String); 
 
// 数字显示 
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length); 
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length); 
void OLED_ShowFNum(uint8_t Line, uint8_t Column, float Number, uint8_t Length, uint8_t FLength); 
 
// 中文16x16（Line 1-4, Column 1-8） 
void OLED_ShowCC_F16x16(uint8_t Line, uint8_t Column, uint8_t num);

// ECG 波形显示
void OLED_DrawECG(float ecg_val);
void OLED_DrawECGFull(float ecg_val); // 全屏波形绘制

typedef enum {
    OLED_MODE_FULL_WAVE = 0, // 全屏波形 (Default)
    OLED_MODE_WAVE_HR,      // 波形 + 右上角心率
    OLED_MODE_LARGE_HR,     // 大字心率显示
    OLED_MODE_MAX
} OLED_DisplayMode_t;

extern OLED_DisplayMode_t g_display_mode;

#endif /* __OLED_H */
