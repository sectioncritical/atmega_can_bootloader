/******************************************************************************
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2021 Joseph Kroesche
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "io.h"
#include "boot.h"

#define FLASH_SIZE (FLASHEND + 1)


// represent flash temp buffer and flash memory.
// they are physically 16-bit so represented that way here
uint16_t flashbuf[SPM_PAGESIZE/2];
uint16_t flashmem[FLASH_SIZE/2];

// a variable that can be used by test to cause flash wait
bool flash_busy = false;

bool flash_rww_enabled = true;

void flash_reset(void)
{
    memset(flashbuf, 0xff, sizeof(flashbuf));
    memset(flashmem, 0xff, sizeof(flashmem));
}

// addr is defined to be a byte address, even though flash buf can only
// be addressed by word
void boot_page_fill_safe(uint16_t addr, uint16_t w)
{
    uint16_t waddr = (addr & (SPM_PAGESIZE - 1)) >> 1;
    flashbuf[waddr] = w;
}

void boot_page_erase_safe(uint16_t addr)
{
    uint16_t page = addr / SPM_PAGESIZE;    // page number
    uint16_t waddr = page * SPM_PAGESIZE;   // get page start address (word)
    memset(&flashmem[waddr], 0xff, SPM_PAGESIZE / 2);
    flash_rww_enabled = false;
}

void boot_page_write_safe(uint16_t addr)
{
    uint16_t page = addr / SPM_PAGESIZE;    // page number
    uint16_t waddr = page * SPM_PAGESIZE;   // get page start address (word)
    memcpy(&flashmem[waddr], flashbuf, SPM_PAGESIZE / 2);
    memset(flashbuf, 0xff, sizeof(flashbuf)); // clear the buffer for next use
    flash_rww_enabled = false;
}

void boot_spm_busy_wait(void)
{
    while (flash_busy) {
    }
}

void boot_rww_enable(void)
{
    flash_rww_enabled = true;
}
