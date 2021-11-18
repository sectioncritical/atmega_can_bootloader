CAN Boot Loader Implementation
==============================

This CAN boot loader was originally developed for the Zeva 24 cell BMS module.

https://www.zeva.com.au/index.php?product=143

This board uses an ATMega16M1 automotive microcontroller with CAN bus
peripheral. It should be possible to adapt it to other hardware that uses a
similar microcontroller.

For the boot loader CAN message protocol [see here](protocol.md).

Hardware Environment
--------------------

The microcontroller used is ATMega16M1. It should also work with the other
ATMegaXXM1 variants in this MCU family.

The board uses an external 8 MHz crystal.

There is an active high red LED attached to PD3.

There is 16 position rotary switch for the board ID (0-15). It is connected to
the following GPIO pins in descending significance order: PD5, PD7, PB2, PD6.
These signals are active low and therefore require the pullups to be enabled
for these GPIOs.

Implementation Details
----------------------

The boot loader takes advantage of the MCU boot loader support. This feature
reserves memory for the boot loader and also redirects the boot vector so that
the boot loader will always start at reset. The application then remains
located at starting address 0 and retains the normal boot and interrupt vector
locations. This means that no special steps need to be taken for application
development to make it compatible with the boot loader, such as being located
at a special location other than zero, or redirecting interrupt vectors. The
application can be unaware of the boot loader, and will also run on a device
that does not have a boot loader installed.

The boot loader does not use any interrupts so it does not relocate the
interrupt vector table.

The boot loader uses the reset cause status bits to determine if a watchdog
reset occurred. The boot loader interprets this to mean that the boot loader
was deliberately started by the application. Therefore, the correct way for the
application to start the boot loader is to allow a watchdog reset.

### Memory Usage

The boot loader is about 1500 bytes. So the 2K boot loader size option is used,
leaving 14K for the application (on a 16K device).

It may be possible to optimize the source code to get the size down to 1024
bytes in which case the 1K boot loader size could be used, providing 1K more
flash space for the programm. No attempt has been made to make the boot loader
any smaller.

| Address   | Usage               |
|-----------|---------------------|
| 0000:3FFF | All flash           |
| 0000:37FF | Application section |
| 3800:3FFF | Boot loader section |

The boot loader start address is 0x3800. These settings are controlled by
fuses (see below).

#### EEPROM Usage

The boot loader uses the last 4 bytes of EEPROM to store the application length
and CRC. The application must not overwrite these locations or else the boot
loader will not be able to start the application at the next reset.

### Fuses

This section shows how the fuses are set for an ATMega16M1 to work with the
CAN boot loader. Not all fuse bits are required to be set exactly this way.
This is the tested configuration.

#### HFUSE

| Bit       | Setting                   |
|-----------|---------------------------|
|`1xxx xxxx`| leave reset enabled       |
|`x1xx xxxx`| debugwire disabled        |
|`xx0x xxxx`| SPI programming enabled   |
|`xxx1 xxxx`| WDT not enabled           |
|`xxxx 0xxx`| dont erase EEPROM         |
|`xxxx x01x`| boot size 2048            |
|`xxxx xxx0`| reset to boot loader      |
| *Result*  | *Final Value*             |
|`1101 0010`| 0xD2                      |

#### LFUSE

| Bit       | Setting                           |
|-----------|-----------------------------------|
|`1xxx xxxx`| disable /8 clock divider          |
|`x1xx xxxx`| clock pin output disabled         |
|`xx01 xxxx`| startup select: fast rising power |
|`xxxx 1111`| 8 MHz xtal, BOD enabled           |
| *Result*  | *Final Value*                     |
|`1101 1111`| 0xDF                              |

#### EFUSE

| Bit       | Setting                   |
|-----------|---------------------------|
|`1111 1xxx`| PSC default (not used)    |
|`xxxx x010`| BOD level 4.2V            |
| *Result*  | *Final Value*             |
|`1111 1010`| 0xFA                      |
