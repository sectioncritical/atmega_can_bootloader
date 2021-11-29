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

# TODO: future code size profiling, look at puncover or bloaty

# using bashisms
SHELL=/bin/bash

PROGNAME=canboot

TARGET_MCU=atmega16m1

# development version, can be overridden for production release
VERSION?=99.99.99

# AVRDUDE ports
# you will need to override these for your system. The easiest thing to do
# is to set an environment variable and then you wont need to type on command
# line each time you run this Makefile.
#
# AVRDUDE_PORT is the device port used for programmer like ArduinoISP
# it is used for any Makefile targets that "program" something. If you are
# not using the programmer, then this does not matter.
#
## Not needed for USBTiny programmer
##AVRDUDE_PORT?=/dev/cu.usbmodem14101

# settings for avrdude binary. You can override these to match your system.
# avrdude should be installed somewhere on your system. The default setting
# below assumes it is on your path. Standard avrdude does not include
# support for atmega61m1 so the m16m1.conf file is specified as a supplement.
# You can override the location of avrdude executable and conf file if you
# need to. (the + means add to system configuration)
# TODO: come up with better way to find existing avrdude
#AVRDUDE?=$(shell command -v avrdude)
#AVRDUDE_CONF?= +avrdude/m16m1.conf
AVRDUDE?=~/.platformio/packages/tool-avrdude/bin/avrdude
AVRDUDE_CONF?=~/.platformio/packages/tool-avrdude/avrdude.conf
AVRDUDE_DEVICE?=atmega16m1
AVRDUDE_PROGRAMMER=usbtiny
AVRDUDE_BAUD=19200

# not needed for usbtiny programmer # -P$(AVRDUDE_PORT) -b(AVRDUDE_BAUD)
AVRDUDE_CMD=$(AVRDUDE) -v -p $(AVRDUDE_DEVICE) -C $(AVRDUDE_CONF) -c $(AVRDUDE_PROGRAMMER)

# MEMORY LAYOUT (Assuming ATMega16M1)
#
# Allocated 2K for the boot loader. This leaves 14K for application.
# (all addresses are byte addresses - note datasheet uses word addresses a lot)
#
# All flash:   0x0000 - 0x3FFF (0x4000/16384)
# App memory:  0x0000 - 0x37FF (0x3800/14336)
# Boot memory: 0x3800 - 0x3FFF (0x0800/2048)
#
# Note that the NRWW section starts at 0x3000
#
# boot loader start address
# this relies on correct fuse setting
#
START_ADDRESS?=0x3800

OUT=obj
SRC=../src

OBJS=$(OUT)/main.o

# device remains unlocked
LOCKFUSE=0xff

# leave reset enabled   : 1xxx xxxx
# debugwire disabled    : x1xx xxxx
# SPI prog enabled      : xx0x xxxx
# WDT not enabled       : xxx1 xxxx
# dont erase eeprom     : xxxx 0xxx
# boot size 2048        : xxxx x01x
# bootloader reset      : xxxx xxx0
# result                : 1101 0010 = 0xD2
HFUSE=0xd2

# disable /8, external osc, longer startup time
LFUSE=0xdf

# BOD level 4.2
EFUSE=0xfa

HEXFILE:=$(OUT)/$(PROGNAME).hex
ELFFILE:=$(OUT)/$(PROGNAME).elf

all: $(HEXFILE)

.PHONY: help
help:
	@echo ""
	@echo "ATMega16M1 CAN Bootloader Makefile Help"
	@echo "======================================="
	@echo ""
	@echo "all/(default)    - build the boot loader hex file (BAUD)"
	@echo "clean            - delete all build products"
	@echo ""
	@echo "program          - program boot loader to target using programmer"
	@echo "fuses            - program the target fuses (new device)"
	@echo "dumpflash        - save flash contents to binary file"
	@echo "avrping          - programmer probe the target"
	@echo "erase            - erase the device using programmer (not usual)"
	@echo ""
	@echo "check            - run code checker"
	@echo "test             - run unit tests"
	@echo "cleantest        - clean up test residue"
	@echo ""
	@echo "package          - create versioned package archive"
	@echo "program-pkg      - program from a release package (VERSION=1.2.3)"
	@echo ""

CC=avr-gcc
OBJCOPY=avr-objcopy
SIZE=avr-size

CFLAGS=-std=c99 -Os -Werror -Wall -ffunction-sections -fdata-sections -fshort-enums -flto -mmcu=$(TARGET_MCU)
LDFLAGS=-Wl,-Map,$(OUT)/$(PROGNAME).map -Wl,--gc-sections -Wl,--section-start=.text=$(START_ADDRESS) -fuse-linker-plugin

$(OUT):
	mkdir -p $(OUT)

$(OUT)/%.o: $(SRC)/%.c | $(OUT)
	VERHEX=$$(python3 version2hex.py $(VERSION)); \
	$(CC) $(CFLAGS) -DBOOTVER=$$VERHEX -o $@  -c $<

$(ELFFILE): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBFLAGS)

$(HEXFILE): $(ELFFILE)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) $< 2>&1 | tee -a $(BUILDLOG)

.PHONY: clean
clean:
	rm -rf $(OUT)

# flash the firmware onto the target
.PHONY: program
program: $(HEXFILE)
	$(AVRDUDE_CMD) -U flash:w:$<:i

# dump the flash contents into a file
.PHONY: dumpflash
dumpflash:
	$(AVRDUDE_CMD) -U flash:r:flash_dump.bin:r

# dump the eeprom contents into a file
.PHONY: dumpeep
dumpeep:
	$(AVRDUDE_CMD) -U eeprom:r:eeprom_dump.bin:r

.PHONY: fuses
fuses:
	$(AVRDUDE_CMD) -Ulock:w:$(LOCKFUSE):m -Uhfuse:w:$(HFUSE):m -Ulfuse:w:$(LFUSE):m -Uefuse:w:$(EFUSE):m

.PHONY: erase
erase:
	$(AVRDUDE_CMD) -e

.PHONY: avrping
avrping:
	$(AVRDUDE_CMD)

# run cppcheck with plain output - useful for local running, and no reports
.PHONY: check
check:
	cppcheck --std=c99 --platform=avr8 --enable=all -UUNIT_TEST --suppress=missingIncludeSystem --inline-suppr -v ../src/main.c

# clean test build products
.PHONY: cleantest
cleantest:
	make -C ../test clean

# run the unit tests
.PHONY: test
test: cleantest
	make -C ../test
	make -C ../test run

.PHONY: package
package: $(HEXFILE)
	rm -rf package
	mkdir -p package
	cp $(HEXFILE) package/.
	cp $(ELFFILE) package/.
	cp $(OUT)/$(PROGNAME).map package/.
	cp ../CHANGELOG.md package/.
	PKGNAME=$(PROGNAME)-$(VERSION); rm -rf $$PKGNAME; \
	mv package $$PKGNAME; \
	tar -zcvf $$PKGNAME.tar.gz $$PKGNAME/*

# extract hex from release package and program it
.PHONY: program-pkg
program-pkg:
	PKGNAME=$(PROGNAME)-$(VERSION); \
	tar -zxvf $$PKGNAME.tar.gz; \
	$(AVRDUDE_CMD) -U flash:w:$$PKGNAME/$(PROGNAME).hex:i; \
	rm -rf $$PKGNAME
