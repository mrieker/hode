#!/bin/bash
#
#  run randmem test in background
#  supplies random opcodes and data to the cpu for testing
#  runs on raspi plugged into rasboard
#  assumes whole cpu is available with all boards
#  paddles are optional
#
cd `dirname $0`
if [ "$1" == "" ]
then
    if ! lsmod | grep -q ^enabtsc
    then
        sudo insmod km/enabtsc.ko
    fi
    nohup ./`basename $0` loop < /dev/null >> randmem.log 2>&1 &
    exit
fi
if [ "$1" == "loop" ]
then
    for ((cpuhz=470000; $cpuhz != 90000; cpuhz=$cpuhz-10000))
    do
        date >> randmem-$cpuhz.log
        ./raspictl.armv7l -randmem -mintimes -cpuhz $cpuhz >> randmem-$cpuhz.log 2>&1
        date >> randmem-$cpuhz.log
        sleep 3
    done
    exit
fi
