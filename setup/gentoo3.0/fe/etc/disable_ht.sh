#!/bin/bash

# Script to disable hyperthreading (needed for real-time models)
# K. Thorne - ported from the Internet

# Be careful to not skip the space at the beginning nor the end
CPUS_TO_SKIP=" $(cat /sys/devices/system/cpu/cpu*/topology/thread_siblings_list | cut -d '-' -f 1 | sort | uniq | tr "\r\n" "  ") "

for CPU_PATH in /sys/devices/system/cpu/cpu[0-9]*; do
    CPU="$(echo $CPU_PATH | tr -cd "0-9")"
    echo "$CPUS_TO_SKIP" | grep " $CPU " > /dev/null
    if [ $? -ne 0 ]; then
        echo 0 > $CPU_PATH/online
    fi
done
