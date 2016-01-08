#!/bin/bash
# dxnodecfg- determine switch config for front-end node

HOSTNAME=`hostname`

DXCFG=`grep -v \^# /etc/dxnodetab | grep $HOSTNAME | sed s/$HOSTNAME//`
DXCFG="${DXCFG#"${DXCFG%%[![:space:]]*}"}"
echo $DXCFG

