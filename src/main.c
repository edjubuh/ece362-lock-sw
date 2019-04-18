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
#include "ssd1306.h"
#include "rfid/cr95hf.h"
#include "rfid/iso14443a.h"

int main(void)
{
  systick_init();
  ssd1306_init();
  cr95hf_init();

  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);

  // struct cr95hf_idn_rx idn;
  // cr95hf_idn(&idn);
  // if(idn.header.code) {
  //   STM_EVAL_LEDOn(LED4);
  // }
  millis_wait(50);

  uint8_t resp = iso14443a_proto_select();
  if(resp) {
    STM_EVAL_LEDOn(LED4);
  }

  uint8_t data[SSD1306_SIZE];
  uint8_t color = 0;
  for (size_t i = 0; i < SSD1306_SIZE; i++)
  {
    data[i] = color;
    color = ~(~color + 1);
  }
  ssd1306_fills(data);

  for (;;)
  {
    STM_EVAL_LEDToggle(LED3);
    millis_wait(500);
    if(cr95hf_echo()) {
     STM_EVAL_LEDToggle(LED4);
    }
    if (iso14443a_tag_present())
    {
      STM_EVAL_LEDOn(LED4);
      struct iso14443a_uid uid;
      uint8_t sak = iso14443a_get_uid(&uid);
      asm("nop");
    } else {
      STM_EVAL_LEDOff(LED4);
    }

    ssd1306_sleep();
    millis_wait(500);
    ssd1306_wake();
  }
}
