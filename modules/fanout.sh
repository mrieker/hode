#!/bin/bash
./master.sh fanout.mod -gen fanout -report fanout.rep 2>&1 | grep -v 'position 0.0,0.0 collision'
grep -i exceed fanout.rep
