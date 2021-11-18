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
#include "eeprom.h"

#define EEP_SIZE (E2END + 1)

uint8_t eepmem[EEP_SIZE];
bool eeprom_ready = true;   // can be used to wait for some test reason

void eep_reset(void)
{
    memset(eepmem, 0xFF, sizeof(eepmem));
}

uint16_t eeprom_read_word(const uint16_t *addr)
{
    uintptr_t idx = (uintptr_t)addr;
    uint16_t w = eepmem[idx] + (eepmem[idx+1] << 8);
    return w;
}

void eeprom_update_word(uint16_t *addr, uint16_t val)
{
    uintptr_t idx = (uintptr_t)addr;
    eepmem[idx] = val;
    eepmem[idx + 1] = val >> 8;
}

bool eeprom_is_ready(void)
{
    return eeprom_ready;
}
