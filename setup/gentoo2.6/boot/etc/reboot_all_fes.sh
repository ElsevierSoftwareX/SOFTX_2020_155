#!/bin/bash

echo "Rebooting all frontends"
/etc/allrt.sh 'sudo /sbin/init 6'
