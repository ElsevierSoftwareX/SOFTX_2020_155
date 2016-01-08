#!/bin/bash

HOSTNAME=`hostname`
SYSTEMS=`grep -v \^# /etc/rtsystab | grep $HOSTNAME | sed s/$HOSTNAME//`

# remove leading whitespace
SYSTEMS="${SYSTEMS#"${SYSTEMS%%[![:space:]]*}"}"
echo $SYSTEMS
