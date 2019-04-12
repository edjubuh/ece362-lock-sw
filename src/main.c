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

#include <stdio.h>

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

extern void serial_init(void);
int main(void)
{
  serial_init();
  STM_EVAL_LEDInit(LED3);
	for(;;) {
    STM_EVAL_LEDToggle(LED3);
    printf("Hello!\n");
    nano_wait(250000000);
  }
}
