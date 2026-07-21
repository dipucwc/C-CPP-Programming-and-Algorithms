/*
=========================================================================================================================
 *** packet_protocol.c ***
 Implementation of CRC-16-CCITT, single-block COBS coding, frame building, and the resynchronizing parser.
=========================================================================================================================

 Description:
     This file implements the packet layer declared in packet_protocol.h. The CRC is the bitwise CCITT variant with
     polynomial 0x1021 and initial value 0xFFFF, processed most significant bit first with no reflection and no final
     exclusive-or, which matches the widely used check value 0x29B1 for the ASCII string "123456789". The COBS coder
     handles frames up to one block: because the payload limit plus the two CRC bytes stays below 254, exactly one
     overhead byte is needed, and the encoder is a single pass that back-patches the code byte whenever a zero is
     consumed. The decoder inverts the pass and rejects code bytes that would run past the input, which is how
     truncated or corrupted frames surface as malformed instead of reading out of bounds.

     The parser is a small accumulator: bytes are collected until the zero delimiter arrives, then the frame is
     decoded and the CRC verified. Oversized accumulations are discarded byte by byte until the next delimiter, so
     the parser always regains frame lock after garbage, and every outcome increments its dedicated counter for
     link-quality monitoring.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned through values, buffers, and counters.

 Supporting files:
     header      include/packet_protocol.h
=========================================================================================================================
*/
#include "packet_protocol.h"

#include <string.h>

uint16_t pkt_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; ++b) {
            /* MSB-first update: shift out the top bit and fold in the
               polynomial when that bit was set. */
            if (crc & 0x8000u) crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else               crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

size_t pkt_cobs_encode(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap)
{
    if (!src || !dst || len > 253 || dst_cap < len + 1) return 0;

    size_t code_pos = 0;     /* Where the current block's code byte lives. */
    uint8_t code = 1;        /* Distance to the next zero, so far.         */
    size_t out = 1;          /* Output cursor; slot 0 is the code byte.    */

    for (size_t i = 0; i < len; ++i) {
        if (src[i] == 0) {
            /* Back-patch: the code byte records how far ahead the zero
               sat, then a fresh block starts at the current position. */
            dst[code_pos] = code;
            code_pos = out++;
            code = 1;
        } else {
            dst[out++] = src[i];
            code++;
        }
    }
    dst[code_pos] = code;
    return out;
}

size_t pkt_cobs_decode(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap)
{
    if (!src || !dst || len == 0) return 0;

    size_t in = 0, out = 0;
    while (in < len) {
        const uint8_t code = src[in++];
        if (code == 0) return 0;               /* Zeros never appear inside. */
        /* The code byte promises code-1 literal bytes; a promise that runs
           past the input marks the frame as malformed. */
        if (in + (size_t)(code - 1) > len) return 0;
        for (uint8_t k = 1; k < code; ++k) {
            if (out >= dst_cap) return 0;
            dst[out++] = src[in++];
        }
        if (code < 0xFF && in < len) {
            if (out >= dst_cap) return 0;
            dst[out++] = 0;                    /* The zero the code encoded. */
        }
    }
    return out;
}

size_t pkt_build_frame(const uint8_t *payload, size_t len,
                       uint8_t *frame, size_t frame_cap)
{
    if (!payload || !frame || len > PKT_MAX_PAYLOAD) return 0;

    uint8_t body[PKT_MAX_PAYLOAD + 2];
    memcpy(body, payload, len);

    const uint16_t crc = pkt_crc16(payload, len);
    body[len]     = (uint8_t)(crc >> 8);       /* Big-endian on the wire. */
    body[len + 1] = (uint8_t)(crc & 0xFFu);

    const size_t enc = pkt_cobs_encode(body, len + 2, frame, frame_cap);
    if (enc == 0 || enc + 1 > frame_cap) return 0;

    frame[enc] = 0x00;                         /* Delimiter closes the frame. */
    return enc + 1;
}

void pkt_parser_init(pkt_parser_t *p)
{
    memset(p, 0, sizeof *p);
}

int pkt_parser_feed(pkt_parser_t *p, uint8_t byte)
{
    if (byte != 0x00) {
        if (p->raw_len < sizeof p->raw) {
            p->raw[p->raw_len++] = byte;
        } else {
            /* Oversized: drop bytes until the next delimiter restores
               frame lock; the discard is counted once at the delimiter. */
        }
        return 0;
    }

    /* Delimiter: judge the accumulated frame. */
    int accepted = 0;
    if (p->raw_len == 0) {
        /* Back-to-back delimiters are idle filler, not an error. */
    } else if (p->raw_len >= sizeof p->raw) {
        p->frames_malformed++;
    } else {
        uint8_t body[PKT_MAX_PAYLOAD + 2];
        const size_t dec = pkt_cobs_decode(p->raw, p->raw_len, body, sizeof body);
        if (dec < 2) {
            p->frames_malformed++;
        } else {
            const size_t plen = dec - 2;
            const uint16_t rx_crc =
                (uint16_t)(((uint16_t)body[plen] << 8) | body[plen + 1]);
            if (rx_crc == pkt_crc16(body, plen) && plen <= PKT_MAX_PAYLOAD) {
                memcpy(p->payload, body, plen);
                p->payload_len = plen;
                p->frames_ok++;
                accepted = 1;
            } else {
                p->frames_crc_err++;
            }
        }
    }
    p->raw_len = 0;
    return accepted;
}
