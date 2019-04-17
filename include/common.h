#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

void nano_wait(const uint32_t);
#define millis_wait(A) nano_wait((A) * 1000000)
#define micros_wait(A) nano_wait((A) * 1000)

#endif