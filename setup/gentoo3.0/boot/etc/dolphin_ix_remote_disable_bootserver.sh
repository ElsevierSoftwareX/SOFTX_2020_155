#!/bin/sh
# /etc/dolphin_ix_remote_disable_bootserver.sh
#  - Disable IX switch ports for all adapters on specified front-end
# ** Do this from boot server **

# Get list of switches, ports for all adapters on front-end
FENAME="$1"
IXCFG=`grep -v \^# /diskless/root/etc/ixnodetab | grep $FENAME | sed s/$FENAME//`
IXCFG=$(sed 's/^[ \t]*//;s/[ \t]*$//' <<< "$IXCFG")

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
     swname=${SWCFG[ix_idx]}
     ipcfg=`grep -v \^# /diskless/root/etc/ixswtab | grep \^$swname | sed s/"^$swname "//`
     # remove leading whitespace
     ipcfg="${ipcfg#"${ipcfg%%[![:space:]]*}"}"    
     /diskless/root/etc/dolphin_ix_port_control.sh "--disable" $ipcfg ${PTCFG[ix_idx]}
# let it work
    sleep 5
    ix_idx=$(($ix_idx+1))
done


