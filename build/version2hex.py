#!/usr/bin/env python3

# convert a version string like "v1.2.3" or "1.2.3" to hex formatting
# expected for the boot loader build, like "0x010203"

import sys

verstr = sys.argv[1]
if verstr[0] == 'v':
    verstr = verstr[1:]
versem = verstr.split('.')
verhex = "0x" + "".join([f"{int(s):02X}" for s in versem])
print(verhex, end="")
