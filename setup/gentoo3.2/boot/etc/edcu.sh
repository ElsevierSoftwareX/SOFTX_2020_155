#!/bin/bash

cat << EOF
[default]
gain=1.00
datatype=4
ifoid=0
slope=6.1028e-05
acquire=1
offset=0
units=V
dcuid=4
datarate=16

EOF

cat /opt/rtcds/llo/l1/target/*/*/*/*/*.db | grep L1: | sed 's/.*\(L1:.*\)".*/\[\1\]/g' | awk '{ if (length < 36) print }' | sort | uniq 
echo "# The following (if any) channels were commented out, they are longer than 34 chars and the frame builder will not take that"
cat /opt/rtcds/llo/l1/target/*/*/*/*/*.db | grep L1: | sed 's/.*\(L1:.*\)".*/\[\1\]/g' | awk '{ if (length >= 36) print "#" $0 }' | sort | uniq 
