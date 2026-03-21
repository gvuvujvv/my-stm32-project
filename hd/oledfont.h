#ifndef __OLEDFONT_H
#define __OLEDFONT_H

#include "stm32f10x.h"
#include <stdint.h>

// ASCII 8x16 font (0x20-0x7F)
extern const uint8_t OLED_F8x16[95][16];

// ASCII 6x8 font (0x20-0x7F)
extern const uint8_t OLED_F6x8[95][6];

// Example Chinese 16x16 font
extern const uint8_t CD_F16x16[][32];

#endif /* __OLEDFONT_H */
