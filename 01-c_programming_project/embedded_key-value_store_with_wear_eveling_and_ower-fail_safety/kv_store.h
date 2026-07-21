/*
=========================================================================================================================
 *** kv_store.h ***
 Log-structured key-value store on NOR flash: append-only records, two-page wear leveling, power-fail recovery.
=========================================================================================================================

 Description:
     This header declares a small persistent key-value store of the kind used for device settings and calibration
     data. The store is log-structured: every update appends a new record to the active page and then invalidates the
     previous record for that key by clearing its state byte, an operation NOR flash allows without an erase. Records
     carry a CRC-16 over their content, and the record state byte is programmed only after the content, so a power
     cut at any byte leaves either a complete valid record or a torn one that the mount scan recognizes and discards;
     committed data is never lost.

     Two flash pages alternate in the active role. When the active page cannot hold a new record, the live records
     are compacted onto the other page, which then takes over through a page header carrying a monotonically
     increasing sequence number; the old page is erased afterwards. The ordering of the compaction steps (erase
     target, copy live records, write the new header, erase the source) guarantees that at every possible power-cut
     instant exactly one page wins the mount election, and the alternation spreads erase wear evenly across the
     pages, which the test suite measures through the emulator's erase counters.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/kv_store.c
     tests       tests/test_kv.c (semantics, persistence, compaction, wear leveling, power-fail recovery)
=========================================================================================================================
*/
#ifndef KV_STORE_H
#define KV_STORE_H

#include <stddef.h>
#include <stdint.h>

#define KV_MAX_VALUE 64u

typedef enum {
    KV_OK = 0,
    KV_NOT_FOUND,
    KV_FULL,          /* Live data no longer fits even after compaction. */
    KV_BAD_ARG,
    KV_IO_ERR
} kv_status_t;

/* Mount the store: elect the active page, clean up interrupted
   compactions and torn record tails. Formats blank flash on first use. */
kv_status_t kv_mount(void);

kv_status_t kv_put(uint16_t key, const void *value, uint8_t len);
kv_status_t kv_get(uint16_t key, void *value, uint8_t cap, uint8_t *len_out);
kv_status_t kv_delete(uint16_t key);

#endif /* KV_STORE_H */
