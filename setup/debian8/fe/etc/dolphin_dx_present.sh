#!/bin/bash
# return 0 exit status if Dolphin DX PCIE card is present
#  on Debian 8, lspci has moved to /usr/bin
/usr/bin/lspci -v | grep Stargen >& /dev/null
