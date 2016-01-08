#!/bin/sh
# /etc/start_dolphin_node.sh
#  - Start dolphin node on front-end

# Stop any existing node manager
/etc/init.d/dis_nodemgr stop

# Set node to start an unused node ID (communication with nodemgr will set correctly)
/opt/DIS/sbin/dxconfig -c 1 -a 0 -slw 4 -n 4

# Start up Dolphin node manager
/etc/init.d/dis_nodemgr start

