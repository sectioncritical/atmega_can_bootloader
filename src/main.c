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

#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/crc16.h>

// search for PORTING, for notes about possible changes to port the boot loader

// PORTING: set the frequency to match the hardware setup
#define F_CPU 8000000UL
#include <util/delay.h>

// CAN ID that must match to receive a message.
// The mask shows the bits that must match. This leaves the lower 4 bits of
// boot loader command available to match on any 4 bit command value. The
// board ID portion (bits 7:4) will be replaced at run time with the
// board ID.
#define CANID       0x1B007100UL
#define CANIDMASK   0x1FFFFFF0UL

// define timeouts used when waiting for messages
// units are milliseconds
#define BOOT_TIMEOUT 2000U
#define ACTIVITY_TIMEOUT 10000U

// define EEPROM locations for image info
// this is 2 words (4 bytes total) at the end of the eeprom space
// Use the end so that the app can use eeprom from the start
#define EEP_APP_LEN ((uint16_t *)(E2END - 3))
#define EEP_APP_CRC ((uint16_t *)(E2END - 1))

// BOOTVER should be defined when firmware is built
// a placeholder is used if it is not defined. The placeholder means
// development, non-production version
#ifndef BOOTVER
#define BOOTVER 0x636363UL
#endif
static const uint8_t version[3] = {
    (uint8_t)(BOOTVER >> 16), (uint8_t)(BOOTVER >> 8), (uint8_t)BOOTVER
};

/** Boot loader command definitions. */
enum CmdId {
    CMD_PING = 0,   ///< Check for boot loader device presence
    CMD_REBOOT,     ///< Cause the target to reset (not implemented)
    CMD_START,      ///< Start a program load
    CMD_DATA,       ///< Send 8 bytes of program data
    CMD_STOP,       ///< Finish program load and provide CRC
    CMD_REPORT,     ///< Reply from boot loader to all commands
};

/** Boot loader report definitions. */
enum RptId {
    RPT_PONG = 0,   ///< Reply to PING
    RPT_READY,      ///< Ready for next DATA block
    RPT_END,        ///< All expected DATA blocks have been received
    RPT_DONE,       ///< Acknowledge completion of load success or failure
    RPT_BOOT,       ///< Acknowledge imminent reboot (not used)
    RPT_ERR,        ///< Bad command or other error condition
};

/** Receive message status. */
enum RcvStatus {
    MSG_NONE = 0,   ///< No message is available
    MSG_READY,      ///< A new message was received
    MSG_ERROR       ///< An error occurred recieving a message
};

// convenience macros for manipulating CAN page register
// some code space could be saved by not saving and restoring CANPAGE in
// various places. The code is all single thread with no interrupts, so it is
// probably not necessary to save/restore the CANPAGE where it is used
#define SAVE_CANPAGE uint8_t _cansave = CANPAGE
#define RESTORE_CANPAGE CANPAGE = _cansave
#define SET_CANPAGE(p) do { CANPAGE = (p) << MOBNB0; } while (0)

// convenience macros for manipulating LED used for signalling state
// some code space could be saved by not using the LED
// PORTING: change to match the GPIO used for the LED, or define to void
// if no LED is used
#define LED_ON()        do { PORTD |= _BV(PORTD3); } while (0)
#define LED_OFF()       do { PORTD &= ~_BV(PORTD3); } while (0)
#define LED_TOGGLE()    do { PIND = _BV(PORTD3); } while (0)

/** Command ID of received message.
 *
 * This is valid after `receive_message()` returned `MSG_READY`, and before
 * `receive_message()` or `send_message()` is called again.
 */
static enum CmdId cmdid;

/** Length in bytes (DLC) of received message.
 *
 * This is valid after `receive_message()` returned `MSG_READY`, and before
 * `receive_message()` or `send_message()` is called again.
 */
static uint8_t msglen = 0;

/** Payload bytes of received or sent messages.
 *
 * This is used to buffer payload data for incoming CAN messages. It holds
 * received message data after `receive_message()` has been called and returned
 * `MSG_READY`. It will be overwritten the next time `receive_message()` is
 * called.
 */
static uint8_t msgbuf[8];

/** Payload bytes for a REPORT message.
 *
 * This buffer is used to hold the REPORT response bytes. Some of the values
 * are fixed and filled in advance during C startup.
 */
static uint8_t rptbuf[8] = {
    (uint8_t)(BOOTVER >> 16),
    (uint8_t)(BOOTVER >> 8),
    (uint8_t)BOOTVER,               // version is fixed
    1,                              // general status set to 1
    0,                              // report type placeholder
    0, 0, 0                         // spare bytes
};

/** Receive message counter. Rolls over. */
static uint8_t rxcount = 0;

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

// if unit testing, dont use section attributes on the special
// variable and function below. these only have meaning on actual
// AVR target and not on host building a unit test
#ifndef UNIT_TEST
#define ATTRIBUTE(a) __attribute__ (a)
#else
#define ATTRIBUTE(a)
#endif

// storage for the reset cause, determined early in C init code
// the ".noinit" tells compiler to not overwrite during bss init
// This must be volatile to force the compiler to store it in memory.
// Otherwise it just uses a register which gets overwritten during init,
// before the variable can be used.
volatile uint8_t reset_cause ATTRIBUTE((section (".noinit")));

// Runs early in C startup
// Reads the MCUSR to determine reset cause, stores the value and
// clears the reg (per the data sheet)
// This is treated as part of the C init sequence and is not a callable
// function.
void get_reset_cause(void) ATTRIBUTE((naked, used, section(".init3")));
void get_reset_cause(void)  // cppcheck-suppress[unusedFunction]
{
    reset_cause = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

/** Get the assigned 4-bit board ID
 *
 * PORTING: this function can be changed to support other board ID schemes.
 * For example, the board ID could be stored in EEPROM.
 */
static uint8_t get_boardid(void)
{
    // THIS IMPLEMENTATION IS SPECIFIC TO ZEVA BMS-24 BOARDS
    //
    // Because the encoder bits are scattered among GPIOs and not in order
    // on a port, there is not an easier way to do this other than to check
    // every bit.
    uint8_t boardid = 0;
    if (!(PIND & _BV(PD5))) {
        boardid += 8;
    }
    if (!(PIND & _BV(PD7))) {
        boardid += 4;
    }
    if (!(PINB & _BV(PB2))) {
        boardid += 2;
    }
    if (!(PIND & _BV(PD6))) {
        boardid += 1;
    }
    return boardid;
}

/** Initialize the MCU GPIO and CAN peripheral */
static void device_init(void)
{
    // PORTING: set the GPIO configurations here to match the hardware
    //
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

    // PORTING: the CAN timing register values need to be adjusted to
    // match the clock frequency, if not 8 MHz. Also the CAN bus rate can be
    // changed here.
    //
    // CAN timing for 250 khz, TQ=0.5, taken from data sheet table, 8MHz clk
    CANBT1 = 0x06;
    CANBT2 = 0x04;
    CANBT3 = 0x13;

    // disable all the MOBs before enabling controller
    for (uint8_t i = 0; i < 6; ++i)
    {
        SET_CANPAGE(i);     // select MOB
        CANCDMOB = 0;       // disable it
        CANSTMOB = 0;       // clear all status
    }

    // set up MOB1 to receive
    SET_CANPAGE(1);         // select MOB1
    // set up CAN ID and mask. Using 29-bit ID
    uint8_t boardid = get_boardid();
    CANIDT4 = boardid << IDT4;
    CANIDT3 = (uint8_t)(CANID >> 5) + (boardid >> 1);
    CANIDT2 = (uint8_t)(CANID >> 13);
    CANIDT1 = (uint8_t)(CANID >> 21);
    CANIDM4 = (uint8_t)(CANIDMASK << IDT0);
    CANIDM3 = (uint8_t)(CANIDMASK >> 5);
    CANIDM2 = (uint8_t)(CANIDMASK >> 13);
    CANIDM1 = (uint8_t)(CANIDMASK >> 21);

    // enable receive
    CANCDMOB = _BV(CONMOB1) | _BV(IDE) | 8;

    // enable CAN controller
    CANGCON = _BV(ENASTB);
}

/** Send boot loader REPORT message
 *
 * Sends a REPORT message on the CAN bus, using the boot loader defined CAN ID
 * for a REPORT, combined with this board ID.
 *
 * @param len number of bytes in payload
 * @param pmsg point to buffer of payload  bytes
 */
static void send_message(uint8_t len, const uint8_t *pmsg)
{
    // wait for MOB0 to be not busy
    while (CANEN2 & _BV(ENMOB0))
    {}

    // select MOB0
    SAVE_CANPAGE;
    SET_CANPAGE(0);

    // clear any lingering status
    CANSTMOB = 0;

    // set up CAN ID - 29-bit addressing
    uint8_t boardid = get_boardid();
    CANIDT4 = (uint8_t)(boardid << IDT4) + (uint8_t)(CMD_REPORT << IDT0);
    CANIDT3 = (uint8_t)(CANID >> 5) + (boardid >> 1);
    CANIDT2 = (uint8_t)(CANID >> 13);
    CANIDT1 = (uint8_t)(CANID >> 21);

    // set the message payload
    for (uint8_t i = 0; i < len; ++i)
    {
        CANMSG = pmsg[i];
    }

    // enable the MOB for transmission
    CANCDMOB = _BV(CONMOB0) | _BV(IDE) | len;  // IDE=29-bit, DLC

    // wait for transmission complete
    while (!CANSTMOB)
    {}

    // disable the MOB and clear status
    CANCDMOB = 0;
    CANSTMOB = 0;
    RESTORE_CANPAGE;
}

/** Check for new received messages (non-blocking).
 *
 * If return status indicates a message is available, then the recieved message
 * command ID is in the global `cmdid`, the payload length in `msglen`, and the
 * payload bytes in `msgbuf`.
 *
 * @returns status indicating if a message is avaialble
 */
static enum RcvStatus receive_message(void)
{
    enum RcvStatus ret = MSG_NONE;

    // check MOB1
    SAVE_CANPAGE;
    SET_CANPAGE(1);

    // a message has been received
    if (CANSTMOB & _BV(RXOK)) {
        // since we are only matching on messages with this board ID,
        // there is no need to check the message ID, except to extract
        // the 4-bit command field
        cmdid = (CANIDT4 >> IDT0) & 0x0F;

        msglen = CANCDMOB & 0x0f;   // get the DLC

        // extract the payload
        for (uint8_t idx = 0; idx < msglen; ++idx) {
            msgbuf[idx] = CANMSG;
        }

        // clear the status and re-enable the receiver
        CANSTMOB = 0;
        CANCDMOB = _BV(CONMOB1) | _BV(IDE) | 8;     // always use 8 for DLC

        ret = MSG_READY;
    }

    RESTORE_CANPAGE;
    return ret;
}

/** Process any incoming message.
 *
 * This will perform actions based on the incoming command, and then generate
 * a report message in response to the processed command. This function always
 * populates `rptbuf[]` with the appropriate report payload, even if the
 * incoming command message is an error. Therefore, after this function
 * returns, a report is ready to send with `send_message(8, rptbuf)`.
 */
static void process_message(void)
{
    // ongoing load state
    static uint16_t loadaddr = 0;   // byte address of current write
    static uint16_t loadlen = 0;    // load len from START command
    static uint16_t running_crc = 0;

    // a message is available so process according to command ID
    rptbuf[5] = 0;              // clear spare bytes
    rptbuf[6] = 0;              // could save code by not zeroing spares
    rptbuf[7] = ++rxcount;      // receive message counter

    switch (cmdid) {
        case CMD_PING:
            // send a PONG report
            rptbuf[4] = RPT_PONG;
            break;

        case CMD_START:
            running_crc = 0;
            loadaddr = 0;
            loadlen = msgbuf[0] + (msgbuf[1] << 8);
            rptbuf[4] = RPT_READY;
            break;

        case CMD_DATA:
            // make sure we can load another block
            if (loadaddr < loadlen) {
                // write 8 bytes to the page buffer
                for (uint8_t i = 0; i < 8; i += 2) {
                    uint16_t w = msgbuf[i] + (msgbuf[i+1] << 8);
                    boot_page_fill_safe(loadaddr + i, w);
                    // accumulate crc
                    running_crc = _crc16_update(running_crc, msgbuf[i]);
                    running_crc = _crc16_update(running_crc, msgbuf[i+1]);
                }
                loadaddr += 8;  // advance to next 8-byte block

                // if at the end of a page, or end of load, burn the block
                if ((loadaddr >= loadlen)
                || ((loadaddr % SPM_PAGESIZE) == 0)) {
                    uint16_t page = loadaddr - 1;   // previous page
                    boot_page_erase_safe(page);     // erase the page
                    boot_page_write_safe(page);     // write the page
                    boot_spm_busy_wait();           // wait for done
                    boot_rww_enable();              // enable app flash

                    // a flash page has now been programmed
                    // determine response based on end of load vs new page
                    rptbuf[4] = (loadaddr < loadlen) ? RPT_READY : RPT_END;
                }

            } else {
                // load state is not valid so signal an error
                rptbuf[4] = RPT_ERR;
            }
            break;

        case CMD_STOP:
        {
            // extract verification CRC from message
            uint16_t verify_crc = msgbuf[0] + (msgbuf[1] << 8);

            if (verify_crc == running_crc) {
                // crc matches, so save CRC and image length in eeprom
                rptbuf[5] = 1;  // set load status to OK

                // update the image length and CRC in eeprom
                eeprom_update_word(EEP_APP_LEN, loadlen);
                eeprom_update_word(EEP_APP_CRC, running_crc);
                eeprom_busy_wait(); // make sure write done before continue

            } else {
                // crc doesnt match. dont save the crc or image length
                // this will cause app start to fail at boot
                rptbuf[5] = 0;  // load error indication
            }

            rptbuf[4] = RPT_DONE;
            break;
        }

        default:
            // in case of unknown command, send error report
            // with received command id
            rptbuf[4] = RPT_ERR;
            rptbuf[5] = cmdid;
            break;
    }
}

/** Check app integrity and start it
 *
 * Checks the application in flash and if it is okay then it start it.
 * Otherwise it jusr returns which means the app didnt start.
 */
static void  attempt_app_start(void)
{
    static void(*swreset)(void) = 0;

    // compute the CRC over the stored image in flash
    uint16_t len = eeprom_read_word(EEP_APP_LEN);   // length of image
    uint16_t crc = 0;
    for (uint16_t addr = 0; addr < len; ++addr) {
        crc = _crc16_update(crc, pgm_read_byte(addr));
    }
    uint16_t stored_crc = eeprom_read_word(EEP_APP_CRC);

    // if the computed matches the stored, then start the app
    if (crc == stored_crc) {
        // disable WDT
        MCUSR = 0;      // not sure if this is required
        wdt_disable();

        // set all the IO back to the reset state
        DDRB = 0;
        DDRC = 0;
        DDRD = 0;
        PORTB = 0;
        PORTC = 0;
        PORTD = 0;

        // reset the CAN controller (disables it)
        CANGCON = _BV(SWRES);
        // jump to application
        swreset();  // cppcheck-suppress[nullPointer]
    }
}

// make main callable from a unit test
#ifdef UNIT_TEST
#define MAIN app_main
#else
#define MAIN main
#endif
int MAIN(void)
{
    cli();
    device_init();

    // enable the watchdog from this point
    wdt_enable(WDTO_1S);

    // determine reset cause and timeout duration
    uint16_t timeout;
    if (reset_cause & _BV(WDRF)) {
        // if reset was due to WDT, either the app is trying to start the BL,
        // or the boot loader is resetting itself due to error or timeout
        // In this case using the long timeout
        timeout = ACTIVITY_TIMEOUT;
    } else {
        // otherwise we have normal reboot so use short timeout
        timeout = BOOT_TIMEOUT;
    }

    // run forever in this loop until there is a command to reboot or
    // the timeout expires
    uint8_t blinkcount = 50;
    for (;;) {
        wdt_reset();
        // check for available incoming message
        enum RcvStatus status = receive_message();
        if (status == MSG_READY) {
            process_message();
            send_message(8, rptbuf);
            // message was processed, reset timeout
            timeout = ACTIVITY_TIMEOUT;

        } else {
            // no message was processed, update timeout counter
            _delay_ms(1);

            // blink the LED
            if (blinkcount-- == 0) {
                blinkcount = 50;
                LED_TOGGLE();
            }

            // check for timeout
            // if it times out, attempt to run application
            if (timeout-- == 0) {
                LED_ON();   // leave LED on while starting app
                attempt_app_start();
                // if the above returns, it means the app didnt start
                // if main() returns, the AVR runtime library provides
                // a forever loop. This will allow the watchdog to timeout
                // and reset the MCU.
                // In the case of unit test, it allows "app_main()" to be
                // called from the unit test, and it can return to the unit
                // test calling test case
                break;
            }
        }
    }

    return 0;
}
