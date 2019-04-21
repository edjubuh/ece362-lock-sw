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

#define WU_TIMEOUT      0x01
#define WU_TAG_DETECT   0x02
#define WU_LOW_IRQ_IN   0x08
#define WU_LOW_SPI_SS   0x10
#define ENTER_HIBERNATE         0x0004
#define ENTER_SLEEP             0x0001
#define ENTER_TAG_CALIBRATION   0x00A1
#define ENTER_TAG_DETECTOR      0x0021
#define WU_HIBERNATE        0x0004
#define WU_SLEEP            0x0038
#define WU_TAG_CALIBRATION  0x01F8
#define WU_TAG_DETECTOR     0x0179
#define LEAVE_HIBERNATE     0x0018
#define LEAVE_SLEEP             0x0018
#define LEAVE_TAG_CALIBRATION   0x0018
#define LEAVE_TAG_DETECTOR      0x0018

struct cr95hf_idle_tx {
    uint8_t wu_src;
    uint16_t enter;
    uint16_t wu_control;
    uint16_t leave_control;
    uint8_t wu_period;
    uint8_t osc_start;
    uint8_t dac_start;
    union {
        struct {
            uint8_t dac_low;
            uint8_t dac_high;
        };
        uint16_t dac_data;
    };
    uint8_t swing_count;
    uint8_t max_sleep;
} __attribute__((packed));

void cr95hf_init();

bool cr95hf_echo();
uint8_t cr95hf_idn(struct cr95hf_idn_rx * const resp);
uint8_t cr95hf_proto(enum cr95hf_protocol protocol, uint8_t const * const parameters, uint8_t len, struct cr95hf_proto_rx * const resp);
uint8_t cr95hf_sendrecv(uint8_t const * const data, const uint8_t len, struct cr95hf_rx* const resp);

uint16_t cr95hf_calibrate_tag_detection();

uint8_t cr95hf_idle(struct cr95hf_idle_tx const * const settings);
uint8_t cr95hf_wait_awake(uint8_t * const reason);
uint8_t cr95hf_is_awake(uint8_t * const reason);

#endif