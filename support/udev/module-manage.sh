#!/bin/bash -e

function usage() {
    local cmd=$(basename $0)
    echo "usage: $cmd add|remove <module>
load/unload module, and create corresponding device node" >&2

}

function error() {
    echo "$@" >&2
    usage
    exit 1
}

function check_module() {
    local module="$1"
    if [ -z "$module" ] ; then
	error "must specify module" 
    fi
}

function node_add() {
    local module="$1"
    check_module $module
    modprobe $module
    local dev=$(grep $module /proc/devices)
    local major=${dev%% *}
    mknod /dev/$module c $major 0 || true
    chown controls /dev/$module
}

function node_remove() {
    local module="$1"
    check_module $module
    rm -f /dev/$module
    rmmod $module 2>/dev/null || true
}

##########

cmd="$1"
module="$2"

case $cmd in
    add)
	node_add "$module"
	;;
    remove)
	node_remove "$module"
	;;
    help|-h|--help)
	usage
	;;
    *)
	error "invalid command: $cmd"
	;;
esac
