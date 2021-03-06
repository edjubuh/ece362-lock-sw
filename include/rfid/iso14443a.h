/**
 * Functions and macros based on en.stsw-stm320006, developed by ST MMY Application Team
 *
 * Provides functions for using ISO/IEC 14443-3 (Type A) and ISO/IEC 14443-4 (Type A) with CR95HF
 */

#ifndef _RFID_ISO14443A_H_
#define _RFID_ISO14443A_H_

#include "rfid/cr95hf.h"

// command code
#define ISO14443A_CMDCODE_REQA							0x26
#define ISO14443A_CMDCODE_WUPA							0x52
#define ISO14443A_CMDCODE_HLTA							0x50
#define ISO14443A_CMDCODE_RATS							0xE0
#define ISO14443A_CMDCODE_PPS							0xD0
/* Anticollison levels (commands) */
#define ISO14443A_CMDCODE_SELECT_LVL1					0x93
#define ISO14443A_CMDCODE_SELECT_LVL2					0x95
#define ISO14443A_CMDCODE_SELECT_LVL3					0x97

/* UID Sizes */
#define ISO14443A_NBBYTE_UIDSINGLE						4
#define ISO14443A_NBBYTE_UIDDOUBLE						7
#define ISO14443A_NBBYTE_UIDTRIPLE						10
#define ISO14443A_NBBYTE_SELECTRESPONSE					4
#define ISO14443A_NBBYTE_SELECT							5

/* NVB bytes values */
#define ISO14443A_NVB_10								0x10
#define ISO14443A_NVB_20								0x20
#define ISO14443A_NVB_30								0x30
#define ISO14443A_NVB_40								0x40
#define ISO14443A_NVB_50								0x50
#define ISO14443A_NVB_60								0x60
#define ISO14443A_NVB_70								0x70

/* ATQ FLAG */
#define ISO14443A_ATQA_UID_MASK							0xC0
#define ISO14443A_ATQA_UID_SINGLESIZE					0
#define	ISO14443A_ATQA_UID_DOUBLESIZE					1
#define ISO14443A_ATQA_UID_TRIPLESIZE					2


/* SAK byte constant */
#define ISO14443A_SAK_14443_4_COMPATIBLE				0x20
#define ISO14443A_SAK_UIDNOTCOMPLETE					0x04

/* command control byte value of CR95HF */
#define ISO14443A_NBSIGNIFICANTBIT_7					0x07
#define ISO14443A_NBSIGNIFICANTBIT_8					0x08
#define ISO14443A_APPENDCRC								0x20

/* command control byte value of CR95HF */
#define ISO14443A_MAX_NBBYTE_UID						0x07
#define ISO14443A_CMD_MAXNBBYTE							0x10
#define ISO14443A_ANSWER_MAXNBBYTE						0x20

/* protocol select parameters 	*/
#define ISO14443A_PROTOCOLSELECT_NBBYTE					0x02
#define ISO14443A_MASK_APPENDCRC						0x01
#define ISO14443A_MASK_TXDATARATE						0xC0
#define ISO14443A_MASK_RXDATARATE						0x30
#define ISO14443A_MASK_RFU								0x0C

#define ISO14443A_TXVALUE_106K							0x00
#define ISO14443A_TXVALUE_212K							0x01
#define ISO14443A_TXVALUE_424K							0x02
#define ISO14443A_TXVALUE_847K							0x03

#define ISO14443A_RXVALUE_106K							0x00
#define ISO14443A_RXVALUE_212K							0x01
#define ISO14443A_RXVALUE_424K							0x02
#define ISO14443A_RXVALUE_847K							0x03
												
#define ISO14443A_MASK_SLOTNUMBER						0xF0

/* ISO14443_A Mask */
#define ISO14443A_MASK_PROPRIETARYCODING				0x0F
#define ISO14443A_MASK_UIDSIZEBITFRAME					0xC0
#define ISO14443A_MASK_BITFRAMEANTICOL					0x1F
#define ISO14443A_MASK_COLLISIONBIT						0x80
#define ISO14443A_MASK_CRCBIT							0x20
#define ISO14443A_MASK_PARITYBIT						0x10
#define ISO14443A_MASK_SIGNIFICANTBITS					0x0F
#define ISO14443A_MASK_FIRSTBYTECOLLISION				0xFF
#define ISO14443A_MASK_FIRSTBITCOLLISION				0x0F
#define ISO14443A_MASK_RATS_FSDI						0xF0
#define ISO14443A_MASK_RATS_CID							0x0F
#define ISO14443A_MASK_PPS_CID							0x0F
#define ISO14443A_MASK_14443_4_COMPATIBLE				0x24
#define ISO14443A_MASK_SAK_UIDNOTCOMPLETE				0x04
#define ISO14443A_MASK_UIDSIZE							0x03

/* ISO14443_A offset */
#define ISO14443A_OFFSET_APPENDCRC						0x00
#define ISO14443A_OFFSET_TXDATARATE						0x06
#define ISO14443A_OFFSET_RXDATARATE						0x04
#define ISO14443A_OFFSET_SLOTNUMBER						0x04
#define ISO14443A_OFFSET_NVB							0x04
#define ISO14443A_OFFSET_UIDSIZEBITFRAME				0x06
#define ISO14443A_OFFSET_RATS_FSDI						0x04
#define ISO14443A_OFFSET_UIDSIZE						6


/* ISO14443_A error code */
#define ISO14443A_RESULTOK 								0
#define ISO14443A_ERRORGENERIC							0xA0
#define ISO14443A_ERRORCODE_PROTOCOLSELECT				0xA1
#define ISO14443A_ERRORCODE_PARAMETER					0xA2
#define ISO14443A_ERRORCODE_CRCRESIDUE					0xA3
#define ISO14443A_ERRORCODE_BCC							0xA4

struct iso14443a_rx_ftr {
    uint8_t flags;
    uint8_t collision_byte;
    uint8_t collision_bit;
};

struct iso14443a_atqa {
    struct cr95hf_rx_hdr header;
    uint16_t atqa;
    struct iso14443a_rx_ftr footer;
};

struct iso14443a_sak {
    struct cr95hf_rx_hdr header;
    uint8_t sak;
    struct iso14443a_rx_ftr footer;
};

struct iso14443a_uid_cln {
    struct cr95hf_rx_hdr header;
    uint8_t message[4];
    uint8_t bcc;
    struct iso14443a_rx_ftr footer;
};

struct iso14443a_uid {
    enum {
        SINGLE = ISO14443A_NBBYTE_UIDSINGLE,
        DOUBLE = ISO14443A_NBBYTE_UIDDOUBLE,
        TRIPLE = ISO14443A_NBBYTE_UIDTRIPLE
    } size;
    uint8_t uid[TRIPLE]; // max size is triple (10 bytes)
};

uint8_t iso14443a_proto_select();
bool iso14443a_tag_present();
uint8_t iso14443a_get_uid(struct iso14443a_uid * const uid);

#endif