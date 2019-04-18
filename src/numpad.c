#include "numpad.h"

#include "stm32f0xx.h"

void numpad_init() 
{
    // Configure PA8-PA11 (4 pins) to be inputs to timer channels 1-4 (also have pull-down resistors)
    // Cofnigure PB13-15 to be general purpose outputs
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    GPIOA->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10 | GPIO_MODER_MODER11);
    GPIOA->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1;
    GPIOA->AFR[1] &= ~0x0000FFFF;
    GPIOA->AFR[1] |=  0x00002222;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR8_1 | GPIO_PUPDR_PUPDR9_1 | GPIO_PUPDR_PUPDR10_1 | GPIO_PUPDR_PUPDR11_1;

    GPIOB->MODER &= ~(GPIO_MODER_MODER13 | GPIO_MODER_MODER14 | GPIO_MODER_MODER15);
    GPIOB->MODER |= GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1->CCMR1 = TIM1->CCMR2 = 0xF1F1;
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    TIM1->DIER = TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE | TIM_DIER_CC4IE;
    TIM1->CR1 |= TIM_CR1_CEN;

    NVIC->ISER[0] = 1 << TIM1_CC_IRQn;

    GPIOB->BSRR = GPIO_BSRR_BS_13 | GPIO_BSRR_BS_14 | GPIO_BSRR_BS_15;
}

void TIM1_CC_IRQHandler(void) 
{
    uint32_t unused __attribute__((unused));
    if(TIM1->SR & TIM_SR_CC1IF) {
        unused = TIM1->CCR1;
    }
    if(TIM1->SR & TIM_SR_CC2IF) {
        unused = TIM1->CCR2;
    }
    if(TIM1->SR & TIM_SR_CC3IF) {
        unused = TIM1->CCR3;
    }
    if(TIM1->SR & TIM_SR_CC4IF) {
        unused = TIM1->CCR4;
    }
}
