CHANGELOG
=========

CAN boot loader for ATMega16M1 or similar AVR microcontrollers.

Visit the repository for documentation.

**Repository:** https://github.com/sectioncritical/atmega_can_bootloader

[Keep a Changelog](https://keepachangelog.com/en/1.0.0/) /
[Semantic Versioning](https://semver.org/spec/v2.0.0.html)

## [1.0.0] - 2021-11-28

Initial release of the ATMega CAN Bootloader

### Features

- resides in ATMega boot loader memory, does not interfere with app at 0
- compiled size approx 1.5k
- simple CAN protocol for loading flash
- up to 16 devices on a bus
- boot timeout before starting app
- starts app automatically if no boot commands via CAN
- verifies app integrity with 16-bit CRC
- can be entered from app by triggering a watchdog timeout

* * * * *

[1.0.0]: https://github.com/sectioncritical/atmega_can_bootloader/releases/tag/v1.0.0
