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

int main(void)
{
  STM_EVAL_LEDInit(LED3);
	for(;;) {
    STM_EVAL_LEDToggle(LED3);
    millis_wait(1000);
  }
}
