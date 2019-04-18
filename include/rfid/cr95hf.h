#ifndef _RFID_CR95HF_H_
#define _RFID_CR95HF_H_

#include "common.h"

struct cr95hf_rx_hdr {
    uint8_t code;
    uint8_t len;
};

struct cr95hf_rx {
    struct cr95hf_rx_hdr header;
    uint8_t data[528];
};

struct cr95hf_idn_rx {
    struct cr95hf_rx_hdr header;
    char id[13];
    uint16_t rom_crc;
};

enum cr95hf_protocol {
    OFF = 0,
    ISO_15693 = 1,
    ISO_14443A = 2,
    ISO_14443B = 3,
    ISO_18092 = 4
};

struct cr95hf_proto_rx {
    struct cr95hf_rx_hdr header;
};

void cr95hf_init();

bool cr95hf_echo();
uint8_t cr95hf_idn(struct cr95hf_idn_rx * const resp);
uint8_t cr95hf_proto(enum cr95hf_protocol protocol, uint8_t const * const parameters, uint8_t len, struct cr95hf_proto_rx * const resp);
uint8_t cr95hf_sendrecv(uint8_t const * const data, const uint8_t len, struct cr95hf_rx* const resp);

#endif