#include "common.h"

void nano_wait(const uint32_t n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void millis_wait(const uint32_t n) {
    uint32_t start = millis();
    while(millis() - start < n) asm("wfi");
}