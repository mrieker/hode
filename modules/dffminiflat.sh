#!/bin/bash
#
#  Generate circuit board with single mini-flat d flipflop
#  ...to manually inspect pre-wiring
#
cd `dirname $0`
exec ./netgen.sh dffminiflat.mod -gen dffminiflat -pcb dffminiflat.pcb
