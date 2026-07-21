/*
=========================================================================================================================
 *** flash_sim.h ***
 NOR-flash emulator enforcing real device semantics: erase-before-write, bit-clearing programs, and fault injection.
=========================================================================================================================

 Description:
     This header declares a small NOR-flash emulator used as the storage layer of the key-value store and as a test
     instrument. The emulator enforces the two constraints that make flash storage design nontrivial. Erasing is
     page-granular and sets every byte to 0xFF, and it is the only operation that can raise bits. Programming can
     only clear bits from one to zero, which the emulator enforces by AND-ing the written data into the array, so a
     driver that tries to overwrite programmed data with conflicting bits gets silently corrupted data exactly as it
     would on the real device, and the test suite can prove the store never relies on illegal writes.

     Two instrumentation features support the tests. A per-page erase counter measures wear, which is what the
     wear-leveling test asserts on. A programmable fault countdown aborts programming after a chosen number of bytes,
     emulating power loss in the middle of a write, which is what the power-fail recovery tests are built on.

 Input:
     (none)         Interfaces only.

 Output:
     (none)         Interfaces only.

 Supporting files:
     source      src/flash_sim.c
=========================================================================================================================
*/
#ifndef FLASH_SIM_H
#define FLASH_SIM_H

#include <stddef.h>
#include <stdint.h>

#define FLASH_PAGES     2u
#define FLASH_PAGE_SIZE 512u

typedef enum {
    FLASH_OK = 0,
    FLASH_BAD_ARG,
    FLASH_POWER_FAIL     /* The injected fault fired mid-program. */
} flash_status_t;

void           flash_reset(void);                     /* All pages erased, counters zeroed. */
flash_status_t flash_erase(uint32_t page);
flash_status_t flash_program(uint32_t page, uint32_t offset,
                             const uint8_t *data, size_t len);
flash_status_t flash_read(uint32_t page, uint32_t offset,
                          uint8_t *dst, size_t len);

uint32_t flash_erase_count(uint32_t page);

/* Fault injection: abort programming after 'bytes' further programmed
   bytes (simulated power loss). Pass a negative value to disarm. */
void flash_set_fail_after(long bytes);

#endif /* FLASH_SIM_H */
