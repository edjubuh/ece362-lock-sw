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
#include "lock.h"
#include "numpad.h"
#include "rfid/cr95hf.h"
#include "rfid/iso14443a.h"
#include "ssd1306.h"


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
    asm("wfi");
    uint8_t chars[4];
    display_filled(get_pressed_keys(chars));
    // STM_EVAL_LEDToggle(LED3);
    // millis_wait(500);
    // if(cr95hf_is_awake(NULL) == 0x00) {
    //   iso14443a_proto_select();
    //   if (iso14443a_tag_present())
    //   {
    //     STM_EVAL_LEDOn(LED4);
    //     struct iso14443a_uid uid;
    //     uint8_t sak = iso14443a_get_uid(&uid);
    //     asm("nop");
    //     millis_wait(20);
    //   }
    //   STM_EVAL_LEDOff(LED4);
    //   cr95hf_idle(&idle_setting);
    // }

    // ssd1306_sleep();
    // millis_wait(500);
    // ssd1306_wake();
  }
}
