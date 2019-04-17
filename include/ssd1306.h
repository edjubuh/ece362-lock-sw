#ifndef _SSD1306_H_
#define _SSD1306_H_

#include "common.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  8
#define SSD1306_SIZE    (SSD1306_WIDTH * SSD1306_HEIGHT)

struct ssd1306_bounds {
    uint8_t col_start;
    uint8_t col_end;
    uint8_t page_start;
    uint8_t page_end;

    size_t size;
};

void ssd1306_init(void);
void ssd1306_reset();

void ssd1306_sleep();
void ssd1306_wake();

void ssd1306_fill(uint8_t const * const buf, const struct ssd1306_bounds bounds);
void ssd1306_fills(uint8_t const * const buf);

#endif