#!/bin/bash
# ixportcfg - based in Dolphin IX switch input, determine IP ports

IXCFG=`/etc/ixnodecfg.sh`
PORTCFG=`grep -v \^# /etc/ixporttab | grep \^$IXCFG | sed s/"^$IXCFG "//`
# remove leading whitespace
PORTCFG="${PORTCFG#"${PORTFG%%[![:space:]]*}"}"
echo $PORTCFG
