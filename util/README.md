CAN Loader Python Utility
=========================

This is a simple python utility for using the CAN boot loader via CAN bus
from a development host. In this case, the development host is a Raspberry Pi
with an MCP-2515 based CAN board attached. More hardware description below:

It implements the CAN boot loader protocol that is
[described here](../doc/protocol.md).

Usage
-----

The required dependencies are in the `requirements.txt` file. The expected way
to use this is with a python virtual environment:

    python3 -m venv venv
    . venv/bin/activate
    pip install -U pip wheel setuptools
    pip install -r requirements.txt

Once that is done you can run it like this:

    python canloader.py --help

The built-in help shows the command and options, but it has 3 basic features:

* scan - scan all possible addresses (0-15) to find units on the bus that are
  running the CAN boot loader
* ping - send a query to specific address and return some information
* load - load a hex file into target flash

Hardware
--------

This python utility expects to be able to use bus type "socketcan" and CAN
interface named `can0`. It may be possible to use this on a PC with an
attached USB-CAN interface. However, it was only tested using a Raspberry Pi
with an MCP2515-based CAN board.

### RPi/CAN Board

The Raspberry Pi is used to communicate with the CAN boot loader via CAN bus.
Currently it is a RPi 3 B+ but it probably doesnt matter. The RPi is set up
with Raspbian lite (headless) and the normal suite of development packages.
In addition it has:

* can-utils

The CAN board is based on MCP2515 CAN controller. There are many boards
available that use this device. Currently we are using this:

https://www.mikroe.com/can-spi-33v-click

This requires using jumper wires to connect the CAN board to the RPi. In the
future it would be better to get a Pi Hat with CAN, or design a custom test
fixture Hat that includes the CAN controller.

Jumper wire setup:

| Pi | Board       |
|----|-------------|
| 6  | GND         |
| 1  | 33V         |
| 19 | SDI (MOSI)  |
| 21 | SDO (MISO)  |
| 23 | SCK         |
| 24 | CS          |
| 22 | INT (GPIO25)|

There are multiple options for GND and 3.3V. Also multiple choices of GPIO can
be used for INT. It is also possible to use SPI1 but I didn't try that.

#### RPi Config for CAN

The RPi requires several one-time configuration steps.

##### /boot/config.txt

This file needs to be edited using `sudo` or as root. Add the following lines
as needed:

```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=10000000,interrupt=25
```

Make sure that `oscillator` value matches whatever is on your CAN board. Also,
The `interrupt` parameter is the RPi GPIO number where you connected the CAN
board INT pin.

This will not take effect until you reboot the RPi.

##### /etc/modprobe.d/raspi-blacklist.conf

I am not sure if this is really needed. I saw one site mention this was
required, but didnt see it mentioned elsewhere. Add the following to this file:

```
spi-bcm2708
```

##### Set up CAN socket

    sudo ip link set can0 up type can bitrate 250000

This can be automated to a script, or the RPi can be setup to do this at boot
time.

##### /etc/network/interfaces

**I haven't tested this.**

The following is used to set up the CAN socket for automatic start.

```
auto can0
iface can0 inet manual
    pre-up /sbin/ip link set can0 type can bitrate 250000 triple-sampling on restart-ms 100
    up /sbin/ifconfig can0 up
    down /sbin/ifconfig can0 dow
```

I have seen examples of this that did not have "triple-sampling".
