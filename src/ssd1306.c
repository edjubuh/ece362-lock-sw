#include "ssd1306.h"

#include "common.h"
#include "stm32f0xx.h"

void ssd1306_cmd(const uint8_t);
void ssd1306_cmds(uint8_t const * const, const size_t);

void ssd1306_init(void) 
{
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_DMA1EN;

	// PB3: SCK
	// PB4: D/C
	// PB5: MOSI
	// PB6: RST
	// PB7: NSS
	GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
	GPIOB->MODER |= GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0;
	GPIOB->AFR[0] &= ~(0x00f0f000);
	GPIOB->OSPEEDR |= 0x0ff0f000;
	GPIOB->BSRR = GPIO_BSRR_BR_4 | GPIO_BSRR_BR_6;

	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR1 &= ~SPI_CR1_SPE;

	SPI1->CR1 = 0; // reset!
	SPI1->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
	SPI1->CR1 |= (2 << 3); // baud rate clk / 4 = 6 MHz

	SPI1->CR2 = 0x0700; // reset!

	SPI1->CR1 |= SPI_CR1_SPE;

    ssd1306_reset();

    const uint8_t init_seq[] = {
        0xAE, // display off
        0x20, // Set Memory Addressing Mode
        0x00, // horizontal addressing mode
        0xC8, // Normal Scan Mode (not flipped vertically)
        0x81, // Set contrast
        0x8F, // Maximum contrast
        0xA1, // flip horizontally
        0xA6, // Non-inverted color
        0xA8, // set multiplex ratio
        0x3F, // to 63
        0xA4, // Use RAM output
        0xD3, // Set display offset
        0x00, // to no offset
        0xD4, // set display clock ratio
        0xF0, // to 240
        0xD9, // Set pre-charge period
        0x22, // to 0x22
        0xDA, // Set COM pin h/w
        0x12,
        0xDB, // Set vcomh
        0x20, // 0.77*Vcc
        0x8D, // Change charge pump
        0x14, // to enabled
        0xAF, // turn on SSD1306
    };
    ssd1306_cmds(init_seq, sizeof init_seq);
}

void ssd1306_reset(void) {
	// CS = High (not selected)
//	HAL_GPIO_WritePin(SSD1306_CS_Port, SSD1306_CS_Pin, GPIO_PIN_SET);

	// Reset the OLED
	GPIOB->BSRR = GPIO_BSRR_BR_6;
	millis_wait(10);
	GPIOB->BSRR = GPIO_BSRR_BS_6;
	millis_wait(150);
}

void ssd1306_cmd(const uint8_t byte) {
	GPIOB->BSRR = GPIO_BSRR_BR_4; // Select command

	GPIOB->BSRR = GPIO_BSRR_BR_7; // Lower SS/CS (select OLED)
	while(!(SPI1->SR & SPI_SR_TXE));
	*(uint8_t*)&SPI1->DR = byte;
	while((SPI1->SR & SPI_SR_BSY));
	GPIOB->BSRR = GPIO_BSRR_BS_7; // Raise SS/CS (unselect OLED)
}

void ssd1306_cmds(uint8_t const * const bytes, const size_t len) 
{
	GPIOB->BSRR = GPIO_BSRR_BR_4; // sedning command

	GPIOB->BSRR = GPIO_BSRR_BR_7; // Lower SS/CS (select OLED)
    for(size_t i = 0; i < len; i++) {
        while(!(SPI1->SR & SPI_SR_TXE));
        *(uint8_t*)&SPI1->DR = bytes[i];
    }

	while((SPI1->SR & SPI_SR_BSY));
	GPIOB->BSRR = GPIO_BSRR_BS_7; // Raise SS/CS (unselect OLED)
}

void ssd1306_data(uint8_t const * const buffer, struct ssd1306_buf_bounds const * const buf_bounds) {
	GPIOB->BSRR = GPIO_BSRR_BS_4; // sending data
	
	GPIOB->BSRR = GPIO_BSRR_BR_7; // Lower SS/CS (select OLED)

	for(size_t i = 0; i < buf_bounds->n_rows; i++) {
		for(size_t j = 0; j < buf_bounds->n_cols; j++) {
		while(!(SPI1->SR & SPI_SR_TXE));
		*(uint8_t*)&SPI1->DR = buffer[(i * buf_bounds->row_len) + (j * buf_bounds->col_wid)];
		}
	}

	while((SPI1->SR & SPI_SR_BSY));
	GPIOB->BSRR = GPIO_BSRR_BS_7; // Raise SS/CS (unselect OLED)
}

void ssd1306_fill(uint8_t const * const buf, const struct ssd1306_bounds bounds, const struct ssd1306_buf_bounds buf_bounds) {
	uint8_t bound_cmd[] = {
		0x21,
		bounds.col_start,
		bounds.col_end - 1,
		0x22,
		bounds.page_start,
		bounds.page_end - 1
	};
	ssd1306_cmds(bound_cmd, sizeof bound_cmd);

	ssd1306_data(buf, &buf_bounds);
}

void ssd1306_fills(uint8_t const * const buf) {
	static const struct ssd1306_bounds bounds = {
		.col_start = 0,
		.col_end = SSD1306_WIDTH,
		.page_start = 0,
		.page_end = SSD1306_HEIGHT
	};
	static const struct ssd1306_buf_bounds buf_bounds = {
		.n_cols = SSD1306_WIDTH,
		.n_rows = SSD1306_HEIGHT,
		.row_len = SSD1306_WIDTH,
		.col_wid = 1
	};
	ssd1306_fill(buf, bounds, buf_bounds);
}

void ssd1306_sleep() {
	ssd1306_cmd(0xAE);
}

void ssd1306_wake() {
	ssd1306_cmd(0xAF);
}