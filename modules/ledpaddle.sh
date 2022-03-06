#!/bin/bash
rm -f ledpaddle.net
exec ./netgen.sh ledpaddle.mod -gen ledpaddle -net ledpaddle.net -report ledpaddle.rep -pcbmap ledpaddle.map -pcb ledpaddle.pcb
