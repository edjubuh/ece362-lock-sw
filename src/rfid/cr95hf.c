#include "cr95hf.h"

#include <string.h>

#include "stm32f0xx.h"

#define DMA_TX DMA1_Channel5

struct cr95hf_rx * rx;
volatile bool ready = false;
void USART2_IRQHandler(void)
{
    if(rx) {
        USART2->CR1 &= ~USART_CR1_RXNEIE;
        while (!(USART2->ISR & USART_ISR_RXNE));
        rx->header.code = USART2->RDR;
        while (!(USART2->ISR & USART_ISR_RXNE))
            ;
        rx->header.len = USART2->RDR;
        if (rx->header.code & 0x80)
        {
            rx->header.len |= ((rx->header.code & 0x60) << 3);
        }
        uint8_t *ptr = rx->data;
        for (size_t i = 0; i < rx->header.len; i++)
        {
            while (!(USART2->ISR & USART_ISR_RXNE))
                ;
            *ptr = USART2->RDR;
            ptr++;
        }
        USART2->CR2 |= USART_CR1_RXNEIE;
        ready = true;
    } else {
        USART2->RQR = USART_RQR_RXFRQ;
    }
}

void cr95hf_init()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMA1EN;
    GPIOA->MODER &= ~(GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
    GPIOA->AFR[0] &= ~(0x0000ff00);
    GPIOA->AFR[0] |= (1 << (2 * 4)) | (1 << (3 * 4));

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = 0;
    USART2->CR2 &= ~USART_CR2_STOP;
    USART2->CR2 |= USART_CR2_STOP_1;
    USART2->CR1 &= ~USART_CR1_OVER8;
    USART2->BRR = 0x341;
    USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    while (!(USART2->ISR & USART_ISR_REACK))
        ;
    while (!(USART2->ISR & USART_ISR_TEACK))
        ;

    NVIC->ISER[0] = (1 << USART2_IRQn);

    // USART2->TDR = 0x00;
    // millis_wait(11); // wait 11 ms for HFO setup time
}

bool cr95hf_echo()
{
    USART2->CR1 &= ~USART_CR1_RXNEIE;
    while (!(USART2->ISR & USART_ISR_TXE))
        ;
    USART2->TDR = 0x55;
    uint32_t start = millis();
    while (!(USART2->ISR & USART_ISR_RXNE) && (millis() - start) < 10)
        ;
    bool rv = (USART2->ISR & USART_ISR_RXNE) && USART2->RDR == 0x55;
    USART2->CR1 |= USART_CR1_RXNEIE;
    return rv;
}

void cr95hf_send_cmd(uint8_t cmd, uint8_t const *const buf, uint8_t len)
{
    while(!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = cmd;
    while(!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = len;
    for(size_t i = 0; i < len; i++) {
        while(!(USART2->ISR & USART_ISR_TXE));
        USART2->TDR = buf[i];
    }
}

void cr95hf_recv_data(struct cr95hf_rx *const resp)
{
    while (!(USART2->ISR & USART_ISR_RXNE));
    rx->header.code = USART2->RDR;
    while (!(USART2->ISR & USART_ISR_RXNE))
        ;
    rx->header.len = USART2->RDR;
    if (rx->header.code & 0x80)
    {
        rx->header.len |= ((rx->header.code & 0x60) << 3);
    }
    uint8_t *ptr = rx->data;
    for (size_t i = 0; i < rx->header.len; i++)
    {
        while (!(USART2->ISR & USART_ISR_RXNE))
            ;
        *ptr = USART2->RDR;
        ptr++;
    }
    // rx = resp;
    // const uint32_t start = millis();
    // while(!ready /*&& (millis() - start) < 10*/) asm("wfi");
    // if(!ready) {
    //     resp->header.code = -1;
    // }
    // rx = NULL;
    // ready = false;
}

uint8_t cr95hf_idn(struct cr95hf_idn_rx* const resp) {
    rx = resp;
    cr95hf_send_cmd(0x01, NULL, 0);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    return resp->header.code;
}

uint8_t cr95hf_proto(enum cr95hf_protocol protocol, uint8_t const * const parameters, const uint8_t len, struct cr95hf_proto_rx * const resp) {
    uint8_t buf[len + 1];
    buf[0] = protocol;
    memcpy(buf + 1, parameters, len);
    rx = resp;
    cr95hf_send_cmd(0x02, buf, len + 1);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    millis_wait(10);
    return resp->header.code;
}

uint8_t cr95hf_sendrecv(uint8_t const * const data, const uint8_t len, struct cr95hf_rx* const resp) {
    rx = resp;
    cr95hf_send_cmd(0x04, data, len);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    return resp->header.code;
}