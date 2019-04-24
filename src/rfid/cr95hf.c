#include "cr95hf.h"

#include <string.h>

#include "stm32f0xx.h"
#include "ssd1306.h"

// IRQ Handler for receiving data
// DMA solution won't work because we won't know how much data to receive. Could do a mix of both... but why?
struct cr95hf_rx * volatile rx;
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
    /**
     * PA2: Transmitter to CR95HF. Configure to AF1 (USART2_TX)
     * PA3: Receiver from CR95HF. Configured to AF1 (USART2_RX)
     */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMA1EN;
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1;
    GPIOA->AFR[0] &= ~(0x0000ff00);
    GPIOA->AFR[0] |= (1 << (2 * 4)) | (1 << (3 * 4));

    /**
     * Enable DMA Channel for USART2_TX: DMA1_Channel4
     * Peripheral address is USART2's transmitter data register
     * Memory address is dma_tx_buf
     * DMA IRQ on transfer complete shuts down DMA
     */
    DMA_TX->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_TCIE;
    DMA_TX->CPAR = (uint32_t)&(USART2->TDR);
    DMA_TX->CMAR = (uint32_t)dma_tx_buf;
    NVIC->ISER[0] = (1 << DMA1_Channel4_5_IRQn);

    /**
     * Configure USART2
     */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->CR1 = 0; // Reset CR1 (and disable USART2 in the process)
    USART2->CR2 = USART_CR2_STOP_1;
    USART2->CR3 = USART_CR3_DMAT;
    USART2->BRR = 0x341;
    USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    while (!(USART2->ISR & USART_ISR_REACK));
    while (!(USART2->ISR & USART_ISR_TEACK));
    NVIC->ISER[0] = (1 << USART2_IRQn);

    // CR95HF initialization
    USART2->TDR = 0x00; // pull IRQ_IN/RX low for a short period
    millis_wait(11); // wait 11 ms for HFO setup time
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

/**
 * See Section 5.10 of CR95HF Datasheet (DocID018669 Rev 11)
 * Send 0x55. Get 0x55 back. Makes sense.
 *
 * This method does it without any of the DMA/IRQ fanciness for simplicity.
 */
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
    while (!(USART2->ISR & USART_ISR_RXNE) && (millis() - start) < 5)
        ;
    bool rv = (USART2->ISR & USART_ISR_RXNE) && USART2->RDR == 0x55;

    // restore RX IRQ and TX DMA settings
    USART2->CR1 |= cr1_mask;
    USART2->CR3 |= cr3_mask;
    return rv;
}

/**
 * See Section 5.3 of CR95HF Datasheet (DocID018669 Rev 11)
 * Get brief information about the CR95HF and its revision
 *
 * Added mostly for completion and testing of the send_cmd/recv_data IRQ/DMA pipeline
 */
uint8_t cr95hf_idn(struct cr95hf_idn_rx* const resp) {
    rx = (struct cr95hf_rx* const)resp; // set rx early in case we get a resonse quickly
    cr95hf_send_cmd(0x01, NULL, 0);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    return resp->header.code;
}

/**
 * See Section 5.4 of CR95HF Datasheet (DocID018669 Rev 11)
 * Select a protocol (or turn off the antenna)
 */
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

/**
 * See Section 5.5 of CR95HF Datasheet (DocID018669 Rev 11)
 * Send/Recv command with the antenna. Protocol specific data
 */
uint8_t cr95hf_sendrecv(uint8_t const * const data, const uint8_t len, struct cr95hf_rx* const resp) {
    rx = (struct cr95hf_rx* const)resp; // set rx early in case we get a resonse quickly
    cr95hf_send_cmd(0x04, data, len);
    cr95hf_recv_data((struct cr95hf_rx * const)resp);
    return resp->header.code;
}

struct cr95hf_rx idle_response;
/**
 * See Section 5.6 of CR95HF Datasheet (DocID018669 Rev 11)
 * Idle command. See Section 5.6.1 for parameter description
 * 
 * Does not wait for the device to come out of idle. Use cr95hf_wake_awake or cr95hf_is_awake to block/poll
 */
uint8_t cr95hf_idle(struct cr95hf_idle_tx const * const settings) {
    rx = &idle_response;
    cr95hf_send_cmd(0x07, (uint8_t const * const)settings, sizeof *settings);
    millis_wait(2);
    return 0;
}

/**
 * See Section 5.6 of CR95HF Datasheet (DocID018669 Rev 11)
 * Idle command. See Section 5.6.1 for parameter description
 * 
 * Blocks until device comes out of idle
 */
uint8_t cr95hf_wait_awake(uint8_t * const reason) {
    cr95hf_recv_data(&idle_response);
    if(idle_response.header.code == 0x00 && reason) {
        *reason = idle_response.data[0];
    }
    return idle_response.header.code;
}

/**
 * See Section 5.6 of CR95HF Datasheet (DocID018669 Rev 11)
 * Idle command. See Section 5.6.1 for parameter description
 * 
 * Polls device if it's come out idle
 */
uint8_t cr95hf_is_awake(uint8_t * const reason) {
    if(!ready) {
        return 0xff;
    }
    if(idle_response.header.code == 0x00 && reason) {
        *reason = idle_response.data[0];
    }
    ready = false;
    return idle_response.header.code;
}

/**
 * See Appendix B of CR95HF Datasheet (DocID018669 Rev 11)
 */
uint16_t cr95hf_calibrate_tag_detection() {
    struct cr95hf_idle_tx idle_settings = {
        .wu_src = WU_TIMEOUT | WU_TAG_DETECT,
        .enter = ENTER_TAG_CALIBRATION,
        .wu_control = WU_TAG_CALIBRATION,
        .leave_control = LEAVE_TAG_CALIBRATION,
        .wu_period = 0x20,
        .osc_start = 0x06,
        .dac_start = 0x06,
        .dac_low = 0,
        .dac_high = 0x00,
        .swing_count = 0x3F,
        .max_sleep = 0x01
    };
    // Step 0
    ssd1306_clear(0xff);
    uint8_t wakeup_reason;
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason) || wakeup_reason != WU_TAG_DETECT) {
        return 0xffff;
    }
    // Step 1
    ssd1306_set_row(0, 0);
    idle_settings.dac_high = 0xFC;
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason) || wakeup_reason != WU_TIMEOUT) {
        return 0xffff;
    }
    // Step 2
    ssd1306_set_row(0, 1);
    idle_settings.dac_high -= 0x80;
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason)) {
        return 0xffff;
    }
    // Step 3
    ssd1306_set_row(0, 2);
    if(wakeup_reason & WU_TIMEOUT) {
        idle_settings.dac_high -= 0x40;
    } else if(wakeup_reason & WU_TAG_DETECT) {
        idle_settings.dac_high += 0x40;
    } else {
        return 0xffff;
    }
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason)) {
        return 0xffff;
    }
    // Step 4
    ssd1306_set_row(0, 3);
    if(wakeup_reason & WU_TIMEOUT) {
        idle_settings.dac_high -= 0x20;
    } else if(wakeup_reason & WU_TAG_DETECT) {
        idle_settings.dac_high += 0x20;
    } else {
        return 0xffff;
    }
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason)) {
        return 0xffff;
    }
    // Step 5
    ssd1306_set_row(0, 4);
    if(wakeup_reason & WU_TIMEOUT) {
        idle_settings.dac_high -= 0x10;
    } else if(wakeup_reason & WU_TAG_DETECT) {
        idle_settings.dac_high += 0x10;
    } else {
        return 0xffff;
    }
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason)) {
        return 0xffff;
    }
    // Step 6
    ssd1306_set_row(0, 5);
    if(wakeup_reason & WU_TIMEOUT) {
        idle_settings.dac_high -= 0x08;
    } else if(wakeup_reason & WU_TAG_DETECT) {
        idle_settings.dac_high += 0x08;
    } else {
        return 0xffff;
    }
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason)) {
        return 0xffff;
    }
    // Step 7
    ssd1306_set_row(0, 6);
    if(wakeup_reason & WU_TIMEOUT) {
        idle_settings.dac_high -= 0x04;
    } else if(wakeup_reason & WU_TAG_DETECT) {
        idle_settings.dac_high += 0x04;
    } else {
        return 0xffff;
    }
    cr95hf_idle(&idle_settings);
    if(cr95hf_wait_awake(&wakeup_reason)) {
        return 0xffff;
    }
    // Step 8
    ssd1306_set_row(0, 7);
    if(wakeup_reason & WU_TIMEOUT) {
        idle_settings.dac_high -= 0x04;
    } else if(!(wakeup_reason & WU_TAG_DETECT)) {
        return 0xffff;
    }
    idle_settings.dac_low = idle_settings.dac_high - 8;
    idle_settings.dac_high = idle_settings.dac_high + 8;
    return idle_settings.dac_data;
}

