#!/bin/bash
# return 0 exit status if Dolphin DX PCIE card is present
/usr/sbin/lspci | grep Stargen | grep 0101 >& /dev/null
