#!/bin/bash -v
./master.sh -gen aluboard -net aluboard.net -report aluboard.rep -pcb aluboard.pcb -pcbmap aluboard.map -resist aluboard.png
./master.sh -gen rasboard -net rasboard.net -report rasboard.rep -pcb rasboard.pcb -pcbmap rasboard.map -resist rasboard.png
./master.sh -gen regboard -net regboard.net -report regboard.rep -pcb regboard.pcb -pcbmap regboard.map -resist regboard.png
./master.sh -gen seqboard -net seqboard.net -report seqboard.rep -pcb seqboard.pcb -pcbmap seqboard.map -resist seqboard.png
