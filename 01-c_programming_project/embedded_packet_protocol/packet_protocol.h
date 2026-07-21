/*
=========================================================================================================================
 *** packet_protocol.h ***
 Byte-stream packet protocol: COBS framing with CRC-16-CCITT integrity, and a resynchronizing stream parser.
=========================================================================================================================

 Description:
     This header declares a complete packet layer for byte-oriented links such as UARTs. Outgoing payloads receive a
     CRC-16-CCITT appended in big-endian order and are then encoded with consistent-overhead byte stuffing, which
     removes every zero byte from the frame body so that a single zero can serve as an unambiguous frame delimiter.
     The receiving side is a byte-wise parser designed for interrupt-driven reception: each incoming byte is fed
     individually, frames are cut at the delimiter, decoded, and checked, and the parser resynchronizes automatically
     after garbage, truncation, or corruption, reporting each accepted or rejected frame through status counters.

     COBS guarantees a small and bounded overhead of one byte per 254 payload bytes plus one, which makes the framing
     suitable for links with tight bandwidth budgets, and its encoding is a single linear pass in both directions.
     The CRC catches corruption that framing alone cannot see, including bit flips inside a well-formed frame.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/packet_protocol.c
     tests       tests/test_packet.c (known COBS vectors, CRC vector, round trips, corruption, resync)
     reference   Cheshire and Baker, "Consistent Overhead Byte Stuffing," IEEE/ACM Trans. Networking, 1999;
                 CRC-16-CCITT polynomial 0x1021, initial value 0xFFFF, check value 0x29B1 for "123456789"
=========================================================================================================================
*/
#ifndef PACKET_PROTOCOL_H
#define PACKET_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define PKT_MAX_PAYLOAD 250u   /* Keeps every frame inside one COBS block. */

/* CRC-16-CCITT (poly 0x1021, init 0xFFFF, no reflection, no final xor). */
uint16_t pkt_crc16(const uint8_t *data, size_t len);

/* COBS encode/decode. Destination must hold len + 2 bytes for encode
   (one overhead code byte plus the trailing delimiter is NOT written
   here; the frame builder appends it). Returns bytes written, 0 on error. */
size_t pkt_cobs_encode(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap);
size_t pkt_cobs_decode(const uint8_t *src, size_t len, uint8_t *dst, size_t dst_cap);

/* Build a complete wire frame: payload -> +CRC -> COBS -> +0x00 delimiter.
   Returns total frame length, 0 if the payload is too large or buffers fail. */
size_t pkt_build_frame(const uint8_t *payload, size_t len,
                       uint8_t *frame, size_t frame_cap);

/* Byte-wise receiving parser. */
typedef struct {
    uint8_t  raw[PKT_MAX_PAYLOAD + 4];   /* Encoded bytes of the current frame. */
    size_t   raw_len;
    uint8_t  payload[PKT_MAX_PAYLOAD];   /* Last accepted payload. */
    size_t   payload_len;
    uint32_t frames_ok;                  /* Accepted frames.                    */
    uint32_t frames_crc_err;             /* Delimited but CRC-invalid frames.   */
    uint32_t frames_malformed;           /* COBS-invalid or oversized frames.   */
} pkt_parser_t;

void pkt_parser_init(pkt_parser_t *p);

/* Feed one received byte. Returns 1 when a valid frame has just been
   completed (payload/payload_len are then valid), 0 otherwise. */
int pkt_parser_feed(pkt_parser_t *p, uint8_t byte);

#endif /* PACKET_PROTOCOL_H */
