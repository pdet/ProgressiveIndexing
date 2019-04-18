#!/bin/bash

NR_HUGEPAGES=2500

MOUNTPOINT=/mnt/hugetlbfs
mkdir -p "${MOUNTPOINT}"

if mountpoint -q "${MOUNTPOINT}"
then
    echo "hugetlbfs already mounted"
else
    mount -t hugetlbfs nodev "${MOUNTPOINT}"
fi

NR_NUMANODES=$(numactl -H | head -n 1 | awk '{print $2;}')


if [ ${NR_NUMANODES} -ne 1 ];
then
    echo 0 > /proc/sys/vm/nr_hugepages
    numactl -m 0 echo "${NR_HUGEPAGES}" > /proc/sys/vm/nr_hugepages_mempolicy
    cat /sys/devices/system/node/node*/meminfo | fgrep Huge
else
    echo "${NR_HUGEPAGES}" > /proc/sys/vm/nr_hugepages
fi
