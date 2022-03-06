#!/bin/bash
#
# Generate a propagation report
# Depends on code in NetGen.java
#
time ./master.sh fanout.mod -gen fanout -printprop propreport.txt
