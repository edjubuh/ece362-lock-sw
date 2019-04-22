#include "stm32f0xx.h"

#include "common.h"
#include <sys/types.h>

uint8_t pressed[4];
volatile uint8_t pos;

static const char KEYMAP[4][3] = {
    {'#', '0', '*'},
    {'9', '8', '7'},
    {'6', '5', '4'},
    {'3', '2', '1'}
};

void numpad_init()
{
    // Configure PA8-PA11 (4 pins) to be inputs to timer 1 channels 1-4 (also have pull-down resistors)
    // Configure PB13-PB15 (3 pins) to be outputs serviced by timer 3
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    GPIOA->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10 | GPIO_MODER_MODER11);
    GPIOA->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1;
    GPIOA->AFR[1] &= ~0x0000FFFF;
    GPIOA->AFR[1] |=  0x00002222;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR8_1 | GPIO_PUPDR_PUPDR9_1 | GPIO_PUPDR_PUPDR10_1 | GPIO_PUPDR_PUPDR11_1;

    GPIOB->MODER &= ~(GPIO_MODER_MODER13 | GPIO_MODER_MODER14 | GPIO_MODER_MODER15);
    GPIOB->MODER |= GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0;

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->PSC = (48 * 100 / 2) - 1; // counter period is 200 microseconds
    TIM3->ARR = 1;
    TIM3->DIER = TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] = 1 << TIM3_IRQn;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1->CCMR1 = TIM1->CCMR2 = 0xF1F1;
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    TIM1->DIER = TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE | TIM_DIER_CC4IE;
    TIM1->CR1 = TIM_CR1_CKD_1 | TIM_CR1_CEN;
    NVIC->ISER[0] = 1 << TIM1_CC_IRQn;

    pos = 0;
}

volatile uint32_t tim3_cnt = 0;
void TIM3_IRQHandler() {
	tim3_cnt = (tim3_cnt + 1) & 0x7;
	GPIOB->BSRR = (((tim3_cnt & 0x1) << (tim3_cnt >> 1)) << 13) | GPIO_BSRR_BR_13 | GPIO_BSRR_BR_14 | GPIO_BSRR_BR_15;
	TIM3->SR &= ~TIM_SR_UIF;
}

uint8_t last_column = 0xff;
uint8_t last_row = 0xff;
uint32_t last_capture = 0;
// MUST READ TIM3_CNT WITHIN 20 MICROSECONDS!
void TIM1_CC_IRQHandler(void)
{
    register uint8_t column = tim3_cnt >> 1;
    register uint8_t row = 0xff;
    register uint32_t unused __attribute__((unused));
    if(TIM1->SR & TIM_SR_CC1IF) {
        unused = TIM1->CCR1;
        row = 0;
    }
    if(TIM1->SR & TIM_SR_CC2IF) {
        unused = TIM1->CCR2;
        row = 1;
    }
    if(TIM1->SR & TIM_SR_CC3IF) {
        unused = TIM1->CCR3;
        row = 2;
    }
    if(TIM1->SR & TIM_SR_CC4IF) {
        unused = TIM1->CCR4;
        row = 3;
    }

    if(row != 0xff) {
        register uint32_t current_time = millis();
        if(last_capture + 500 < current_time && (last_capture + 1000 < current_time || last_column != column || last_row != row)) {
            last_capture = current_time;
            if(pos < 4) {
                pressed[pos] = KEYMAP[row][column];
                pos += 1;
            } else {
                pos = 5;
            }
        }
        last_column = column;
        last_row = row;
    }
}

uint8_t get_pressed_keys(uint8_t * const keys) {
    if(keys) {
        size_t n = pos > 4 ? 4 : pos;
        for(size_t i = 0; i < n; i++) {
            keys[i] = pressed[i];
        }
    }
    return pos;
}

void reset_keys() {
    pos = 0;
}