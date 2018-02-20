#/bin/bash

# dolphin_ix_port_control.sh
#
# - script to enable/disable port on a Dolphin IXS600 switch
#
# - input parameters (--enable/--disable) IP-of-switch port#

readonly SWITCH_USERNAME="admin"
readonly SWITCH_PASSWORD="admin"

# Do not modify these constants
readonly MIN_PORT=1
readonly MAX_PORT=8

program_available()
{
    command -v $1 >/dev/null 2>&1

    if [ $? -gt 0 ]; then
        echo "I require $1, but it is not available from this shell";
        exit 1
    fi
}

test_parameter()
{
    case "$1" in
        --enable) 
            echo "--enable"
            ctrl_opt="enable"
            ;;
        --disable) 
            echo "--disable"
            ctrl_opt="disable"
            ;;
        --*) echo "bad parameter $1"
            ;;
        *) echo "missing parameter $1"
            ;;
    esac
    switch_ip="$2"
    port="$3"
}

port_to_offs()
{
    case "$1" in
        1) echo "0x0001C050" ;;
        2) echo "0x00018050" ;;
        3) echo "0x0000C050" ;;
        4) echo "0x00008050" ;;
        5) echo "0x00014050" ;;
        6) echo "0x00010050" ;;
        7) echo "0x00004050" ;;
        8) echo "0x00000050" ;;
        *) echo "Invalid port $1";
           exit 1 ;;
    esac
}

# Check for required programs
program_available grep
program_available cut
program_available rev
program_available curl
program_available printf
export PATH="/opt/DIS/sbin:/opt/DIS/bin:"${PATH}
program_available dis_tool

#Import parameters
test_parameter $1 $2 $3

# check port index
if [ $port -lt $MIN_PORT ] || [ $port -gt $MAX_PORT ]; then
    echo "Invalid switch port detected: $port ($MIN_PORT-$MAX_PORT)"
    exit 1;
fi

# Retrieve current port status
offset=$( port_to_offs $port )
raw=$( curl -s -u $SWITCH_USERNAME:$SWITCH_PASSWORD "http://$switch_ip/cmd.shtml" -X POST -H "Content-Type: application/x-www-form-urlencoded" -d "CMD=csr $offset" )
let reg=$( printf "%d" $( echo $raw | rev | cut -d '=' -f 2 | rev | cut -d ' ' -f 1 ) )

if [ $ctrl_opt == "disable" ] ; then
    let reg=$(( $reg | 16 ))
    echo "Disabling switch_ip $switch_ip, port $port"
else
    let reg=$(( $reg & ~16 ))
    echo "Enabling switch_no $switch_ip, port $port"
fi

raw=$( curl -s -u $SWITCH_USERNAME:$SWITCH_PASSWORD "http://$switch_ip/cmd.shtml" -X POST -H "Content-Type: application/x-www-form-urlencoded" -d "CMD=csr $offset $reg" )
echo "Complete - $raw"

