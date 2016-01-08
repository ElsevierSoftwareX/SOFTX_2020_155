#!/bin/sh
# /etc/dolphin_ix_start_node.sh
#  - Start dolphin IX node on front-end

# Stop any existing IX node manager
/etc/init.d/dis_ix_nodemgr stop

# Set node to start an unused node ID (communication with nodemgr will set correctly)
/opt/DIS-IX/sbin/ixconfig -c 1 -a 0 -n 4

# Start up Dolphin node manager
/etc/init.d/dis_ix_nodemgr start

