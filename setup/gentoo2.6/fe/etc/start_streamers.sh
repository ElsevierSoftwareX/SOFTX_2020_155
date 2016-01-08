#!/bin/bash

if [ `id -u` != 0 ]
then
	exec sudo $0
fi

/etc/init.d/mx_stream stop

/etc/init.d/mx_stream start
