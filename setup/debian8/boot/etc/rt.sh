#!/bin/bash

#
# example removing some models from list
#SYSTEMS=`grep -v \^# /diskless/root/etc/rtsystab | sed 's/^\(\S*\)\w*//' | egrep
# -v psl\|lsc\|hpi\|mc3\|susaux`

# all models
SYSTEMS=`grep -v \^# /diskless/root/etc/rtsystab | sed 's/^\(\S*\)\w*//'`

# remove leading whitespace
#SYSTEMS="${SYSTEMS#"${SYSTEMS%%[![:space:]]*}"}"
echo $SYSTEMS

