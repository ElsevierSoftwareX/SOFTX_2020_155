#!/usr/bin/env bash
echo "Kill DAQD,NDS"
#  remove DAQ from inittab and stop
sudo cp /etc/inittab /tmp/inittab0
sudo cat /tmp/inittab0 | sed 's/fb:2345/#fb:2345/' > /tmp/inittab1
sudo cat /tmp/inittab1 | sed 's/nds:2345/#nds:2345/' > /tmp/inittab2
sudo cp /tmp/inittab2 /etc/inittab
sudo init q
