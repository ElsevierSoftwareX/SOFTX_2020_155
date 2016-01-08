#!/bin/bash
# dxportcfg - based in switch input, determine IP ports

DXCFG=`/etc/dxnodecfg.sh`
PORTCFG=`grep -v \^# /etc/dxporttab | grep \^$DXCFG | sed s/"^$DXCFG "//`
# remove leading whitespace
PORTCFG="${PORTCFG#"${PORTFG%%[![:space:]]*}"}"
echo $PORTCFG
