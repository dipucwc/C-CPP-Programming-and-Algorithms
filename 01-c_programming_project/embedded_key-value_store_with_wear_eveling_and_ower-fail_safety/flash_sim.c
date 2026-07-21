/*
=========================================================================================================================
 *** flash_sim.c ***
 Implementation of the NOR emulator: AND-semantics programming, page erase with wear counting, and the fault countdown.
=========================================================================================================================

 Description:
     This file implements the emulator declared in flash_sim.h. The program operation AND-s each incoming byte into
     the array byte by byte, which is precisely the physics of NOR programming: bits can be cleared but never set.
     The fault countdown is decremented per programmed byte and, on reaching zero, the operation stops and reports a
     power failure, leaving every byte programmed so far in place, which reproduces the torn-write state a real
     power cut leaves behind.

 Input:
     (none)         Parameters arrive through the function interfaces.

 Output:
     (none)         Results are returned as status codes and buffers.

 Supporting files:
     header      include/flash_sim.h
=========================================================================================================================
*/
#include "flash_sim.h"

#include <string.h>

static uint8_t  mem[FLASH_PAGES][FLASH_PAGE_SIZE];
static uint32_t erase_cnt[FLASH_PAGES];
static long     fail_after = -1;

void flash_reset(void)
{
    memset(mem, 0xFF, sizeof mem);
    memset(erase_cnt, 0, sizeof erase_cnt);
    fail_after = -1;
}

flash_status_t flash_erase(uint32_t page)
{
    if (page >= FLASH_PAGES) return FLASH_BAD_ARG;
    memset(mem[page], 0xFF, FLASH_PAGE_SIZE);
    erase_cnt[page]++;
    return FLASH_OK;
}

flash_status_t flash_program(uint32_t page, uint32_t offset,
                             const uint8_t *data, size_t len)
{
    if (page >= FLASH_PAGES || !data ||
        offset + len > FLASH_PAGE_SIZE) return FLASH_BAD_ARG;

    for (size_t i = 0; i < len; ++i) {
        if (fail_after == 0) return FLASH_POWER_FAIL;   /* Torn write. */
        if (fail_after > 0) fail_after--;
        /* NOR physics: programming can only clear bits, so the write
           lands as the AND of old and new data. Illegal overwrites
           corrupt silently, exactly as on the real device. */
        mem[page][offset + i] &= data[i];
    }
    return FLASH_OK;
}

flash_status_t flash_read(uint32_t page, uint32_t offset,
                          uint8_t *dst, size_t len)
{
    if (page >= FLASH_PAGES || !dst ||
        offset + len > FLASH_PAGE_SIZE) return FLASH_BAD_ARG;
    memcpy(dst, &mem[page][offset], len);
    return FLASH_OK;
}

uint32_t flash_erase_count(uint32_t page)
{
    return (page < FLASH_PAGES) ? erase_cnt[page] : 0;
}

void flash_set_fail_after(long bytes)
{
    fail_after = bytes;
}
