/**
 * Functions based on en.stsw-stm320006, developed by ST MMY Application Team
 *
 * Provides functions for using ISO/IEC 14443-3 (Type A) and ISO/IEC 14443-4 (Type A) with CR95HF
 */

#include "iso14443a.h"

#include <string.h>

#include "cr95hf.h"

static uint8_t iso14443a_reqa(struct iso14443a_atqa *const resp)
{
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_REQA, ISO14443A_NBSIGNIFICANTBIT_7};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_wupa(struct iso14443a_atqa *const resp)
{
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_WUPA, ISO14443A_NBSIGNIFICANTBIT_7};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_hlta(struct cr95hf_rx *const resp)
{
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_HLTA, 0, ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_anticol1(struct iso14443a_uid_cln *const resp)
{
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_SELECT_LVL1, ISO14443A_NVB_20, ISO14443A_NBSIGNIFICANTBIT_8};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_anticol2(struct iso14443a_uid_cln *const resp)
{
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_SELECT_LVL2, ISO14443A_NVB_20, ISO14443A_NBSIGNIFICANTBIT_8};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_anticol3(struct iso14443a_uid_cln *const resp)
{
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_SELECT_LVL3, ISO14443A_NVB_20, ISO14443A_NBSIGNIFICANTBIT_8};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_select1(uint8_t const *const uid, struct iso14443a_sak *const resp)
{
    uint8_t tx_data[3 + 5];
    tx_data[0] = ISO14443A_CMDCODE_SELECT_LVL1;
    tx_data[1] = ISO14443A_NVB_70;
    memcpy(tx_data + 2, uid, 5);
    tx_data[5 + 2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, 3 + 5, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_select2(uint8_t const *const uid, struct iso14443a_sak *const resp)
{
    uint8_t tx_data[3 + 5];
    tx_data[0] = ISO14443A_CMDCODE_SELECT_LVL2;
    tx_data[1] = ISO14443A_NVB_70;
    memcpy(tx_data + 2, uid, 5);
    tx_data[5 + 2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, 3 + 5, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_select3(uint8_t const *const uid, struct iso14443a_sak *const resp)
{
    uint8_t tx_data[3 + 5];
    tx_data[0] = ISO14443A_CMDCODE_SELECT_LVL3;
    tx_data[1] = ISO14443A_NVB_70;
    memcpy(tx_data + 2, uid, 5);
    tx_data[5 + 2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, 3 + 5, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_rats(uint8_t fsdi, uint8_t cid, struct cr95hf_rx *const resp)
{
    uint8_t tx_data[3];
    tx_data[0] = ISO14443A_CMDCODE_RATS;
    tx_data[1] = ((fsdi << ISO14443A_OFFSET_RATS_FSDI) & ISO14443A_MASK_RATS_FSDI) | (cid & ISO14443A_MASK_RATS_CID);
    tx_data[2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

static uint8_t iso14443a_pps(uint8_t cid, uint8_t pps[2], struct cr95hf_rx *const resp)
{
    uint8_t tx_data[4];
    tx_data[0] = ISO14443A_CMDCODE_PPS | (cid & ISO14443A_MASK_PPS_CID);
    tx_data[1] = pps[0];
    tx_data[2] = pps[1];
    tx_data[3] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx *const)resp);
}

uint8_t iso14443a_proto_select()
{
    struct cr95hf_proto_rx resp;
    uint8_t param = (ISO14443A_TXVALUE_106K << ISO14443A_OFFSET_TXDATARATE) | (ISO14443A_RXVALUE_106K << ISO14443A_OFFSET_RXDATARATE);
    return cr95hf_proto(ISO_14443A, &param, 1, &resp);
}

bool iso14443a_tag_present()
{
    struct iso14443a_atqa resp;
    uint8_t r = iso14443a_reqa(&resp);
    return r == 0x80;
}

uint8_t iso14443a_select_sequence()
{
    // Check if tag is present (twice)
    if(!iso14443a_tag_present() && !iso14443a_tag_present()) {
        return 0xFF;
    }

    struct iso14443a_uid_cln uid_cln;
    struct iso14443a_sak sak;
    if(iso14443a_anticol1(&uid_cln) != 0x80) {
        return 0xFF;
    }
    // Note: passing in uid_cln.message is also supposed to pass in the bcc field
    if(iso14443a_select1(uid_cln.message, &sak) != 0x80) {
        return 0xFF;
    }
    if(!(sak.sak & ISO14443A_MASK_SAK_UIDNOTCOMPLETE)) { // so response did complete
        return sak.sak;
    }

    if(iso14443a_anticol2(&uid_cln) != 0x80) {
        return 0xFF;
    }
    if(iso14443a_select2(uid_cln.message, &sak) != 0x80) {
        return 0xFF;
    }
    if(!(sak.sak & ISO14443A_MASK_SAK_UIDNOTCOMPLETE)) { // so response did complete
        return sak.sak;
    }

    if(iso14443a_anticol3(&uid_cln) != 0x80) {
        return 0xFF;
    }
    if(iso14443a_select3(uid_cln.message, &sak) != 0x80) {
        return 0xFF;
    }
    if(!(sak.sak & ISO14443A_MASK_SAK_UIDNOTCOMPLETE)) { // so response did complete
        return sak.sak;
    }

    return 0xFF;
}

// just like select but keep track of UID along the way
uint8_t iso14443a_get_uid(struct iso14443a_uid * const uid)
{
    // Check if tag is present (twice)
    if(!iso14443a_tag_present() && !iso14443a_tag_present()) {
        return 0xFF;
    }

    uid->size = 0;
    struct iso14443a_uid_cln uid_cln;
    struct iso14443a_sak sak;
    ////////////////////////////////////////////////////////////////////////////
    if(iso14443a_anticol1(&uid_cln) != 0x80) {
        return 0xFF;
    }
    // Check the check byte
    if(uid_cln.message[0] ^ uid_cln.message[1] ^ uid_cln.message[2] ^ uid_cln.message[3] ^ uid_cln.bcc) {
        return 0xFF;
    }
    // if the first message byte is 0x88 (CT), then UID is larger and only copy 3 of the 4 bytes
    if(uid_cln.message[0] != 0x88) {
        memcpy(uid->uid + uid->size, uid_cln.message, ISO14443A_NBBYTE_UIDSINGLE);
        uid->size += ISO14443A_NBBYTE_UIDSINGLE;
    } else {
        memcpy(uid->uid + uid->size, uid_cln.message + 1, ISO14443A_NBBYTE_UIDSINGLE - 1);
        uid->size += ISO14443A_NBBYTE_UIDSINGLE - 1;
    }
    // Note: passing in uid_cln.message is also supposed to pass in the bcc field
    if(iso14443a_select1(uid_cln.message, &sak) != 0x80) {
        return 0xFF;
    }
    if(!(sak.sak & ISO14443A_MASK_SAK_UIDNOTCOMPLETE)) { // so response did complete
        return sak.sak;
    }

    ////////////////////////////////////////////////////////////////////////////
    if(iso14443a_anticol2(&uid_cln) != 0x80) {
        return 0xFF;
    }
    // Check the check byte
    if(uid_cln.message[0] ^ uid_cln.message[1] ^ uid_cln.message[2] ^ uid_cln.message[3] ^ uid_cln.bcc) {
        return 0xFF;
    }
    // if the first message byte is 0x88 (CT), then UID is larger and only copy 3 of the 4 bytes
    if(uid_cln.message[0] != 0x88) {
        memcpy(uid->uid + uid->size, uid_cln.message, ISO14443A_NBBYTE_UIDSINGLE);
        uid->size += ISO14443A_NBBYTE_UIDSINGLE;
    } else {
        memcpy(uid->uid + uid->size, uid_cln.message + 1, ISO14443A_NBBYTE_UIDSINGLE - 1);
        uid->size += ISO14443A_NBBYTE_UIDSINGLE - 1;
    }
    // Note: passing in uid_cln.message is also supposed to pass in the bcc field
    if(iso14443a_select2(uid_cln.message, &sak) != 0x80) {
        return 0xFF;
    }
    if(!(sak.sak & ISO14443A_MASK_SAK_UIDNOTCOMPLETE)) { // so response did complete
        return sak.sak;
    }

    ////////////////////////////////////////////////////////////////////////////
    if(iso14443a_anticol3(&uid_cln) != 0x80) {
        return 0xFF;
    }
    // Check the check byte
    if(uid_cln.message[0] ^ uid_cln.message[1] ^ uid_cln.message[2] ^ uid_cln.message[3] ^ uid_cln.bcc) {
        return 0xFF;
    }
    // if the first message byte is 0x88 (CT), then UID is larger and only copy 3 of the 4 bytes
    if(uid_cln.message[0] != 0x88) {
        memcpy(uid->uid + uid->size, uid_cln.message, ISO14443A_NBBYTE_UIDSINGLE);
        uid->size += ISO14443A_NBBYTE_UIDSINGLE;
    } else {
        memcpy(uid->uid + uid->size, uid_cln.message + 1, ISO14443A_NBBYTE_UIDSINGLE - 1);
        uid->size += ISO14443A_NBBYTE_UIDSINGLE - 1;
    }
    // Note: passing in uid_cln.message is also supposed to pass in the bcc field
    if(iso14443a_select3(uid_cln.message, &sak) != 0x80) {
        return 0xFF;
    }
    if(!(sak.sak & ISO14443A_MASK_SAK_UIDNOTCOMPLETE)) { // so response did complete
        return sak.sak;
    }

    return 0xFF;
}
