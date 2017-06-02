#!/bin/bash

echo "Powering off all frontends"
/etc/allrt.sh 'sudo /sbin/init 0'
