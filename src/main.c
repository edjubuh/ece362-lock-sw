/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include "stm32f0_discovery.h"

#include "common.h"
#include "numpad.h"
#include "rfid/cr95hf.h"
#include "rfid/iso14443a.h"
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

int main(void)
{
  systick_init();
  ssd1306_init();
  cr95hf_init();
  numpad_init();

  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);

  // struct cr95hf_idn_rx idn;
  // cr95hf_idn(&idn);
  // if(idn.header.code) {
  //   STM_EVAL_LEDOn(LED4);
  // }
  // millis_wait(50);

  uint8_t resp = iso14443a_proto_select();
  if(resp) {
    STM_EVAL_LEDOn(LED4);
  }

  // uint8_t data[SSD1306_SIZE];
  // uint8_t color = 0;
  // for (size_t i = 0; i < SSD1306_SIZE; i++)
  // {
  //   data[i] = color;
  //   color = ~(~color + 1);
  // }
  // ssd1306_fills(data);

  struct cr95hf_idle_tx idle_setting = {
    .wu_src = WU_TAG_DETECT,
    .enter = ENTER_TAG_DETECTOR,
    .wu_control = WU_TAG_DETECTOR,
    .leave_control = LEAVE_TAG_DETECTOR,
    .wu_period = 0x20, // default value
    .osc_start = 0x60,
    .dac_start = 0x60,
    .dac_data = cr95hf_calibrate_tag_detection(),
    .swing_count = 0x3F, // recommended value
    .max_sleep = 0 // unused
  };
  cr95hf_idle(&idle_setting);

  for (;;)
  {
    STM_EVAL_LEDToggle(LED3);
    millis_wait(500);
    if(cr95hf_is_awake(NULL) == 0x00) {
      iso14443a_proto_select();
      if (iso14443a_tag_present())
      {
        STM_EVAL_LEDOn(LED4);
        struct iso14443a_uid uid;
        uint8_t sak = iso14443a_get_uid(&uid);
        asm("nop");
        millis_wait(20);
      }
      STM_EVAL_LEDOff(LED4);
      cr95hf_idle(&idle_setting);
    }

    // ssd1306_sleep();
    // millis_wait(500);
    // ssd1306_wake();
  }
}
