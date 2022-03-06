#!/bin/bash
exec ./netgen.sh \
    xor.mod aluboard.mod psw.mod bmux.mod memread.mod insreg.mod rasboard.mod regdff.mod regboard.mod sequencer.mod seqboard.mod master.mod \
    "$@"
