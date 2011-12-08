#!/usr/bin/env bash
echo "Kill DAQD,NDS"

if [ -f /etc/init.d/daqd ]; then
  sudo /etc/init.d/monit stop
  sudo /etc/init.d/daqd stop
  sudo /etc/init.d/nds stop
else
#  remove DAQ from inittab and stop
  sudo cp /etc/inittab /tmp/inittab0
  sudo cat /tmp/inittab0 | sed 's/fb:2345/#fb:2345/' > /tmp/inittab1
  sudo cat /tmp/inittab1 | sed 's/nds:2345/#nds:2345/' > /tmp/inittab2
  sudo cp /tmp/inittab2 /etc/inittab
  sudo init q
fi
