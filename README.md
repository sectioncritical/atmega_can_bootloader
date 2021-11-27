[![](doc/img/license-badge.svg)](https://opensource.org/licenses/MIT)
[![build-bootloader](https://github.com/sectioncritical/atmega_can_bootloader/actions/workflows/build.yml/badge.svg)](https://github.com/sectioncritical/atmega_can_bootloader/actions/workflows/build.yml)


The authoritative location for this project is:
https://github.com/sectioncritical/atmega_can_bootloader

* * * * *

README
======

This project is a CAN boot loader for an ATMega16M1 or similar AVR
microcontrollers.

License
-------

This project uses the [MIT license](https://opensource.org/licenses/MIT).
See the [LICENSE.txt](LICENSE.txt) file for the license terms.

About
-----

This boot loader was originally written for the
[Zeva BMS-24](https://www.zeva.com.au/index.php?product=143). The company,
Zeva, decided to open-source their design files. I wanted to be able to modify
the BMS firmware and make updates in system. The MCU used, the ATMega16M1, has
plenty of memory for a boot loader and some nice features to make it easy to
support a boot loader.

I was not able to find a suitable existing CAN boot loader. There seems to be
or have been a CAN boot loader provided by Atmel at one time. I found some
source code but not from Atmel and it also referred to some CAN libraries that
I could not easily find. I also found some CAN boot loaders that were pretty
bloated with high level CAN protocols. I just wanted something simple, and
small and to the the point.

Even though this is targeting a specific board, I believe it can be modified
fairly easily to work on any other board that uses one of these ATMega16M1
MCUs. There are some variants of this MCU that I beleive would also work.

It can probably also be altered to work with other Atmel AVR-based MCUs that
have a CAN peripheral.

Releases
--------

Released and tested versions can be found in the
[Releases section](https://github.com/sectioncritical/atmega_can_bootloader/releases).

Docs
----

See [doc/README.md](doc/README.md) for some documentation about the CAN bus
protocol, and some implementation details.

See [build/README.md](build/README.md) for information about building the boot
loader and burning it to target flash memory.

See [test/README.md](test/README.md) for information about available
automated tests.

CI/Automation
-------------

Routine build and tests are performed on a push. A release workflow runs when
a tag is pushed that looks like "v1.2.3".

Releases *are not automatic*. The release workflow builds a versioned release
package. But the developer must manually create the release from the tag and
upload the release package.

A custom docker image that has all the necessary tools is used for the CI
operations. It has been pushed to the Github container registry. See
[build/docker/README.md](build/docker/README.md) for more info.
