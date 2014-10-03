#!/bin/bash

echo Killing all front-ends
/etc/allrt.sh 'sudo /etc/kill_fes.sh; sudo /etc/kill_epics.sh'
echo Restart all front-ends
/etc/allrt.sh 'sudo /etc/start_epics.sh; sudo /etc/start_fes.sh'
