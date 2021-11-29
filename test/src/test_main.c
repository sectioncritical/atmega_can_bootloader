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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "unity_fixture.h"
#include "libcrc/checksum.h"

#include "avr/io.h"
#include "avr/pgmspace.h"

#include "main.c"

#define FLASH_SIZE (FLASHEND + 1)

/*****************************************************************************/

TEST_GROUP(send_message);

TEST_SETUP(send_message)
{
    reset_all();
    // send_message() polls on CANSTMOB zero. CANSTMOB is accessed
    // once before the poll so need to use index 1 for the non-zero
    // to keep send_message() from hanging in a poll loop
    CANSTMOB_reg8.data[1] = 1;
}

TEST_TEAR_DOWN(send_message)
{
}

TEST(send_message, nominal_send)
{
    uint8_t msg[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    send_message(8, msg);
    TEST_ASSERT(CANMSG_reg8.idx == 8);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(msg, CANMSG_reg8.data, 8);
}

TEST_GROUP_RUNNER(send_message)
{
    RUN_TEST_CASE(send_message, nominal_send);
}

/*****************************************************************************/

TEST_GROUP(process_message);

static uint8_t saved_rxcount;
static uint16_t test_crc;
static uint16_t membufidx;
static uint8_t test_image[FLASH_SIZE];

uint8_t pgm_read_byte(uint16_t addr)
{
    return test_image[addr];
}

TEST_SETUP(process_message)
{
    // process_message() does not use any registers
    saved_rxcount = rxcount;    // for verify rx counter is functioning
    // reset rx message variables state
    msglen = 0;
    cmdid = 0;
    for (unsigned int i = 0; i < 8; ++i) {
        msgbuf[i] = 0;
    }
}

TEST_TEAR_DOWN(process_message)
{
}

static void verify_report_header(uint8_t rpt_type)
{
    // verify common fields of report
    TEST_ASSERT_EQUAL_UINT8(99, rptbuf[0]);     // version
    TEST_ASSERT_EQUAL_UINT8(99, rptbuf[1]);
    TEST_ASSERT_EQUAL_UINT8(99, rptbuf[2]);
    TEST_ASSERT_EQUAL_UINT8(1, rptbuf[3]);      // status
    TEST_ASSERT_EQUAL_UINT8(rpt_type, rptbuf[4]);
    // verify rx counter was incremented
    TEST_ASSERT_EQUAL_UINT8(saved_rxcount + 1, rptbuf[7]);
}

TEST(process_message, ping)
{
    cmdid = 0;  // PING command
    msglen = 0;
    process_message();
    // verify contents of report
    verify_report_header(0);    // type PONG
}

// create a fake image based on random seed
static uint8_t *create_image(unsigned seed, unsigned len)
{
    srand(seed);
    for (unsigned i = 0; i < len; ++i) {
        test_image[i] = (uint8_t)rand();
    }
    return test_image;
}

// send a START message and verify response
static void test_message_start(uint16_t len)
{
    cmdid = 2;  // start command
    uint16_t testlen = len;
    msgbuf[0] = (uint8_t)testlen;
    msgbuf[1] = (uint8_t)(testlen >> 8);
    msglen = 2;
    process_message();

    // verify contents of report
    verify_report_header(1);    // type READY

    ++saved_rxcount;

    // internal load state variables are function local and are not
    // visible for testing access. they are verified to be correct through
    // other test side effects
}

// send a DATA message and verify response
// this is a data message that is not expected to complete the load
// keep track of crc
// message payload is always 8 bytes
static void test_message_data_ongoing(const uint8_t *payload)
{
    cmdid = 3;      // DATA command
    msglen = 8;
    for (unsigned int i = 0; i < 8; ++i) {
        msgbuf[i] = payload[i];
        test_crc = update_crc_16(test_crc, payload[i]);
    }
    process_message();

    // verify contents of report
    verify_report_header(1);    // type READY

    ++saved_rxcount;
}

// send a DATA message and verify response
// this is the expected last segment
// it should receive and end report
static void test_message_data_end(const uint8_t *payload, uint16_t final_len)
{
    cmdid = 3;      // DATA command
    msglen = 8;     // message always has 8 bytes even if there are fewer
    memset(msgbuf, 0, 8);    // init buffer value since less than 8 loaded
    for (unsigned int i = 0; i < final_len; ++i) {
        msgbuf[i] = payload[i];
        test_crc = update_crc_16(test_crc, payload[i]);
    }
    process_message();

    // verify contents of report
    verify_report_header(2);    // type END

    ++saved_rxcount;
}

// send a STOP message and verify the response
// CRC should be verified
static void test_message_stop(void)
{
    cmdid = 4;
    msglen = 2;
    // set the crc in the payload
    msgbuf[0] = test_crc;
    msgbuf[1] = test_crc >> 8;

    process_message();

    // verify report
    verify_report_header(3);
    TEST_ASSERT_EQUAL_UINT8(1, rptbuf[5]);  // verify status is ok

    ++saved_rxcount;
}

// send a STOP message with bad crc
// CRC should cause error
static void test_message_stop_bad(void)
{
    cmdid = 4;
    msglen = 2;
    // set the crc in the payload
    msgbuf[0] = test_crc + 1;
    msgbuf[1] = test_crc >> 8;

    process_message();

    // verify report
    verify_report_header(3);
    TEST_ASSERT_EQUAL_UINT8(0, rptbuf[5]);  // verify status is error

    ++saved_rxcount;
}

TEST(process_message, start)
{
    // test the start message for several different image sizes
    test_message_start(1);
    test_message_start(8);
    test_message_start(9);
    test_message_start(15);
    test_message_start(255);
    test_message_start(256);
    test_message_start(257);
    test_message_start(0);
}

TEST(process_message, data_basic)
{
    // reset the test CRC and the expected address in mem buffer
    test_crc = 0;
    membufidx = 0;

    test_message_start(257);    // initiate a load
    uint8_t testbuf[8] = { 5, 6, 7, 8, 1, 2, 3, 4 };
    test_message_data_ongoing(testbuf);

    // this should have loaded 8 bytes into the flash buffer
    // since there was only one load, it is the start of flash buf
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testbuf, flashbuf, 8);
}

TEST(process_message, data_end)
{
    // reset the test CRC and the expected address in mem buffer
    test_crc = 0;
    membufidx = 0;
    flash_reset();  // reset the simulated flash memory

    // create a test image
    uint8_t *testimg = create_image(1, 15);

    // 15 byte load only requires 2 DATA messages
    test_message_start(15);    // initiate a load
    test_message_data_ongoing(testimg);
    // verify first 8 bytes into flash buffer
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testimg, flashbuf, 8);

    // load next 7 bytes
    test_message_data_end(&testimg[8], 7);
    // this should cause the flash mem to get loaded with the image,
    // and the flash buf to get reset
    TEST_ASSERT_EQUAL_UINT8_ARRAY(testimg, flashmem, 15);
    uint8_t *flashmem8 = (uint8_t *)flashmem;
    TEST_ASSERT_EQUAL_UINT8(0, flashmem8[15]); // last byte
    TEST_ASSERT_EACH_EQUAL_UINT8(0xff, flashbuf, SPM_PAGESIZE);
    TEST_ASSERT_EACH_EQUAL_UINT8(0xff, &flashmem8[16], FLASH_SIZE - 16);

    // rww should be re-enabled
    TEST_ASSERT_TRUE(flash_rww_enabled);
}

TEST(process_message, stop)
{
    // to keep boot loader simple, it require image load size to always be
    // multiple of 8 bytes

    // reset the test CRC and the expected address in mem buffer
    test_crc = 0;
    membufidx = 0;
    flash_reset();  // reset the simulated flash memory
    eep_reset();    // reset the simulated eeprom

    // create a test image
    uint8_t *testimg = create_image(42, 24); // seed=42
    test_message_start(24);

    // first 8 byte load
    test_message_data_ongoing(&testimg[0]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&testimg[0], flashbuf, 8);

    // second 8 byte load
    test_message_data_ongoing(&testimg[8]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&testimg[0], flashbuf, 16);

    // final 8 byte load
    uint8_t *flashmem8 = (uint8_t *)flashmem;
    test_message_data_end(&testimg[16], 8);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&testimg[0], flashmem8, 24); // flash
    TEST_ASSERT_EACH_EQUAL_UINT8(0xff, &flashmem8[24], FLASH_SIZE - 24);

    // rww should be re-enabled
    TEST_ASSERT_TRUE(flash_rww_enabled);

    // send stop message and verify ok
    test_message_stop();    // sends crc and verifies

    // verify eeprom length and crc was updated
    uint16_t eep_len = eepmem[E2END-3] + (eepmem[E2END-2] << 8);
    uint16_t eep_crc = eepmem[E2END-1] + (eepmem[E2END] << 8);
    TEST_ASSERT_EQUAL_UINT16(24, eep_len);
    TEST_ASSERT_EQUAL_UINT16(test_crc, eep_crc);
}

TEST(process_message, stop_bad)
{
    // reset the test CRC and the expected address in mem buffer
    test_crc = 0;
    membufidx = 0;
    flash_reset();  // reset the simulated flash memory
    eep_reset();    // reset the simulated eeprom

    // create a test image
    uint8_t *testimg = create_image(19, 20); // seed=19
    test_message_start(20);

    // first 8 byte load
    test_message_data_ongoing(&testimg[0]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&testimg[0], flashbuf, 8);

    // second 8 byte load
    test_message_data_ongoing(&testimg[8]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&testimg[0], flashbuf, 16);

    // final 4 byte load
    uint8_t *flashmem8 = (uint8_t *)flashmem;
    test_message_data_end(&testimg[16], 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&testimg[0], flashmem8, 20); // flash
    TEST_ASSERT_EACH_EQUAL_UINT8(0, &flashmem8[20], 4);
    TEST_ASSERT_EACH_EQUAL_UINT8(0xff, &flashmem8[24], FLASH_SIZE - 24);

    // rww should be re-enabled
    TEST_ASSERT_TRUE(flash_rww_enabled);

    // send stop message with bad crc and verify error return
    test_message_stop_bad();    // sends crc and verifies

    // verify eeprom was not modified
    TEST_ASSERT_EACH_EQUAL_UINT8(0xFF, &eepmem[E2END-3], 4);
}

TEST_GROUP_RUNNER(process_message)
{
    RUN_TEST_CASE(process_message, ping);
    RUN_TEST_CASE(process_message, start);
    RUN_TEST_CASE(process_message, data_basic);
    RUN_TEST_CASE(process_message, data_end);
    RUN_TEST_CASE(process_message, stop);
    RUN_TEST_CASE(process_message, stop_bad);
}

static void runner(void)
{
    //RUN_TEST_GROUP(sample);
    RUN_TEST_GROUP(send_message);
    RUN_TEST_GROUP(process_message);
}

int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, runner);
}
