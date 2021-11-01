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
#include <stddef.h>
#include <avr/io.h>

#define F_CPU 8000000UL
#include <util/delay.h>

// Port Configuration
//
// This configuration is for a Zeva BMS-24 board.
//
//  | Port  |IO |PU | Usage                                 |
//  |-------|---|---|---------------------------------------|
//  | PB0   | I | - | NC                                    |
//  | PB1   | I | - | NC                                    |
//  | PB2   | I |PU | ID encoder position 2                 |
//  | PB3   | O | - | LTC1 SCK (not used for bootloader)    |
//  | PB4   | I | - | 5V (why?)                             |
//  | PB6   | O | - | LTC2 CS                               |
//  | PB7   | O | - | LTC2 SDI                              |
//  | PC0   | I | - | NC                                    |
//  | PC1   | O | - | Green LED                             |
//  | PC2   | O | - | CAN TX                                |
//  | PC3   | I | - | CAN RX                                |
//  | PC4   | I | - | LTC1 SDO                              |
//  | PC5   | O | - | LTC1 CS                               |
//  | PC6   | O | - | LTC1 SDI                              |
//  | PC7   | I | - | NC                                    |
//  | PD0   | O | - | LTC2 SCK                              |
//  | PD1   | I | - | tied to reset line (why?)             |
//  | PD2   | O | - | prog MISO (needs configured?)         |
//  | PD3   |I/O| - | prog MOSI / Red LED                   |
//  | PD4   | I | - | prog SCK                              |
//  | PD5   | I |PU | ID encoder position 8                 |
//  | PD6   | I |PU | ID encoder position 1                 |
//  | PD7   | I |PU | ID encoder position 4                 |
//  | PE0   | I | - | RESET/                                |
//  | PE1   | I | - | XTAL                                  |
//  | PE2   | I | - | XTAL                                  |
//
// Notes:
// - Battery chips LTCx are not used for boot loader
// - Progamming pin functions are automatic so they dont need to be
//   configured (PD2 not set to output)
// - Even though PD3 is programming input, it is LED output so should
//   be configured as output
//

static void device_init(void)
{
    // set GPIO directions
    DDRB = 0;                       // LTC outputs not enabled for boot loader
    DDRC = _BV(DDC1) | _BV(DDC2);   // CAN TX and green LED
    DDRD = _BV(DDD3);               // Red LED

    // set pullups for ID encoder
    PORTB = _BV(PORTB2);
    PORTC = 0;
    PORTD = _BV(PORTD5) | _BV(PORTD6) | _BV(PORTD7);

    // CAN init
    // reset CAN controller
    CANGCON = _BV(SWRES);
    // a delay is needed after reset - not sure how long is required
    _delay_ms(1);

    // CAN timing for 250 khz, TQ=0.5, taken from data sheet table, 8MHz clk
    CANBT1 = 0x06;
    CANBT2 = 0x04;
    CANBT3 = 0x13;

    // disable all the MOBs before enabling controller
    for (uint8_t i = 0; i < 6; ++i)
    {
        CANPAGE = i << MOBNB0;  // select MOB
        CANCDMOB = 0;           // disable it
        CANSTMOB = 0;           // clear all status
    }

    // set up MOB1 to receive
    CANPAGE = 1 << MOBNB0;      // select MOB1
    // set up CAN ID 2
    CANIDT4 = 0;
    CANIDT3 = 0;
    CANIDT2 = 2 << 5;   // CAN address
    CANIDT1 = 0;
    CANIDM4 = 0;
    CANIDM3 = 0;
    CANIDM2 = 0xE0;     // force address match
    CANIDM1 = 0xFF;

    // enable receive
    CANCDMOB = _BV(CONMOB1) | 2;

    // enable CAN controller
    CANGCON = _BV(ENASTB);
}

static void send_message(uint8_t len, const uint8_t *pmsg)
{
    // wait for MOB0 to be not busy
    while (CANEN2 & _BV(ENMOB0))
    {}

    // select MOB0
    uint8_t cansave = CANPAGE;
    CANPAGE = 0;

    // clear any lingering status
    CANSTMOB = 0;

    // set up CAN ID - use ID=1, 11 bit
    CANIDT4 = 0;
    CANIDT3 = 0;
    CANIDT2 = 1 << 5;   // CAN address
    CANIDT1 = 0;

    // load some bytes into the data field
    for (uint8_t i = 0; i < len; ++i)
    {
        CANMSG = pmsg[i];
    }

    // enable the MOB for transmission
    CANCDMOB = _BV(CONMOB0) | len;  // IDE=11-bit, DLC

    // wait for transmission complete
    while (!CANSTMOB)
    {}

    // disable the MOB and clear status
    CANCDMOB = 0;
    CANSTMOB = 0;
    CANPAGE = cansave;
}

static void rx_can(void)
{
    // select MOB1 for rx
    uint8_t cansave = CANPAGE;
    CANPAGE = 1 << MOBNB0;

    // wait until something is received
    while (CANEN2 & _BV(ENMOB1))
    {}
//    while (!(CANSTMOB & _BV(RXOK)))
//    {}

    // assume we got the message
    // clear the status
    CANSTMOB = 0;

    // enable receive
    CANCDMOB = _BV(CONMOB1) | 2;
    CANPAGE = cansave;
}

static uint8_t msgbuf[8];

#if 0
static void debug_dump(void)
{
    msgbuf[0] = CANGSTA;
    msgbuf[1] = CANGIT;
    msgbuf[2] = CANEN2;

    uint8_t cansave = CANPAGE;
    CANPAGE = 1 << MOBNB0;
    msgbuf[3] = CANPAGE;

    msgbuf[4] = CANSTMOB;
    msgbuf[5] = CANCDMOB;
    CANPAGE = cansave;

    msgbuf[6] = 0;
    msgbuf[7] = 0;

    send_message(8, msgbuf);
}
#endif

int main(void)
{
    uint8_t rxcount = 0;

    device_init();

    for (;;)
    {
        // turn on red LED
        PORTC &= ~_BV(PORTC1);  // green off
        PORTD |= _BV(PORTD3);

        // wait for a message
        rx_can();

        // turn off red LED and turn on Green
        PORTD &= ~_BV(PORTD3);
        PORTC |= _BV(PORTC1);

        // send a reply
        msgbuf[0] = ++rxcount;
        send_message(1, msgbuf);

        // wait a bit then start over
        _delay_ms(1000);
    }

    return 0;
}
