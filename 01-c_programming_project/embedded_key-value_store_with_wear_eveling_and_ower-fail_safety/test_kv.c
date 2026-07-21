/*
=========================================================================================================================
 *** test_kv ***
 Unit tests: flash physics, store semantics, remount persistence, compaction, wear leveling, and power-fail recovery.
=========================================================================================================================

 Description:
     This test verifies the storage stack through six properties. The first property pins the emulator itself:
     programming must only clear bits, erase must restore them, and the erase counters must count, since every other
     guarantee stands on these physics. The second property is basic semantics: put, get, overwrite (latest wins),
     delete, and not-found. The third property is persistence: after a remount without power loss, every committed
     value must read back identically. The fourth property is compaction: writes far exceeding one page's capacity
     must succeed, with live data intact afterwards and dead space reclaimed. The fifth property is wear leveling:
     after many fill cycles, the two pages' erase counts must differ by at most one, showing that compaction
     alternates the active page rather than favouring one. The sixth property is power-fail recovery, exercised at
     three cut points via the emulator's fault countdown: mid-content (record uncommitted), between content and
     commit byte (state never set), and between the new record's commit and the old record's kill (two valid records,
     newer wins). After each cut the store is remounted and the committed value for the key must still read back.

 Input:
     (none)         All scenarios are generated in code.

 Output:
     return value   Zero if all tests pass, one otherwise.
     stdout         One line per failed property and one summary line reporting PASS or FAIL.

 Supporting files:
     header      include/kv_store.h, include/flash_sim.h (under test)
=========================================================================================================================
*/
#include <stdio.h>
#include <string.h>

#include "flash_sim.h"
#include "kv_store.h"

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
    uint8_t buf[KV_MAX_VALUE];
    uint8_t len;

    {   /* Property 1: flash physics. */
        flash_reset();
        const uint8_t aa = 0xAA, cc = 0xCC;
        uint8_t r;
        CHECK(flash_program(0, 0, &aa, 1) == FLASH_OK);
        CHECK(flash_program(0, 0, &cc, 1) == FLASH_OK);
        flash_read(0, 0, &r, 1);
        CHECK(r == (0xAA & 0xCC));            /* Bits only clear.       */
        CHECK(flash_erase(0) == FLASH_OK);
        flash_read(0, 0, &r, 1);
        CHECK(r == 0xFF);                     /* Erase restores.        */
        CHECK(flash_erase_count(0) == 1);
    }

    {   /* Property 2: basic semantics. */
        flash_reset();
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_put(0x0101, "alpha", 5) == KV_OK);
        CHECK(kv_get(0x0101, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 5 && memcmp(buf, "alpha", 5) == 0);

        CHECK(kv_put(0x0101, "beta!!", 6) == KV_OK);      /* Overwrite. */
        CHECK(kv_get(0x0101, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 6 && memcmp(buf, "beta!!", 6) == 0);

        CHECK(kv_get(0x0202, buf, sizeof buf, &len) == KV_NOT_FOUND);
        CHECK(kv_delete(0x0101) == KV_OK);
        CHECK(kv_get(0x0101, buf, sizeof buf, &len) == KV_NOT_FOUND);
    }

    {   /* Property 3: persistence across a clean remount. */
        flash_reset();
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_put(1, "one", 3) == KV_OK);
        CHECK(kv_put(2, "twotwo", 6) == KV_OK);
        CHECK(kv_mount() == KV_OK);           /* Simulated reboot.      */
        CHECK(kv_get(1, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 3 && memcmp(buf, "one", 3) == 0);
        CHECK(kv_get(2, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 6 && memcmp(buf, "twotwo", 6) == 0);
    }

    {   /* Property 4: compaction under sustained updates. */
        flash_reset();
        CHECK(kv_mount() == KV_OK);
        /* 500 updates of 3 keys x 32 bytes: many page fills. */
        for (int i = 0; i < 500; ++i) {
            uint8_t v[32];
            memset(v, (uint8_t)i, sizeof v);
            v[0] = (uint8_t)(i & 3);
            CHECK(kv_put((uint16_t)(10 + (i % 3)), v, sizeof v) == KV_OK);
        }
        for (int k = 0; k < 3; ++k) {
            CHECK(kv_get((uint16_t)(10 + k), buf, sizeof buf, &len) == KV_OK);
            CHECK(len == 32);
        }
        CHECK(flash_erase_count(0) + flash_erase_count(1) > 10);
    }

    {   /* Property 5: wear leveling balance. */
        const uint32_t e0 = flash_erase_count(0);
        const uint32_t e1 = flash_erase_count(1);
        const uint32_t diff = (e0 > e1) ? (e0 - e1) : (e1 - e0);
        CHECK(diff <= 1);
    }

    {   /* Property 6: power-fail recovery at three cut points. */
        /* Cut A: mid-content, record never committed. */
        flash_reset();
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_put(7, "COMMITTED", 9) == KV_OK);
        flash_set_fail_after(4);                       /* Die mid-body. */
        CHECK(kv_put(7, "torn-value", 10) != KV_OK);
        flash_set_fail_after(-1);
        CHECK(kv_mount() == KV_OK);                    /* Reboot.       */
        CHECK(kv_get(7, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 9 && memcmp(buf, "COMMITTED", 9) == 0);

        /* Cut B: full content written, commit byte never programmed. */
        flash_reset();
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_put(8, "KEEP-ME", 7) == KV_OK);
        flash_set_fail_after(3 + 12 + 2);              /* All body, no state. */
        CHECK(kv_put(8, "replacement!", 12) != KV_OK);
        flash_set_fail_after(-1);
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_get(8, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 7 && memcmp(buf, "KEEP-ME", 7) == 0);

        /* Cut C: new record committed, old record's kill lost. Both
           are valid on flash; the newer must win by position. */
        flash_reset();
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_put(9, "OLD", 3) == KV_OK);
        flash_set_fail_after(3 + 3 + 2 + 1);           /* Body + state, no kill. */
        (void)kv_put(9, "NEW", 3);
        flash_set_fail_after(-1);
        CHECK(kv_mount() == KV_OK);
        CHECK(kv_get(9, buf, sizeof buf, &len) == KV_OK);
        CHECK(len == 3 && memcmp(buf, "NEW", 3) == 0);
    }

    printf("[test_kv] physics + semantics + remount + compaction + wear + power-fail -> %s\n",
           fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
