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

int main(void)
{
  STM_EVAL_LEDInit(LED3);

  ssd1306_init();

  uint8_t data[SSD1306_SIZE];
  uint8_t color = 0;
  for(size_t i = 0; i < SSD1306_SIZE; i++) {
    data[i] = color;
    color = ~(~color + 1);
  }
  ssd1306_fills(data);
	for(;;) {
    STM_EVAL_LEDToggle(LED3);
    millis_wait(1000);
  }
}
