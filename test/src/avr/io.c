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

#include "io.h"

// functional code and variables representing the AVR IO ports

volatile uint8_t *reg8_eval(struct reg8 *r)
{
    volatile uint8_t *ret = &r->data[r->idx];
    if (r->idx < sizeof(r->data)) {
        ++r->idx;
    }
    return ret;
}
void reg8_init(struct reg8 *r)
{
    for (unsigned int i = 0; i < sizeof(r->data); ++i) {
        r->data[i] = 0;
    }
    r->idx = 0;
}

#define REG8_DEF(regname) struct reg8 regname ## _reg8 = { \
    .idx = 0, .eval = reg8_eval, .reset = reg8_init }

REG8_DEF(PORTB);
REG8_DEF(PORTC);
REG8_DEF(PORTD);

REG8_DEF(PINB);
REG8_DEF(PIND);

REG8_DEF(DDRB);
REG8_DEF(DDRC);
REG8_DEF(DDRD);

REG8_DEF(MCUSR);

REG8_DEF(CANGCON);
REG8_DEF(CANPAGE);
REG8_DEF(CANEN2);
REG8_DEF(CANCDMOB);
REG8_DEF(CANMSG);
REG8_DEF(CANSTMOB);

REG8_DEF(CANBT1);
REG8_DEF(CANBT2);
REG8_DEF(CANBT3);

REG8_DEF(CANIDM1);
REG8_DEF(CANIDM2);
REG8_DEF(CANIDM3);
REG8_DEF(CANIDM4);

REG8_DEF(CANIDT1);
REG8_DEF(CANIDT2);
REG8_DEF(CANIDT3);
REG8_DEF(CANIDT4);

void reset_all(void)
{
    PORTB_reg8.reset(&PORTB_reg8);
    PORTC_reg8.reset(&PORTC_reg8);
    PORTD_reg8.reset(&PORTD_reg8);
    PINB_reg8.reset(&PINB_reg8);
    PIND_reg8.reset(&PIND_reg8);
    DDRB_reg8.reset(&DDRB_reg8);
    DDRC_reg8.reset(&DDRC_reg8);
    DDRD_reg8.reset(&DDRD_reg8);
    MCUSR_reg8.reset(&MCUSR_reg8);
    CANGCON_reg8.reset(&CANGCON_reg8);
    CANPAGE_reg8.reset(&CANPAGE_reg8);
    CANEN2_reg8.reset(&CANEN2_reg8);
    CANCDMOB_reg8.reset(&CANCDMOB_reg8);
    CANMSG_reg8.reset(&CANMSG_reg8);
    CANSTMOB_reg8.reset(&CANSTMOB_reg8);
    CANBT1_reg8.reset(&CANBT1_reg8);
    CANBT2_reg8.reset(&CANBT2_reg8);
    CANBT3_reg8.reset(&CANBT3_reg8);
    CANIDM1_reg8.reset(&CANIDM1_reg8);
    CANIDM2_reg8.reset(&CANIDM2_reg8);
    CANIDM3_reg8.reset(&CANIDM3_reg8);
    CANIDM4_reg8.reset(&CANIDM4_reg8);
    CANIDT1_reg8.reset(&CANIDT1_reg8);
    CANIDT2_reg8.reset(&CANIDT2_reg8);
    CANIDT3_reg8.reset(&CANIDT3_reg8);
    CANIDT4_reg8.reset(&CANIDT4_reg8);
}

void cli(void)
{
}
