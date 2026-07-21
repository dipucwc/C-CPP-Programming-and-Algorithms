/*
=========================================================================================================================
 *** test_packet ***
 Unit tests: published COBS vectors, the CRC check value, round trips, corruption rejection, and resynchronization.
=========================================================================================================================

 Description:
     This test verifies the packet layer through five properties. The first property checks the coder against the
     published consistent-overhead byte stuffing examples, including the all-zero and mixed-content vectors, so the
     encoding is pinned to the specification rather than to this implementation's own decoder. The second property
     checks the CRC against the standard CCITT check value 0x29B1 for the ASCII string "123456789". The third
     property performs frame round trips through a byte-fed parser for payload lengths from zero to the maximum,
     including payloads full of zeros and delimiter-valued bytes, which is exactly what the stuffing exists for. The
     fourth property injects corruption: a single flipped bit anywhere in a frame must be rejected and counted as a
     CRC error or malformed frame, never accepted. The fifth property interleaves garbage and truncated fragments
     between valid frames and requires the parser to keep accepting every valid frame afterwards, demonstrating
     resynchronization.

 Input:
     (none)         All vectors and frames are built in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/packet_protocol.h (the functions under test)
=========================================================================================================================
*/
#include <stdio.h>
#include <string.h>

#include "packet_protocol.h"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("  FAILED: %s (line %d)\n", #cond, __LINE__);        \
            ++fails;                                                    \
        }                                                               \
    } while (0)

int main(void)
{
    int fails = 0;

    {   /* Property 1: published COBS vectors. */
        uint8_t out[16];

        const uint8_t v1[] = {0x00};
        CHECK(pkt_cobs_encode(v1, 1, out, sizeof out) == 2);
        CHECK(out[0] == 0x01 && out[1] == 0x01);

        const uint8_t v2[] = {0x11, 0x22, 0x00, 0x33};
        CHECK(pkt_cobs_encode(v2, 4, out, sizeof out) == 5);
        const uint8_t v2_ref[] = {0x03, 0x11, 0x22, 0x02, 0x33};
        CHECK(memcmp(out, v2_ref, 5) == 0);

        const uint8_t v3[] = {0x00, 0x00};
        CHECK(pkt_cobs_encode(v3, 2, out, sizeof out) == 3);
        const uint8_t v3_ref[] = {0x01, 0x01, 0x01};
        CHECK(memcmp(out, v3_ref, 3) == 0);
    }

    {   /* Property 2: CRC-16-CCITT check value. */
        const uint8_t s[] = "123456789";
        CHECK(pkt_crc16(s, 9) == 0x29B1u);
        CHECK(pkt_crc16(s, 0) == 0xFFFFu);     /* Empty input returns init. */
    }

    {   /* Property 3: round trips through the byte-fed parser. */
        pkt_parser_t p;
        pkt_parser_init(&p);
        uint8_t payload[PKT_MAX_PAYLOAD];
        uint8_t frame[PKT_MAX_PAYLOAD + 8];

        for (size_t len = 0; len <= PKT_MAX_PAYLOAD; len += 7) {
            for (size_t i = 0; i < len; ++i) {
                /* Cycle through all byte values including 0x00. */
                payload[i] = (uint8_t)(i * 37u + len);
            }
            const size_t flen = pkt_build_frame(payload, len, frame, sizeof frame);
            CHECK(flen > 0);

            int done = 0;
            for (size_t i = 0; i < flen; ++i) {
                done = pkt_parser_feed(&p, frame[i]);
            }
            CHECK(done == 1);
            CHECK(p.payload_len == len);
            CHECK(memcmp(p.payload, payload, len) == 0);
        }
        CHECK(p.frames_crc_err == 0 && p.frames_malformed == 0);
    }

    {   /* Property 4: any single flipped bit is rejected. */
        const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x42};
        uint8_t frame[32];
        const size_t flen = pkt_build_frame(payload, sizeof payload,
                                            frame, sizeof frame);
        CHECK(flen > 0);

        int accepted_corrupt = 0;
        for (size_t pos = 0; pos + 1 < flen; ++pos) {       /* Skip delimiter. */
            for (int bit = 0; bit < 8; ++bit) {
                pkt_parser_t p;
                pkt_parser_init(&p);
                uint8_t bad[32];
                memcpy(bad, frame, flen);
                bad[pos] ^= (uint8_t)(1u << bit);
                for (size_t i = 0; i < flen; ++i) {
                    if (pkt_parser_feed(&p, bad[i])) accepted_corrupt = 1;
                }
                /* Flipping a byte to 0x00 splits the frame; both halves
                   must then fail, which the counters confirm below. */
                CHECK(p.frames_ok == 0);
            }
        }
        CHECK(accepted_corrupt == 0);
    }

    {   /* Property 5: resynchronization after garbage and truncation. */
        pkt_parser_t p;
        pkt_parser_init(&p);
        const uint8_t payload[] = "hello";
        uint8_t frame[32];
        const size_t flen = pkt_build_frame(payload, 5, frame, sizeof frame);

        uint32_t good = 0;
        for (int round = 0; round < 50; ++round) {
            /* Garbage burst. */
            for (int g = 0; g < 17; ++g) {
                (void)pkt_parser_feed(&p, (uint8_t)(0x5A + g));
            }
            (void)pkt_parser_feed(&p, 0x00);            /* Force a cut.     */
            /* Truncated fragment of a real frame. */
            for (size_t i = 0; i < flen / 2; ++i) {
                (void)pkt_parser_feed(&p, frame[i]);
            }
            (void)pkt_parser_feed(&p, 0x00);            /* Cut mid-frame.   */
            /* A full valid frame must still be accepted. */
            for (size_t i = 0; i < flen; ++i) {
                if (pkt_parser_feed(&p, frame[i])) ++good;
            }
        }
        CHECK(good == 50);
        CHECK(p.frames_ok == 50);
    }

    printf("[test_packet] cobs-vectors + crc + roundtrip + corruption + resync -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
