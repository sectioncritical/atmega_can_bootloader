Building and Flashing the CAN Boot Loader
=========================================

Tools
-----

Assumes a "standard" build environment with tools like GNU Make, etc.

The "avr-gcc" compiler is used. The Makefile assumes it is installed on your
system and available on the PATH. You can edit the Makefile to specify the
location of your compiler.

If you want to program the boot loader then "avrdude" is used. It's location
is specified in the Makefile, currently part of a platformio installation. You
can edit the Makefile to specify the location of your "avrdude".

If you want to run the code checker, then "cppcheck" must be installed.

Building
--------

`make help` will show the available make targets.

`make` will compile the boot loader and produce a hex file, elf file, and a
map file.

The build products will be in the "obj" directory.

`make VERSION=1.2.3` to specify the version number that will be built into the
boot loader.

`make clean` will clean the build products.

`make check` will run a cppcheck report.

`make package VERSION=1.2.3` will create a .tar.gz file, with version attached
to the name, containing the important build products. This is useful for making
a release package.

Hardware and Programming
------------------------

You need some kind of programming dongle to program the flash of the AVR. For
this project, we are using the SPI/serial programming interface and a "USBtiny"
programmer.

Here is one that does the job:

https://www.adafruit.com/product/46

The Makefile currently assumes you are using a USBtiny programmer, with
avrdude. If you are using something else, the Makefile needs to be modified.

There are several make targets related to working with the target. But the main
one is `make program` which will burn the boot loader hex file into the target
flash memory.

Another important one is `make fuses` which should probably be the first thing
run on a fresh device.

Use `make help` to see the other possibly useful actions.
