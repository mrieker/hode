#!/bin/bash -v
#
#  Runs all the tests for master board combination
#  Takes about 32 mins
#
./fanout.sh
time ./master.sh -sim aluboard.sim  | tail
time ./master.sh -sim bmux.sim      | tail
time ./master.sh -sim dff.sim       | tail
time ./master.sh -sim pswreg.sim    | tail
time ./master.sh -sim rasboard.sim  | tail
time ./master.sh -sim regdff.sim    | tail
time ./master.sh -sim sequencer.sim | tail
echo ============================
echo M_PRINTLN:multiply result 093F9AE5
echo in decimal 39653
time ./master.sh -sim master.sim    | tail -15
