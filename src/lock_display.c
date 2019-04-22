#include "lock.h"

#include "ssd1306.h"

const static uint8_t UPPER_QUARTER_THICK[24 * 3] = {
//          0     1     2     3     4    5     6     7     8     9     10    11    12    13    14    15    16    17    18    19    20    21    22    23
/*00-07*/ 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x7e, 0x7e, 0x7e, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf0, 0xe0, 0xe0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*08-15*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x1f, 0x3f, 0xff, 0xff, 0xff, 0xfc, 0xf0, 0xe0, 0x80, 0x00,
/*15-23*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0
};

const static uint8_t LOWER_QUARTER_THICK[24 * 3] = {
//         0     1     2     3     4    5     6     7     8     9     10    11    12    13    14    15    16    17    18    19    20    21    22    23
/*15-23*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x0f,
/*08-15*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf8, 0xfc, 0xff, 0xff, 0xff, 0x3f, 0x0f, 0x07, 0x01, 0x00,
/*00-07*/ 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x7e, 0x7e, 0x7e, 0x7f, 0x3f, 0x3f, 0x1f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const static uint8_t UPPER_QUARTER_THIN[24 * 3] = {
//          0     1     2     3     4    5     6     7     8     9     10    11    12    13    14    15    16    17    18    19    20    21    22    23
/*00-07*/ 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60, 0xc0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*08-15*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0e, 0x3c, 0xf0, 0xe0, 0x80, 0x00, 0x00, 0x00,
/*15-23*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0xff, 0xfc, 0x00, 0x00
};

const static uint8_t LOWER_QUARTER_THIN[24 * 3] = {
//         0     1     2     3     4    5     6     7     8     9     10    11    12    13    14    15    16    17    18    19    20    21    22    23
/*15-23*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xf0, 0xff, 0x3f, 0x00, 0x00,
/*08-15*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xE0, 0x70, 0x3c, 0x0f, 0x03, 0x01, 0x00, 0x00, 0x00,
/*00-07*/ 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x38, 0x38, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const static struct ssd1306_bounds UL_DISP_BOUNDS = {
  .col_start = (SSD1306_WIDTH / 2),
  .col_end = (SSD1306_WIDTH / 2) + 24,
  .page_start = 1,
  .page_end = 1 + 3
};
const static struct ssd1306_buf_bounds UL_BUFF_BOUNDS = {
  .n_rows = 3,
  .n_cols = 24,
  .row_len = 24,
  .col_wid = 1
};

struct ssd1306_bounds UR_DISP_BOUNDS = {
  .col_start = (SSD1306_WIDTH / 2) - 24,
  .col_end = (SSD1306_WIDTH / 2),
  .page_start = 1,
  .page_end = 1 + 3
};
struct ssd1306_buf_bounds UR_BUFF_BOUNDS = {
  .n_rows = 3,
  .n_cols = 24,
  .row_len = 24,
  .col_wid = -1
};

struct ssd1306_bounds LR_DISP_BOUNDS = {
  .col_start = (SSD1306_WIDTH / 2) - 24,
  .col_end = (SSD1306_WIDTH / 2),
  .page_start = 4,
  .page_end = 4 + 3
};
struct ssd1306_buf_bounds LR_BUFF_BOUNDS = {
  .n_rows = 3,
  .n_cols = 24,
  .row_len = 24,
  .col_wid = -1
};

struct ssd1306_bounds LL_DISP_BOUNDS = {
  .col_start = (SSD1306_WIDTH / 2),
  .col_end = (SSD1306_WIDTH / 2)+24,
  .page_start = 4,
  .page_end = 4 + 3
};
struct ssd1306_buf_bounds LL_BUFF_BOUNDS = {
  .n_rows = 3,
  .n_cols = 24,
  .row_len = 24,
  .col_wid = 1
};

void display_filled(uint8_t count) {
  uint8_t const * buf = UPPER_QUARTER_THIN;
  if(count > 0) {
    buf = UPPER_QUARTER_THICK;
  } else {
    buf = UPPER_QUARTER_THIN;
  }
  ssd1306_fill(buf, UL_DISP_BOUNDS, UL_BUFF_BOUNDS);

  if(count > 1) {
    buf = LOWER_QUARTER_THICK;
  } else {
    buf = LOWER_QUARTER_THIN;
  }
  ssd1306_fill(buf, LL_DISP_BOUNDS, LL_BUFF_BOUNDS);

  if(count > 2) {
    buf = LOWER_QUARTER_THICK;
  } else {
    buf = LOWER_QUARTER_THIN;
  }
  ssd1306_fill((uint8_t const *)buf + 23, LR_DISP_BOUNDS, LR_BUFF_BOUNDS);

  if(count > 3) {
    buf = UPPER_QUARTER_THICK;
  } else {
    buf = UPPER_QUARTER_THIN;
  }
  ssd1306_fill((uint8_t const *)buf + 23, UR_DISP_BOUNDS, UR_BUFF_BOUNDS);
}