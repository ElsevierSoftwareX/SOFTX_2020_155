#!/bin/bash
# return 0 exit status if Dolphin DX PCIE card is present
/usr/sbin/lspci -v | grep Stargen >& /dev/null
