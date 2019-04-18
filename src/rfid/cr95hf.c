#include "cr95hf.h"

#include <string.h>

#include "stm32f0xx.h"

// IRQ Handler for receiving data
// DMA solution won't work because we won't know how much data to receive. Could do a mix of both... but why?
struct cr95hf_rx * rx;
volatile bool ready = false;
void USART2_IRQHandler(void)
{
    if(rx && USART2->ISR & USART_ISR_RXNE) {
        USART2->CR1 &= ~USART_CR1_RXNEIE;
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
        USART2->CR1 |= USART_CR1_RXNEIE;
        rx = NULL;
        ready = true;
    } else {
        USART2->RQR = USART_RQR_RXFRQ;
    }
}

#define DMA_TX DMA1_Channel4
uint8_t dma_tx_buf[0xff + 2];

// simple IRQ handler to disable DMA_TX once transfer's complete (so we can start a new one)
void DMA1_Channel4_5_IRQHandler(void)
{
    if(DMA1->ISR & DMA_ISR_TCIF4) {
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        DMA_TX->CCR &= ~DMA_CCR_EN;
    } else {
        asm("nop"); // for breakpoint
    }
}

void cr95hf_init()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMA1EN;
    GPIOA->MODER &= ~(GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
    GPIOA->AFR[0] &= ~(0x0000ff00);
    GPIOA->AFR[0] |= (1 << (2 * 4)) | (1 << (3 * 4));

    DMA_TX->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_TCIE;
    DMA_TX->CPAR = (uint32_t)&(USART2->TDR);
    DMA_TX->CMAR = (uint32_t)dma_tx_buf;
    NVIC->ISER[0] = (1 << DMA1_Channel4_5_IRQn);

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = 0;
    USART2->CR2 &= ~USART_CR2_STOP;
    USART2->CR2 |= USART_CR2_STOP_1;
    USART2->CR1 &= ~USART_CR1_OVER8;
    USART2->BRR = 0x341;
    USART2->CR3 |= USART_CR3_DMAT;
    USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    while (!(USART2->ISR & USART_ISR_REACK))
        ;
    while (!(USART2->ISR & USART_ISR_TEACK))
        ;

    NVIC->ISER[0] = (1 << USART2_IRQn);

    USART2->TDR = 0x00; // pull IRQ_IN/RX low for a short period
    millis_wait(11); // wait 11 ms for HFO setup time
}

bool cr95hf_echo()
{
    while(DMA_TX->CCR & DMA_CCR_EN) asm("wfi"); // wait for running transaction to end
    while (!(USART2->ISR & USART_ISR_TXE));

    // Disable RX IRQ and TX DMA, if on
    uint32_t cr1_mask = USART2->CR1 & (USART_CR1_RXNEIE | USART_CR1_TE);
    uint32_t cr3_mask = USART2->CR3 & USART_CR3_DMAT;
    USART2->CR1 &= ~cr1_mask;
    USART2->CR3 &= ~cr3_mask;

    USART2->CR1 |= USART_CR1_TE;
    while (!(USART2->ISR & USART_ISR_TEACK));

    USART2->TDR = 0x55;
    uint32_t start = millis();
    while (!(USART2->ISR & USART_ISR_RXNE) && (millis() - start) < 10)
        ;
    bool rv = (USART2->ISR & USART_ISR_RXNE) && USART2->RDR == 0x55;

    // restore RX IRQ and TX DMA settings
    USART2->CR1 |= cr1_mask;
    USART2->CR3 |= cr3_mask;
    return rv;
}

void cr95hf_send_cmd(uint8_t cmd, uint8_t const *const buf, uint8_t len)
{
    while(DMA_TX->CCR & DMA_CCR_EN) asm("wfi"); // wait for running transaction to end
    dma_tx_buf[0] = cmd;
    dma_tx_buf[1] = len;
    memcpy(dma_tx_buf + 2, buf, len);
    DMA_TX->CNDTR = len + 2;
    DMA_TX->CCR |= DMA_CCR_EN;
}

void cr95hf_recv_data(struct cr95hf_rx *const resp)
{
    rx = resp;
    const uint32_t start = millis();
    while(!ready /*&& (millis() - start) < 10*/) asm("wfi");
    if(!ready) {
        resp->header.code = -1;
    }
    ready = false;
}

uint8_t cr95hf_idn(struct cr95hf_idn_rx* const resp) {
    rx = (struct cr95hf_rx* const)resp; // set rx early in case we get a resonse quickly
    cr95hf_send_cmd(0x01, NULL, 0);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    return resp->header.code;
}

uint8_t cr95hf_proto(enum cr95hf_protocol protocol, uint8_t const * const parameters, const uint8_t len, struct cr95hf_proto_rx * const resp) {
    uint8_t buf[len + 1];
    buf[0] = protocol;
    memcpy(buf + 1, parameters, len);
    rx = (struct cr95hf_rx* const)resp; // set rx early in case we get a resonse quickly
    cr95hf_send_cmd(0x02, buf, len + 1);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    millis_wait(10);
    return resp->header.code;
}

uint8_t cr95hf_sendrecv(uint8_t const * const data, const uint8_t len, struct cr95hf_rx* const resp) {
    rx = (struct cr95hf_rx* const)resp; // set rx early in case we get a resonse quickly
    cr95hf_send_cmd(0x04, data, len);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    return resp->header.code;
}