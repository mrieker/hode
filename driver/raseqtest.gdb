#!/bin/bash
cd `dirname $0`
if ! lsmod | grep -q ^enabtsc
then
    sudo insmod km/enabtsc.ko
fi
. ./iow56sns.si
exec sudo -E gdb --args ./raseqtest.armv7l "$@"
