#!/bin/bash
#
#  Test physical sequencer board plugged into two IOW56Paddles
#
while true
do
    echo ========================================
    date
    time sudo ./netgen.sh seqtester.mod -sim sequencer.sim
done
