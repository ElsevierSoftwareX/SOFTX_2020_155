#!/bin/bash
# return 0 exit status if Dolphin IX PCIE card is present
/usr/sbin/lspci -v | grep Dolphin >& /dev/null
