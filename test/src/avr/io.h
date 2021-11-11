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

#ifndef __IO_H__
#define __IO_H__

// substitute header file for the AVR avr/io.h
// a place to declare all IO registers and functions
//
// the port and IO registers are implemented in a way that the values can be
// controller during a unit test. Each "register" has a backing data store
// and all app reads and writes to these registers use the store (array)
// to either store the written data or read out the next data value.
//
// Every IO port has a "reg8" structure that holds the data and fn pointers
// for operations. Using PORTB as an example, they are named like PORTB_reg8.
// The reset function resets the data index and clears the store.
// The eval function returns a dereferenced pointer to backing data store.
// The store is indexed each time the IO port is accessed using the eval
// function. If it gets to the end of the store array, then the index no
// longer increments.
//
// If a sequence of values are written to the IO register by the code under
// test, those values are stored in the array and can be tested for
// correctness. If a known sequence should be read by the code under test,
// then the array can be filled with the expected value sequence before the
// function to be tested is called.
//
// Examples, using PORTB:
//
// PORTB_reg8.reset(&PORTB_reg8); // reset the IO port and clear data
//
// PORTB_reg8.data[0] = 0x5a;     // set value for first read by app code
//
// TEST_ASSERT(PORTB_reg8.data[1] == 42); // check expected value of second access
//
// TEST_ASSERT(PORTB_reg8.idx == 3); // verify there were 3 accesses
//
// There is a convenience macro for reset:
//
// reg8_reset(PORTB);
//
// For example, for PORTB:
//
// - reset the data store:
//   PORTB_reg8.reset(&PORTB_reg8);

struct reg8 {
    unsigned int idx;
    uint8_t data[32];
    volatile uint8_t *(*eval)(struct reg8 *);
    void (*reset)(struct reg8 *);
};

#define reg8_reset(regname) regname ## _reg8.reset(& regname ## _reg8)

extern void reset_all(void);

// standard macro from AVR header
#define _BV(bit) (1 << (bit))

// define all the IO ports used in the app code

#define PORTB (*PORTB_reg8.eval(&PORTB_reg8))
extern struct reg8 PORTB_reg8;

#define PORTB2 2
#define PB2 2

#define PINB (*PINB_reg8.eval(&PINB_reg8))
extern struct reg8 PINB_reg8;

#define DDRB (*DDRB_reg8.eval(&DDRB_reg8))
extern struct reg8 DDRB_reg8;

#define PORTC (*PORTC_reg8.eval(&PORTC_reg8))
extern struct reg8 PORTC_reg8;

#define DDRC (*DDRC_reg8.eval(&DDRC_reg8))
extern struct reg8 DDRC_reg8;
#define DDC1 1
#define DDC2 2

#define PORTD (*PORTD_reg8.eval(&PORTD_reg8))
extern struct reg8 PORTD_reg8;
#define PORTD3 3
#define PORTD5 5
#define PORTD6 6
#define PORTD7 7

#define PD5 5
#define PD6 6
#define PD7 7

#define PIND (*PIND_reg8.eval(&PIND_reg8))
extern struct reg8 PIND_reg8;

#define DDRD (*DDRD_reg8.eval(&DDRD_reg8))
extern struct reg8 DDRD_reg8;
#define DDD3 3

#define MCUSR (*MCUSR_reg8.eval(&MCUSR_reg8))
extern struct reg8 MCUSR_reg8;
#define WDRF 3

#define CANGCON (*CANGCON_reg8.eval(&CANGCON_reg8))
extern struct reg8 CANGCON_reg8;
#define SWRES 0
#define ENASTB 1

#define CANEN2 (*CANEN2_reg8.eval(&CANEN2_reg8))
extern struct reg8 CANEN2_reg8;
#define ENMOB0 0

#define CANBT1 (*CANBT1_reg8.eval(&CANBT1_reg8))
extern struct reg8 CANBT1_reg8;
#define CANBT2 (*CANBT2_reg8.eval(&CANBT2_reg8))
extern struct reg8 CANBT2_reg8;
#define CANBT3 (*CANBT3_reg8.eval(&CANBT3_reg8))
extern struct reg8 CANBT3_reg8;

#define CANPAGE (*CANPAGE_reg8.eval(&CANPAGE_reg8))
extern struct reg8 CANPAGE_reg8;
#define MOBNB0 4

#define CANCDMOB (*CANCDMOB_reg8.eval(&CANCDMOB_reg8))
extern struct reg8 CANCDMOB_reg8;
#define IDE 4
#define CONMOB0 6
#define CONMOB1 7

#define CANSTMOB (*CANSTMOB_reg8.eval(&CANSTMOB_reg8))
extern struct reg8 CANSTMOB_reg8;
#define RXOK 5

#define CANIDT1 (*CANIDT1_reg8.eval(&CANIDT1_reg8))
extern struct reg8 CANIDT1_reg8;
#define CANIDT2 (*CANIDT2_reg8.eval(&CANIDT2_reg8))
extern struct reg8 CANIDT2_reg8;
#define CANIDT3 (*CANIDT3_reg8.eval(&CANIDT3_reg8))
extern struct reg8 CANIDT3_reg8;
#define CANIDT4 (*CANIDT4_reg8.eval(&CANIDT4_reg8))
extern struct reg8 CANIDT4_reg8;
#define IDT0 3
#define IDT4 7

#define CANIDM1 (*CANIDM1_reg8.eval(&CANIDM1_reg8))
extern struct reg8 CANIDM1_reg8;
#define CANIDM2 (*CANIDM2_reg8.eval(&CANIDM2_reg8))
extern struct reg8 CANIDM2_reg8;
#define CANIDM3 (*CANIDM3_reg8.eval(&CANIDM3_reg8))
extern struct reg8 CANIDM3_reg8;
#define CANIDM4 (*CANIDM4_reg8.eval(&CANIDM4_reg8))
extern struct reg8 CANIDM4_reg8;

extern struct reg8 CANMSG_reg8;
#define CANMSG (*CANMSG_reg8.eval(&CANMSG_reg8))

#endif
