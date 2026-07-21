/*
=========================================================================================================================
 *** kv_store.c ***
 Implementation of the log-structured store: record format, mount election, append path, and compaction.
=========================================================================================================================

 Description:
     This file implements the store declared in kv_store.h. The on-flash record is state byte, two key bytes, one
     length byte, the value, and a CRC-16 over key, length, and value. The state byte uses bit-clearing transitions:
     erased 0xFF means empty, 0xAA means valid, 0x00 means dead; programming the content first and the valid state
     last makes the state byte the commit point of the record. The page header is a magic byte and a sequence number;
     the page with the higher sequence number wins the mount election, and a losing page that still carries a header
     is a leftover from an interrupted compaction and is erased.

     The mount scan walks the active page record by record. A record whose state byte is empty terminates the scan;
     if any byte after that point is not 0xFF, the tail is torn (a power cut hit mid-record) and the store schedules
     a compaction, which rewrites only complete valid records and thereby discards the debris. Reads scan the whole
     log and return the last valid record for the key, which makes a power cut between writing the new record and
     killing the old one harmless: both are valid for a moment, and the newer one wins by position.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as status codes and buffers.

 Supporting files:
     header      include/kv_store.h, include/flash_sim.h
=========================================================================================================================
*/
#include "kv_store.h"
#include "flash_sim.h"

#include <string.h>

#define HDR_MAGIC   0x5Au
#define HDR_SIZE    4u            /* magic, seq lo, seq hi, reserved.   */
#define ST_EMPTY    0xFFu
#define ST_VALID    0xAAu
#define ST_DEAD     0x00u
#define REC_FIXED   6u            /* state + key(2) + len + crc(2).     */

static uint32_t active_page;
static uint32_t write_off;        /* First free byte on the active page. */

/* CRC-16-CCITT, the same bitwise form used in the packet project. */
static uint16_t crc16(const uint8_t *d, size_t n)
{
    uint16_t c = 0xFFFFu;
    for (size_t i = 0; i < n; ++i) {
        c ^= (uint16_t)((uint16_t)d[i] << 8);
        for (int b = 0; b < 8; ++b) {
            if (c & 0x8000u) c = (uint16_t)((c << 1) ^ 0x1021u);
            else             c = (uint16_t)(c << 1);
        }
    }
    return c;
}

static uint16_t page_seq(uint32_t page, int *has_hdr)
{
    uint8_t h[HDR_SIZE];
    flash_read(page, 0, h, HDR_SIZE);
    *has_hdr = (h[0] == HDR_MAGIC);
    return (uint16_t)(h[1] | ((uint16_t)h[2] << 8));
}

static void write_header(uint32_t page, uint16_t seq)
{
    const uint8_t h[HDR_SIZE] = {
        HDR_MAGIC, (uint8_t)(seq & 0xFF), (uint8_t)(seq >> 8), 0xFF
    };
    flash_program(page, 0, h, HDR_SIZE);
}

/* Scan one record at 'off'. Returns record length, or 0 at log end /
   torn tail (reported via *torn). */
static uint32_t scan_record(uint32_t page, uint32_t off,
                            uint8_t *state, uint16_t *key,
                            uint8_t *len, int *torn)
{
    *torn = 0;
    if (off + REC_FIXED > FLASH_PAGE_SIZE) return 0;

    uint8_t hdr[4];
    flash_read(page, off, hdr, 4);
    *state = hdr[0];
    *key   = (uint16_t)(hdr[1] | ((uint16_t)hdr[2] << 8));
    *len   = hdr[3];

    if (*state == ST_EMPTY) {
        /* End of log only if the whole tail is still erased; any
           programmed byte after this point is a torn write. */
        uint8_t b;
        for (uint32_t i = off; i < FLASH_PAGE_SIZE; ++i) {
            flash_read(page, i, &b, 1);
            if (b != 0xFF) { *torn = 1; break; }
        }
        return 0;
    }
    if (*len > KV_MAX_VALUE ||
        off + REC_FIXED + *len > FLASH_PAGE_SIZE) {
        *torn = 1;                 /* Nonsense length: corrupted record. */
        return 0;
    }
    return REC_FIXED + *len;
}

static int record_crc_ok(uint32_t page, uint32_t off, uint8_t len)
{
    uint8_t buf[3 + KV_MAX_VALUE + 2];
    flash_read(page, off + 1, buf, (size_t)(3 + len + 2));
    const uint16_t stored =
        (uint16_t)(((uint16_t)buf[3 + len] << 8) | buf[3 + len + 1]);
    return crc16(buf, (size_t)(3 + len)) == stored;
}

/* Find the LAST valid record for a key; returns its offset or 0. */
static uint32_t find_latest(uint16_t key, uint8_t *len_out)
{
    uint32_t off = HDR_SIZE, found = 0;
    uint8_t state, len;
    uint16_t k;
    int torn;

    for (;;) {
        const uint32_t rlen = scan_record(active_page, off, &state, &k, &len, &torn);
        if (rlen == 0) break;
        if (state == ST_VALID && k == key &&
            record_crc_ok(active_page, off, len)) {
            found = off;
            *len_out = len;
        }
        off += rlen;
    }
    return found;
}

static kv_status_t append_record(uint32_t page, uint32_t off,
                                 uint16_t key, const uint8_t *val, uint8_t len)
{
    uint8_t body[3 + KV_MAX_VALUE + 2];
    body[0] = (uint8_t)(key & 0xFF);
    body[1] = (uint8_t)(key >> 8);
    body[2] = len;
    memcpy(&body[3], val, len);
    const uint16_t c = crc16(body, (size_t)(3 + len));
    body[3 + len]     = (uint8_t)(c >> 8);
    body[3 + len + 1] = (uint8_t)(c & 0xFF);

    /* Content first, then the state byte: the state is the commit
       point, so a power cut mid-record leaves it uncommitted. */
    if (flash_program(page, off + 1, body, (size_t)(3 + len + 2)) != FLASH_OK)
        return KV_IO_ERR;
    const uint8_t valid = ST_VALID;
    if (flash_program(page, off, &valid, 1) != FLASH_OK)
        return KV_IO_ERR;
    return KV_OK;
}

static kv_status_t compact(void)
{
    const uint32_t src = active_page;
    const uint32_t dst = 1u - src;
    int has_hdr;
    const uint16_t seq = page_seq(src, &has_hdr);

    if (flash_erase(dst) != FLASH_OK) return KV_IO_ERR;

    /* Copy the latest valid record of every live key. Outer scan visits
       each record; a key is copied only at its LAST valid occurrence. */
    uint32_t out = HDR_SIZE;
    uint32_t off = HDR_SIZE;
    uint8_t state, len;
    uint16_t key;
    int torn;

    for (;;) {
        const uint32_t rlen = scan_record(src, off, &state, &key, &len, &torn);
        if (rlen == 0) break;
        if (state == ST_VALID && record_crc_ok(src, off, len)) {
            uint8_t latest_len;
            if (find_latest(key, &latest_len) == off) {
                uint8_t val[KV_MAX_VALUE];
                flash_read(src, off + 4, val, len);
                if (out + REC_FIXED + len > FLASH_PAGE_SIZE) return KV_FULL;
                if (append_record(dst, out, key, val, len) != KV_OK)
                    return KV_IO_ERR;
                out += REC_FIXED + len;
            }
        }
        off += rlen;
    }

    /* Header last: until this point the mount election still chooses
       the source page, so a power cut anywhere above loses nothing. */
    write_header(dst, (uint16_t)(seq + 1));
    flash_erase(src);

    active_page = dst;
    write_off = out;
    return KV_OK;
}

kv_status_t kv_mount(void)
{
    int h0, h1;
    const uint16_t s0 = page_seq(0, &h0);
    const uint16_t s1 = page_seq(1, &h1);

    if (!h0 && !h1) {
        /* Blank device: format page 0. */
        flash_erase(0);
        write_header(0, 1);
        active_page = 0;
        write_off = HDR_SIZE;
        return KV_OK;
    }
    if (h0 && h1) {
        /* Interrupted compaction: the higher sequence wins, the loser
           is a stale source and is erased. */
        active_page = ((uint16_t)(s1 - s0) < 0x8000u) ? 1u : 0u;
        flash_erase(1u - active_page);
    } else {
        active_page = h0 ? 0u : 1u;
    }

    /* Walk the log to the end; a torn tail forces a cleaning compaction. */
    uint32_t off = HDR_SIZE;
    uint8_t state, len;
    uint16_t key;
    int torn;
    for (;;) {
        const uint32_t rlen = scan_record(active_page, off, &state, &key, &len, &torn);
        if (rlen == 0) break;
        off += rlen;
    }
    write_off = off;
    if (torn) return compact();
    return KV_OK;
}

kv_status_t kv_put(uint16_t key, const void *value, uint8_t len)
{
    if (!value || len == 0 || len > KV_MAX_VALUE) return KV_BAD_ARG;

    if (write_off + REC_FIXED + len > FLASH_PAGE_SIZE) {
        const kv_status_t rc = compact();
        if (rc != KV_OK) return rc;
        if (write_off + REC_FIXED + len > FLASH_PAGE_SIZE) return KV_FULL;
    }

    uint8_t old_len;
    const uint32_t old_off = find_latest(key, &old_len);

    const uint32_t off = write_off;
    const kv_status_t rc = append_record(active_page, off, key, value, len);
    if (rc != KV_OK) return rc;
    write_off = off + REC_FIXED + len;

    /* Kill the superseded record last: if power fails before this,
       both records are valid and reads pick the newer by position. */
    if (old_off != 0) {
        const uint8_t dead = ST_DEAD;
        flash_program(active_page, old_off, &dead, 1);
    }
    return KV_OK;
}

kv_status_t kv_get(uint16_t key, void *value, uint8_t cap, uint8_t *len_out)
{
    if (!value || !len_out) return KV_BAD_ARG;
    uint8_t len;
    const uint32_t off = find_latest(key, &len);
    if (off == 0) return KV_NOT_FOUND;
    if (len > cap) return KV_BAD_ARG;
    flash_read(active_page, off + 4, value, len);
    *len_out = len;
    return KV_OK;
}

kv_status_t kv_delete(uint16_t key)
{
    uint8_t len;
    const uint32_t off = find_latest(key, &len);
    if (off == 0) return KV_NOT_FOUND;
    const uint8_t dead = ST_DEAD;
    flash_program(active_page, off, &dead, 1);
    return KV_OK;
}
