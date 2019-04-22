#ifndef _NUMPAD_H_
#define _NUMPAD_H_

void numpad_init();

uint8_t get_pressed_keys(uint8_t * const keys);
void reset_keys();

#endif