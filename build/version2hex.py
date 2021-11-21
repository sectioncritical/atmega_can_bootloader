#!/usr/bin/env python3

import sys

verstr = sys.argv[1]
if verstr[0] == 'v':
    verstr = verstr[1:]
versem = verstr.split('.')
verhex = "0x" + "".join([f"{int(s):02X}" for s in versem])
print(verhex, end="")
