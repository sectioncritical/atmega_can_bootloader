# SPDX-License-Identifier: MIT
#
# Copyright 2021 Joseph Kroesche
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

EXE=bootloader_test

SRCS=src/test_main.c
#SRCS+=src/sample_test.c
SRCS+=unity/src/unity.c
SRCS+=unity/extras/fixture/src/unity_fixture.c
SRCS+=src/avr/wdt.c
SRCS+=src/avr/io.c
SRCS+=src/avr/boot.c
SRCS+=src/avr/eeprom.c
SRCS+=src/util/delay.c
SRCS+=src/util/crc16avr.c
SRCS+=src/libcrc/crc16.c

INCS=-Iunity/src -Iunity/extras/fixture/src -Isrc -I../src

CC=gcc

CFLAGS=-std=c99
CFLAGS+=-pedantic-errors -Wall -Werror
CFLAGS+=--coverage
CFLAGS+=-DUNIT_TEST
CFLAGS+=-DUNITY_EXCLUDE_FLOAT
CFLAGS+=-DUNITY_FIXTURE_NO_EXTRAS

#CFLAGS+=-E

ifeq ($(DEBUG), 1)
	CFLAGS+=-g -Og
endif

all: $(EXE)

$(EXE): $(SRCS)
	$(CC) $(CFLAGS) $(INCS) $(SRCS) -o $@
#	$(CC) $(CFLAGS) $(INCS) $(SRCS)

.PHONY: tidy
tidy:
	rm -f *.gcda *.gcno

.PHONY: clean
clean: tidy
	rm -f $(EXE)

.PHONY: run
run: $(EXE)
	./$(EXE) -v
