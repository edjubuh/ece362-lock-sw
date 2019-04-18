#include "common.h"

#include "stm32f0xx_conf.h"

volatile uint32_t ticks = 0;

void SysTick_Handler()
{
    ticks++;
}

void systick_init()
{
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    (void)SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
}

uint32_t millis()
{
    return ticks;
}