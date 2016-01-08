#!/bin/bash

echo Killing all front-ends
/etc/allrtbg.sh 'bash -c "sudo /etc/kill_fes.sh; sudo /etc/kill_epics.sh"&'
