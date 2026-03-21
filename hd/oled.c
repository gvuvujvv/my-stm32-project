#include "oled.h"
#include "oledfont.h"
#include "system.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// I2C Slave Address
#define OLED_ADDRESS 0x78

// OLED GDDRAM Buffer
uint8_t OLED_GRAM[OLED_PAGES][OLED_WIDTH];

OLED_DisplayMode_t g_display_mode = OLED_MODE_FULL_WAVE;

/* ECG Drawing State Variables */
static uint8_t ecg_ptr = 0;
static uint8_t ecg_last_y = 48;
static uint8_t ecg_full_last_y = 32;
static float ecg_baseline = 0.0f;
static float ecg_prev_val = 0.0f;
static uint8_t ecg_waiting_trigger = 0;
static uint32_t ecg_settle_count = 0;

/* I2C IO Definitions */
#define OLED_SCL_PIN  GPIO_Pin_8
#define OLED_SDA_PIN  GPIO_Pin_9
#define OLED_I2C_PORT GPIOB

/* I2C GPIO Macros (Safe bitwise logic) */
#define OLED_W_SCL(x) GPIO_WriteBit(OLED_I2C_PORT, OLED_SCL_PIN, (BitAction)(!!(x)))
#define OLED_W_SDA(x) GPIO_WriteBit(OLED_I2C_PORT, OLED_SDA_PIN, (BitAction)(!!(x)))

/**
 * @brief I2C Short Delay for 72MHz bit-banging (~200kHz)
 */
static void OLED_I2C_Delay(void) {
    volatile uint32_t i = 20; // Increased for better compatibility
    while(i--);
}

/**
 * @brief Initialize I2C pins for PB8, PB9
 */
void OLED_I2C_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN | OLED_SDA_PIN;
    GPIO_Init(OLED_I2C_PORT, &GPIO_InitStructure);
    
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/**
 * @brief I2C Start condition
 */
void OLED_I2C_Start(void) {
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(0);
}

/**
 * @brief I2C Stop condition
 */
void OLED_I2C_Stop(void) {
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(1);
    OLED_I2C_Delay();
}

/**
 * @brief I2C Send byte (No ACK check)
 */
void OLED_I2C_SendByte(uint8_t Byte) {
    uint8_t i;
    __disable_irq(); // Protect timing from 500Hz IRQ
    for (i = 0; i < 8; i++) {
        OLED_W_SDA(Byte & (0x80 >> i));
        OLED_I2C_Delay();
        OLED_W_SCL(1);
        OLED_I2C_Delay();
        OLED_W_SCL(0);
        OLED_I2C_Delay();
    }
    // Release SDA for ACK slot
    OLED_W_SDA(1);
    OLED_I2C_Delay();
    OLED_W_SCL(1); // ACK clock pulse
    OLED_I2C_Delay();
    OLED_W_SCL(0);
    OLED_I2C_Delay();
    __enable_irq();
}

/**
 * @brief Write a command to OLED
 */
void OLED_WriteCommand(uint8_t Command) {
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDRESS);
    OLED_I2C_SendByte(0x00); // Command mode
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/**
 * @brief Write data to OLED
 */
void OLED_WriteData(uint8_t Data) {
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDRESS);
    OLED_I2C_SendByte(0x40); // Data mode
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/**
 * @brief Set GDDRAM cursor
 */
void OLED_SetCursor(uint8_t Y, uint8_t X) {
    OLED_WriteCommand(0xB0 | Y); // Set Page (0-7)
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4)); // Set Column High 4 bits
    OLED_WriteCommand(0x00 | (X & 0x0F)); // Set Column Low 4 bits
}

/**
 * @brief Refresh specific area of OLED
 */
static void OLED_RefreshArea(uint8_t start_page, uint8_t end_page) {
    uint8_t i, j;
    for (i = start_page; i <= end_page && i < OLED_PAGES; i++) {
        OLED_SetCursor(i, 0);
        OLED_I2C_Start();
        OLED_I2C_SendByte(OLED_ADDRESS);
        OLED_I2C_SendByte(0x40);
        for (j = 0; j < OLED_WIDTH; j++) {
            OLED_I2C_SendByte(OLED_GRAM[i][j]);
        }
        OLED_I2C_Stop();
    }
}

/**
 * @brief Refresh OLED with GRAM content
 */
void OLED_Refresh(void) {
    OLED_RefreshArea(0, 7);
}

/**
 * @brief Refresh only the waveform area (Pages 4-7)
 */
void OLED_RefreshWaveform(void) {
    OLED_RefreshArea(4, 7);
}

/**
 * @brief Refresh only the text area (Pages 0-3)
 */
void OLED_RefreshText(void) {
    OLED_RefreshArea(0, 3);
}

/**
 * @brief Reset Waveform Drawing state
 */
void OLED_ResetWaveform(void) {
    ecg_ptr = 0;
    ecg_full_last_y = 32;
    ecg_waiting_trigger = 1;
    ecg_settle_count = 0;
}

/**
 * @brief Clear GRAM
 */
void OLED_Clear(void) {
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/**
 * @brief Draw a point in GRAM
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (color) {
        OLED_GRAM[y / 8][x] |= (0x01 << (y % 8));
    } else {
        OLED_GRAM[y / 8][x] &= ~(0x01 << (y % 8));
    }
}

/**
 * @brief Initialize OLED SSD1306
 */
void OLED_Init(void) {
    OLED_I2C_Init();
    sys_delay_ms(200); // Wait for power stable
    
    OLED_WriteCommand(0xAE); // 关闭显示
    OLED_WriteCommand(0xD5); // 设置时钟分频/振荡频率
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8); // 设置多路复用率
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3); // 设置显示偏移
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40); // 设置显示开始行
    
    OLED_WriteCommand(0x20); // 设置存储模式
    OLED_WriteCommand(0x02); // 页寻址模式 (Page Addressing Mode)
    
    OLED_WriteCommand(0xA1); // 设置段重映射
    OLED_WriteCommand(0xC8); // 设置COM输出扫描方向
    OLED_WriteCommand(0xDA); // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81); // 设置对比度控制
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9); // 设置预充电周期
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB); // 设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4); // 全局显示开启
    OLED_WriteCommand(0xA6); // 设置正常/反向显示
    OLED_WriteCommand(0x8D); // 使能电荷泵
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF); // 开启显示
    
    OLED_Clear();
    OLED_Refresh();
}

/**
 * @brief Display a character (8x16)
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char) {
    uint8_t i;
    uint8_t Page = (Line - 1) * 2;
    uint8_t Col = (Column - 1) * 8;
    for (i = 0; i < 8; i++) {
        OLED_GRAM[Page][Col + i] = OLED_F8x16[Char - ' '][i];
        OLED_GRAM[Page + 1][Col + i] = OLED_F8x16[Char - ' '][i + 8];
    }
}

/**
 * @brief Display a string (8x16)
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String) {
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++) {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
 * @brief Display a character (6x8)
 */
void OLED_ShowChar6x8(uint8_t Line, uint8_t Column, char Char) {
    uint8_t i;
    uint8_t Page = (Line - 1);
    uint8_t Col = (Column - 1) * 8;
    for (i = 0; i < 6; i++) {
        OLED_GRAM[Page][Col + i] = OLED_F6x8[Char - ' '][i];
    }
}

/**
 * @brief Display a string (6x8)
 */
void OLED_ShowString6x8(uint8_t Line, uint8_t Column, const char *String) {
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++) {
        OLED_ShowChar6x8(Line, Column + i, String[i]);
    }
}

/**
 * @brief Display a number
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length) {
    char buf[12];
    sprintf(buf, "%0*lu", Length, Number);
    OLED_ShowString(Line, Column, buf);
}

/**
 * @brief Display a signed number
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length) {
    char buf[12];
    sprintf(buf, "%+0*ld", Length, Number);
    OLED_ShowString(Line, Column, buf);
}

/**
 * @brief Display a float number
 */
void OLED_ShowFNum(uint8_t Line, uint8_t Column, float Number, uint8_t Length, uint8_t FLength) {
    char buf[20];
    sprintf(buf, "%0*.*f", Length, FLength, Number);
    OLED_ShowString(Line, Column, buf);
}

/**
 * @brief Display a Chinese character (16x16)
 */
void OLED_ShowCC_F16x16(uint8_t Line, uint8_t Column, uint8_t num) {
    uint8_t i;
    uint8_t Page = (Line - 1) * 2;
    uint8_t Col = (Column - 1) * 16;
    for (i = 0; i < 16; i++) {
        OLED_GRAM[Page][Col + i] = CD_F16x16[num][i];
        OLED_GRAM[Page + 1][Col + i] = CD_F16x16[num][i + 16];
    }
}

/**
 * @brief ECG Waveform Display (Optimized for continuity)
 */
void OLED_DrawECG(float ecg_val) {
    // Waveform area: Pages 4-7 (Y: 32-63, Height: 32)
    // Filtered ECG is zero-centered due to high-pass filter.
    // Center baseline at Y=48.
    // AD8232 gain is ~1100, so 1mV signal is ~1.1V.
    // Let's use a scale that maps 1V to ~15 pixels.
    int y = (int)(48 - ecg_val * 20.0f); 
    if (y < 32) y = 32;
    if (y > 63) y = 63;
    
    // Clear the current column in the waveform area (Pages 4-7)
    uint8_t i;
    for (i = 4; i < 8; i++) {
        OLED_GRAM[i][ecg_ptr] = 0; 
    }
    
    // Draw vertical line from last_y to current y to ensure continuity
    int y0 = ecg_last_y;
    int y1 = y;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int v = y0; v <= y1; v++) {
        OLED_DrawPoint(ecg_ptr, v, 1);
    }
    
    ecg_last_y = y;
    
    // Increment pointer for next sample
    ecg_ptr = (ecg_ptr + 1) % 128;
    
    // Clear a few columns ahead to create a "trailing" effect (optional but looks better)
    uint8_t clear_ptr = (ecg_ptr + 1) % 128;
    for (i = 4; i < 8; i++) OLED_GRAM[i][clear_ptr] = 0;
    clear_ptr = (ecg_ptr + 2) % 128;
    for (i = 4; i < 8; i++) OLED_GRAM[i][clear_ptr] = 0;
}

/**
 * @brief ECG Waveform Display (Full Screen with Trigger)
 */
void OLED_DrawECGFull(float ecg_val) {
    // 1. Filter Settle Period: Wait 1s (500 samples / 4 draw calls = 125 draws)
    if (ecg_settle_count < 125) {
        ecg_settle_count++;
        ecg_baseline = ecg_val; // Fast settle
        return;
    }

    // 2. Auto-baseline tracking
    ecg_baseline = 0.99f * ecg_baseline + 0.01f * ecg_val;
    float normalized = ecg_val - ecg_baseline;
    
    // 3. Trigger Logic: When at start of screen, wait for rising edge
    if (ecg_ptr == 0) {
        ecg_waiting_trigger = 1;
    }
    
    if (ecg_waiting_trigger) {
        // Trigger condition: rising edge crossing 0.2V (QRS start)
        if (normalized > 0.2f && (ecg_val - ecg_prev_val) > 0.05f) {
            ecg_waiting_trigger = 0; // Triggered!
        } else {
            ecg_prev_val = ecg_val;
            return; // Stay at X=0
        }
    }

    // 4. Calculate Y coordinate
    int y = (int)(32 - normalized * 30.0f); 
    if (y < 0) y = 0;
    if (y > 63) y = 63;
    
    // 5. Clear and Draw (Scanline effect)
    uint8_t i;
    for (i = 0; i < 8; i++) {
        OLED_GRAM[i][ecg_ptr] = 0; 
    }
    
    int y0 = ecg_full_last_y;
    int y1 = y;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int v = y0; v <= y1; v++) {
        OLED_DrawPoint(ecg_ptr, v, 1);
    }
    
    ecg_full_last_y = (uint8_t)y;
    ecg_prev_val = ecg_val;
    
    // 6. Increment and Trailing
    ecg_ptr = (ecg_ptr + 1) % 128;
    
    uint8_t clear_ptr = (ecg_ptr + 1) % 128;
    for (i = 0; i < 8; i++) OLED_GRAM[i][clear_ptr] = 0;
    clear_ptr = (ecg_ptr + 2) % 128;
    for (i = 0; i < 8; i++) OLED_GRAM[i][clear_ptr] = 0;
}
