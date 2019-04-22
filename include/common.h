#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

void nano_wait(const uint32_t);
void millis_wait(const uint32_t);
#define micros_wait(A) nano_wait((A) * 1000)

uint32_t millis();
void systick_init();
void systick_enable();
void systick_disable();

#endif