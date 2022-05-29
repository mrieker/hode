#!/bin/bash
if [ "`uname -m`" == armv7l ]
then
    if ! lsmod | grep -q ^enabtsc
    then
        cd `dirname $0`
        sudo insmod km/enabtsc.ko
    fi
fi
