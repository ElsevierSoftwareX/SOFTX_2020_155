#!/bin/bash

HOSTNAME=`hostname`
SYSTEMS=`grep -v \^# /diskless/root/etc/rtsystab | awk '{print $1}' `

# remove leading whitespace
SYSTEMS="${SYSTEMS#"${SYSTEMS%%[![:space:]]*}"}"

if [ $# -eq 0 ]
then
	echo $SYSTEMS
else
	for i in $SYSTEMS
	do
		echo Doing $i
		ssh $i $*
	done
fi

