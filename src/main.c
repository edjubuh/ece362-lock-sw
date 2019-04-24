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

#include <string.h>

#include "common.h"
#include "lock.h"
#include "numpad.h"
#include "rfid/cr95hf.h"
#include "rfid/iso14443a.h"
#include "ssd1306.h"

struct combo {
  uint8_t keys[4];
  struct iso14443a_uid uid;
};
const static struct combo COMBOS[] = {
  {
    .keys = {'1','2','3','4'},
    .uid = {.size = ISO14443A_NBBYTE_UIDSINGLE, .uid = { 0x97, 0x4d, 0x28, 0xd9} }
  },
  {
    .keys = {'2','5','8','#'},
    .uid = {.size = ISO14443A_NBBYTE_UIDDOUBLE, .uid = { 4, 53, 45, 155, 55, 66, 128, 129, 56, 81}}
  },
  {
    .keys = {'*','*','*','*'},
    .uid = {.size = ISO14443A_NBBYTE_UIDSINGLE, .uid = { 0xdc, 0xfc, 0xad, 0xe2} }
  }
};

#define NUM_COMBOS (sizeof COMBOS / sizeof *COMBOS)

int main(void)
{
  systick_init();
  ssd1306_init();
  cr95hf_init();
  numpad_init();

  STM_EVAL_LEDInit(LED4);

  for(size_t i = 0; i < 50; i++) {
    if(cr95hf_echo()) break;
  }

  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
  GPIOB->MODER &= ~GPIO_MODER_MODER0;
  GPIOB->MODER |= GPIO_MODER_MODER0_0;
  GPIOB->BSRR = GPIO_BSRR_BR_0;

  struct cr95hf_idle_tx idle_setting = {
    .wu_src = WU_TAG_DETECT | WU_LOW_IRQ_IN,
    .enter = ENTER_TAG_DETECTOR,
    .wu_control = WU_TAG_DETECTOR,
    .leave_control = LEAVE_TAG_DETECTOR,
    .wu_period = 0x20, // default value
    .osc_start = 0x60,
    .dac_start = 0x60,
    .dac_data = cr95hf_calibrate_tag_detection(),
    .swing_count = 0x3F, // recommended value
    .max_sleep = 0 // unused
  };
  ssd1306_clear(0);

  cr95hf_idle(&idle_setting);
  ssd1306_sleep();
  systick_disable();
  micros_wait(2000);

  uint8_t chars[4];
  uint8_t num_pressed = 0;
  struct iso14443a_uid uid;
  uint8_t have_uid = 0, cr95hf_woke_up = 0;

  for (;;)
  {
    STM_EVAL_LEDOn(LED4);
    asm("wfi");
    STM_EVAL_LEDOff(LED4);

    systick_enable();
    micros_wait(1500); // idle for 1.5 millisecond to get a systick
    ssd1306_wake();
    uint32_t start = millis();
    while(start + 10000 > millis()) {
      num_pressed = get_pressed_keys(chars);
      display_filled(num_pressed);

      if(cr95hf_is_awake(NULL) == 0x00) {
        cr95hf_woke_up = 1;
        iso14443a_proto_select();
        if (iso14443a_tag_present())
        {
          uint8_t sak = iso14443a_get_uid(&uid);
          have_uid = !(sak & ISO14443A_SAK_UIDNOTCOMPLETE);
        }
      }

      if(have_uid && num_pressed == 4) {
        break;
      }

      asm("wfi");
    }

    if(have_uid) {
      if(num_pressed == 4) {
        bool matched = false;
        for(size_t i = 0; i < NUM_COMBOS; i++) {
          if(strncmp(chars, COMBOS[i].keys, 4)) continue;
          if(uid.size != COMBOS[i].uid.size) continue;
          if(strncmp(uid.uid, COMBOS[i].uid.uid, uid.size)) continue;
          matched = true;
          break;
        }
        if(matched) {
          display_access();
          GPIOB->BSRR = GPIO_BSRR_BS_0;
          millis_wait(5000);
          GPIOB->BSRR = GPIO_BSRR_BR_0;
        } else {
          display_no_access();
          millis_wait(2000);
        }
      }
    }
    millis_wait(250);

    if(cr95hf_woke_up) {
      idle_setting.dac_data = cr95hf_calibrate_tag_detection();
      cr95hf_idle(&idle_setting);
    }
    have_uid = 0;
    cr95hf_woke_up = 0;
    reset_keys();
    ssd1306_sleep();
    systick_disable();
    micros_wait(2500);
  }
}
