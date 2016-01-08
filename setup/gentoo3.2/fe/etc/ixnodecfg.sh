#!/bin/bash
# ixnodecfg- determine Dolphin IX switch config for front-end node

HOSTNAME=`hostname`

IXCFG=`grep -v \^# /etc/ixnodetab | grep $HOSTNAME | sed s/$HOSTNAME//`
IXCFG="${IXCFG#"${IXCFG%%[![:space:]]*}"}"
echo $IXCFG

