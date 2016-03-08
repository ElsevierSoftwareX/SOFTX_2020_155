#!/bin/bash

echo "stop all monits"
/etc/allrt.sh 'sudo /etc/init.d/monit stop'
