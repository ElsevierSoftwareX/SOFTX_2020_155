#!/bin/bash

echo Starting all front-ends
/etc/allrtbg.sh 'sudo /etc/start_epics.sh; sleep 2; sudo /etc/start_fes.sh'
