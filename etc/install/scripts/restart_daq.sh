#!/usr/bin/env bash
#  add DAQ to inittab and start
echo " Restart standalone DAQ, NDS"
if [ -f /etc/init.d/monit ]; then
    sudo /etc/init.d/monit restart
else
    sudo cp /etc/inittab /tmp/inittab0
    sudo cat /tmp/inittab0 | sed 's/#fb:2345/fb:2345/' > /tmp/inittab1
    sudo cat /tmp/inittab1 | sed 's/#nds:2345/nds:2345/' > /tmp/inittab2
    sudo cp /tmp/inittab2 /etc/inittab
    sudo init q
fi
