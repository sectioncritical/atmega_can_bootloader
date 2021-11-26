#!/usr/bin/env python
#!/usr/bin/env python
#
# MIT License
#
# Copyright (c) 2021 Joseph Kroesche
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#

import argparse
import can
from intelhex import IntelHex

_can_rate = 250000

# CRC16 implementation that matches the C version in the boot loader
def crc16_update(crc, val):
    crc ^= val
    for i in range(8):
        if (crc & 1):
            crc = (crc >> 1) ^ 0xA001
        else:
            crc = (crc >> 1)
    return crc & 0xFFFF;

def build_arbid(boardid, cmdid):
    return 0x1b007100 + (boardid << 4) + cmdid

# scan for any board running the CAN boot loader
def scan():
    arbid = build_arbid(0, 0)  # initial arb id, cmd=0/PING, boardid=0
    bus = can.interface.Bus(bustype="socketcan", channel="can0", bitrate=_can_rate)
    msg = can.Message(arbitration_id=arbid, is_extended_id=True, data=[])

    print("Scanning for CAN boot loaders")

    for boardid in range(16):
        print(f"{boardid:02d} ... ", end="")
        msg.arbitration_id = build_arbid(boardid=boardid, cmdid=0)
        # send the ping message to the address
        bus.send(msg)

        # check for a reply
        rxmsg = bus.recv(timeout=0.1)
        if rxmsg:
            expid = arbid + (boardid << 4) + 5  # expected REPORT arbid
            # check for proper PONG response
            if ((rxmsg.arbitration_id == expid)
                and (rxmsg.dlc == 8)
                and (rxmsg.data[4] == 0)):
                    print("OK")
            else:
                print("err")
                print(rxmsg)

        # no response
        else:
            print()

# send a PING to a specified boardid
# pretty print the reply information such as boot laoder version
def ping(boardid):
    arbid = build_arbid(boardid=boardid, cmdid=0)  # PING
    bus = can.interface.Bus(bustype="socketcan", channel="can0", bitrate=_can_rate)
    msg = can.Message(arbitration_id=arbid, is_extended_id=True, data=[])
    bus.send(msg)

    rxmsg = bus.recv(timeout=0.1)
    if rxmsg:
        rxcmd = rxmsg.arbitration_id & 0x0F
        if rxcmd != 5:
            print(f"unexpected message was not a REPORT ({rxcmd})")
            return

        payload = rxmsg.data
        verstr = f"{payload[0]}.{payload[1]}.{payload[2]}"
        statstr = "OK" if (payload[3] == 1) else "Err"
        print(f"Board ID: {boardid}")
        print(f"Version:  {verstr}")
        print(f"Status:   {statstr}")
        print(f"payload 5/6: {payload[5]:02X} {payload[6]:02X}")

    else:
        print("No reply")

# get CAN message and verify it is a REPORT
# if so, return the message payload
# else return None
def get_report(canbus):
    msg = canbus.recv(timeout=0.1)
    if msg:
        rxcmd = msg.arbitration_id & 0x0F
        if rxcmd == 5:
            return msg.data

    return None

# upload the hex file filename, to the specified boardid
# using the CAN protocol
def load(boardid, filename):
    # load the hex file
    ih = IntelHex(filename)

    # make sure is just one segment and it starts at 0
    segs = ih.segments()
    if len(segs) != 1:
        print("ERR: more than one segment in hex file")
        return
    seg = segs[0]
    imgaddr = seg[0]
    imglen = seg[1]
    if imgaddr != 0:
        print("ERR: image segment does not start at address 0")
        return

    print(f"original image length: {imglen}")

    # pad out to multiple of 8 bytes length
    padlen = 8 - (imglen % 8)
    print(f"padlen: {padlen}")
    if padlen != 0:
        for idx in range(imglen, imglen+padlen):
            ih[idx] = 0
    imglen = len(ih)    # new image length
    print(f"new image len: {imglen}")

    bus = can.interface.Bus(bustype="socketcan", channel="can0", birate=_can_rate)

    # send start command
    arbid = build_arbid(boardid=boardid, cmdid=2)
    msg = can.Message(arbitration_id=arbid, is_extended_id=True,
                      data=[imglen & 0xFF, (imglen >> 8) & 0xFF])
    bus.send(msg)
    # verify READY report
    rpt = get_report(bus)
    if rpt is None or rpt[4] != 1:
        print("ERR: did not recieve READY after START")
        print("report:", rpt)
        return

    # iterate over image in 8 byte chunks
    loadcrc = 0
    for idx in range(0, imglen, 8):
        print(f"{idx:04X}: ")
        # create a DATA message
        arbid = build_arbid(boardid=boardid, cmdid=3)
        payload = ih.tobinarray(start=idx, size=8)
        for val in payload:
            loadcrc = crc16_update(loadcrc, val)
        msg = can.Message(arbitration_id=arbid, is_extended_id=True,
                          data=payload)
        bus.send(msg)
        rpt = get_report(bus)
        rptype = 2 if (imglen - idx) == 8 else 1
        if rpt is None or rpt[4] != rptype:
            print("ERR: did not recieve READY after DATA")
            print("report:", rpt)
            return

    # send STOP command
    arbid = build_arbid(boardid=boardid, cmdid=4)
    msg = can.Message(arbitration_id=arbid, is_extended_id=True,
            data=[loadcrc & 0xFF, (loadcrc >> 8) & 0xFF])
    bus.send(msg)
    rpt = get_report(bus)
    if rpt is None or rpt[4] != 3:
        print("ERR: did not recieve DONE after STOP")
        print("report:", rpt)
        return

    if rpt[5] != 1:
        print("ERR: target indicates load error")
        return

    print("Load complete with success indication from target")
    print(f"len={imglen:04X} crc={loadcrc:04X}")

# command line interface
def cli():
    global _can_rate

    parser = argparse.ArgumentParser(description="CAN Firmware Loader")
    parser.add_argument('-v', "--verbose", action="store_true",
                        help="turn on some debug output")
    parser.add_argument('-r', "--rate", type=int, default=_can_rate,
                        help=f"CAN data rate ({_can_rate})")
    parser.add_argument('-f', "--file", help="file to upload")
    parser.add_argument('-b', "--board", type=int, help="board ID of target")
    parser.add_argument("command", help="loader command (ping, scan, load)")

    args = parser.parse_args()

    if args.rate:
        _can_rate = args.rate

    if args.command == "scan":
        scan()

    elif args.command == "ping":
        if args.board is None:
            print("ping must specify --board")
        else:
            ping(args.board)

    elif args.command == "load":
        if args.board is None:
            print("load must specify --board")
        elif args.file is None:
            print("load must specify --file")
        else:
            load(args.board, args.file)

    else:
        print("unknown command")

if __name__ == "__main__":
    cli()
