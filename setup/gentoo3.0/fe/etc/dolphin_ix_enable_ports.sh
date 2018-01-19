#!/bin/sh
# /etc/dolphin_ix_enable_ports.sh
#  - enable IX switch ports for all adapters on front-end

# Get list of switches, ports for all adapters on front-end
IXCFG=`/etc/ixnodecfg.sh`

# IXCFG will be a multi-line variable. Each line has switch(1,2,3..) 
# and a port number (1-8) [possibly] followed by serial number of 
# adapter on that switch
# Create array variables with switches, ports, serial numbers in order
sn_idx=0
{ while read cfgline; do
     set $cfgline
     SWCFG[sn_idx]="$1"
     PTCFG[sn_idx]="$2"
     sn_idx=$(($sn_idx+1))
done } <<< "$IXCFG"
# Now loop over adapters. Get IP of switch, then call port control script
ix_idx=0
while [ "$ix_idx" -lt "$sn_idx" ]
do
    ipcfg=`/etc/ixswcfg.sh ${SWCFG[ix_idx]}`  
    /etc/dolphin_ix_port_control.sh "--enable" $ipcfg ${PTCFG[ix_idx]}
# let it work
    sleep 5
    ix_idx=$(($ix_idx+1))
done

