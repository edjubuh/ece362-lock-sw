#include "iso14443a.h"

#include <string.h>

#include "cr95hf.h"

static uint8_t iso14443a_reqa(struct iso14443a_atqa * const resp) {
    const uint8_t tx_data[] = { ISO14443A_CMDCODE_REQA, ISO14443A_NBSIGNIFICANTBIT_7 };
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_wupa(struct iso14443a_atqa * const resp) {
    const uint8_t tx_data[] = { ISO14443A_CMDCODE_WUPA, ISO14443A_NBSIGNIFICANTBIT_7 };
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_hlta(struct cr95hf_rx * const resp) {
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_HLTA, 0, ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_anticol1(struct cr95hf_rx * const resp) {
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_SELECT_LVL1, ISO14443A_NVB_20, ISO14443A_NBSIGNIFICANTBIT_8};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_anticol2(struct cr95hf_rx * const resp) {
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_SELECT_LVL2, ISO14443A_NVB_20, ISO14443A_NBSIGNIFICANTBIT_8};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_anticol3(struct cr95hf_rx * const resp) {
    const uint8_t tx_data[] = {ISO14443A_CMDCODE_SELECT_LVL3, ISO14443A_NVB_20, ISO14443A_NBSIGNIFICANTBIT_8};
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_select1(uint8_t const * const uid, uint8_t len_uid, struct cr95hf_rx * const resp) {
    uint8_t tx_data[3 + len_uid];
    tx_data[0] = ISO14443A_CMDCODE_SELECT_LVL1;
    tx_data[1] = ISO14443A_NVB_20;
    memcpy(tx_data + 2, uid, len_uid);
    tx_data[len_uid + 2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, 3 + len_uid, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_select2(uint8_t const * const uid, uint8_t len_uid, struct cr95hf_rx * const resp) {
    uint8_t tx_data[3 + len_uid];
    tx_data[0] = ISO14443A_CMDCODE_SELECT_LVL2;
    tx_data[1] = ISO14443A_NVB_20;
    memcpy(tx_data + 2, uid, len_uid);
    tx_data[len_uid + 2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, 3 + len_uid, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_select3(uint8_t const * const uid, uint8_t len_uid, struct cr95hf_rx * const resp) {
    uint8_t tx_data[3 + len_uid];
    tx_data[0] = ISO14443A_CMDCODE_SELECT_LVL3;
    tx_data[1] = ISO14443A_NVB_20;
    memcpy(tx_data + 2, uid, len_uid);
    tx_data[len_uid + 2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, 3 + len_uid, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_rats(uint8_t fsdi, uint8_t cid, struct cr95hf_rx * const resp) {
    uint8_t tx_data[3];
    tx_data[0] = ISO14443A_CMDCODE_RATS;
    tx_data[1] = ((fsdi << ISO14443A_OFFSET_RATS_FSDI) & ISO14443A_MASK_RATS_FSDI) | (cid & ISO14443A_MASK_RATS_CID);
    tx_data[2] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

static uint8_t iso14443a_pps(uint8_t cid, uint8_t pps[2], struct cr95hf_rx * const resp) {
    uint8_t tx_data[4];
    tx_data[0] = ISO14443A_CMDCODE_PPS | (cid & ISO14443A_MASK_PPS_CID);
    tx_data[1] = pps[0];
    tx_data[2] = pps[1];
    tx_data[3] = ISO14443A_NBSIGNIFICANTBIT_8 | ISO14443A_APPENDCRC;
    return cr95hf_sendrecv(tx_data, sizeof tx_data, (struct cr95hf_rx * const)resp);
}

uint8_t iso14443a_select() {
    struct cr95hf_proto_rx resp;
    uint8_t param = (ISO14443A_TXVALUE_106K << ISO14443A_OFFSET_TXDATARATE) | (ISO14443A_RXVALUE_106K << ISO14443A_OFFSET_RXDATARATE);
    return cr95hf_proto(ISO_14443A, &param, 1, &resp);
}

bool iso14443a_tag_present() {
    struct iso14443a_atqa resp;
    uint8_t r = iso14443a_reqa(&resp);
    return r == 0x80;
}