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
#include <stdint.h>
#include <stdbool.h>

#include "unity_fixture.h"

#include "avr/io.h"

#include "main.c"


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

TEST_GROUP(process_message);

static uint8_t saved_rxcount;

TEST_SETUP(process_message)
{
    // process_message() does not use any registers
    saved_rxcount = rxcount;    // for verify rx counter is functioning
}

TEST_TEAR_DOWN(process_message)
{
}

TEST(process_message, ping)
{
    cmdid = 0;  // PING command
    process_message();
    // verify contents of report
    TEST_ASSERT_EQUAL_UINT8(99, rptbuf[0]);
    TEST_ASSERT_EQUAL_UINT8(99, rptbuf[1]);
    TEST_ASSERT_EQUAL_UINT8(99, rptbuf[2]);
    TEST_ASSERT_EQUAL_UINT8(1, rptbuf[3]);
    TEST_ASSERT_EQUAL_UINT8(0, rptbuf[4]);
    // verify rx counter was incremented
    TEST_ASSERT_EQUAL_UINT8(saved_rxcount + 1, rptbuf[7]);
}

TEST_GROUP_RUNNER(process_message)
{
    RUN_TEST_CASE(process_message, ping);
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
