##    Copyright (C) Mike Rieker, Beverly, MA USA
##    www.outerworldapps.com
##
##    This program is free software; you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation; version 2 of the License.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
##
##    You should have received a copy of the GNU General Public License
##    along with this program; if not, write to the Free Software
##    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
##    http://www.gnu.org/licenses/gpl-2.0.html

# Compile in Qvartvs, top level module is de0
# Then do Tools->Run Simulation Tools->RTL Simulation
# When ModelSim finishes starting up, type 'do ../../testde0.do' at command prompt

set halfns 50
set fullns 100
set denans 95
set pfx "/de0/sys"

# Simulate->Start Simulation...->work->de0->OK
vsim -i -l msim_transcript work.de0

# get value of gpr as 16 binary digits
# - num = reg num 0..7
# - returns value 0..65535
proc GetGPR {num} {
    global pfx
    return [expr 0b[examine $pfx/gprs/r${num}q]]
}

# get processor status word
proc GetPS {} {
    global pfx
    set ieis [ExamNoSt $pfx/psw/IE]
    set nis  [ExamNoSt $pfx/psw/N]
    set zis  [ExamNoSt $pfx/psw/Z]
    set vis  [ExamNoSt $pfx/psw/V]
    set cis  [ExamNoSt $pfx/psw/C]
    return [expr {($ieis<<15) | 0x7FF0 | ($nis<<3) | ($zis<<2) | ($vis<<1) | $cis}]
}

# get value processor is sending to raspi
# assumes _DENA is 0, ie, MD is being gated to _MBUS through an inverter and tristate buffer
proc GetMD {} {
    if {[ExamNoSt _DENA]} {
        echo GetMD called while _DENA non-zero
        pause
    }
    set _mbus 0b[examine _MBUS]
    return [expr {$_mbus ^ 0xFFFF}]
}

# get instruction register contents
proc GetIR {} {
    global pfx
    return [expr 0b[examine $pfx/irlat]]
}

# examine boolean (0 or 1) and strip off the 'St'
proc ExamNoSt {sig} {
    set raw [examine $sig]
    if {"$raw" == "St0"} {return 0}
    if {"$raw" == "St1"} {return 1}
    return $raw
}

# format value as 16-digit binary
proc Fmt16 {val} {
    set out ""
    for {set bit 0} {$bit < 16} {incr bit} {
        set out [expr {$val & 1}]$out
        set val [expr {$val >> 1}]
    }
    return $out
}

# make sure we are in given state, abort if not
# - makes sure only the given state ff is 1, all others must be 0
set statefflist [list reset0 reset1 ireq1 ireq2 ireq3 ireq4 ireq5 fetch1 fetch2 arith1 iret1 iret2 iret3 iret4 bcc1 store1 store2 lda1 load1 load2 load3 wrps1 rdps1 halt1]
proc VerifyState {st} {
    global pfx statefflist
    set err 0
    foreach ff $statefflist {
        set ffis [ExamNoSt $pfx/seq/${ff}q]
        set ffsb [expr {($ff eq $st)}]
        if {$ffis != $ffsb} {
            if {!$err} {
                echo expecting state $st
                set err 1
            }
            echo " " but $ff is $ffis
        }
    }
    if {$err} pause
}

# make sure data going from processor to memory/raspi has given value, abort if not
# - assumes _DENA is 0
# - val = expected value 0..65535
proc VerifyMD {val} {
    set md [GetMD]
    if {$md != $val} {
        echo expecting MD $val [Fmt16 $val] but is $md [Fmt16 $md]
        pause
    }
}

# make sure gpr has the given value, abort if not
# - num = reg num 0..7
# - val = value 0..65535
proc VerifyGPR {num val} {
    set gpris [GetGPR $num]
    if {$gpris != $val} {
        echo expecting R$num to be $val but is $gpris
        pause
    }
}

# make sure the processor status word is the given value, abort if not
proc VerifyPS {pssb} {
    set psis [GetPS]
    if {$psis != $pssb} {
        echo ps is $psis should be $pssb
        pause
    }
}

# make sure the condition codes (nzvc) are as expected
# - nzvcex = expected nzvc values as a 4-digit number
proc VerifyCC {nzvcex} {
    global pfx
    set nis [ExamNoSt $pfx/psw/N]
    set zis [ExamNoSt $pfx/psw/Z]
    set vis [ExamNoSt $pfx/psw/V]
    set cis [ExamNoSt $pfx/psw/C]
    set nzvcis $nis$zis$vis$cis
    if {$nzvcis != $nzvcex} {
        echo expecting nzvc $nzvcex but is $nzvcis
        pause
    }
}

# send value to processor
# - call in middle of state asking for value, eg, FETCH2, LOAD2, etc
# - returns in middle of next state
proc SendAndStep {val} {
    global denans fullns
    force _DENA 1
    force _MBUS [Fmt16 [expr ~$val]]
    run ${denans}ns
    noforce _MBUS
    force _DENA 0
    run [expr {$fullns - $denans}]ns
}

# execute the given opcode
# - assumes in middle of FETCH1 on entry
# - returns in middle of first cycle of instruction
# opcode = 0..65535
proc DoOpcode {opcode} {
    global fullns pc

    VerifyState fetch1
    VerifyMD $pc

    run ${fullns}ns
    VerifyState fetch2
    incr pc 2
    VerifyMD $pc

    SendAndStep $opcode

    set ir [GetIR]
    if {$ir != $opcode} {
        echo IR contains $ir expect $opcode
        pause
    }
}

# do conditional branch, verify result, abort on error
# - cond = branch condition 1..15
# - disp = pc displacement -512..+510 (must be even)
proc DoBranch {cond disp} {
    global fullns pc pfx

    set n [ExamNoSt $pfx/psw/N]
    set z [ExamNoSt $pfx/psw/Z]
    set v [ExamNoSt $pfx/psw/V]
    set c [ExamNoSt $pfx/psw/C]

    DoOpcode [expr {(($cond & 14) << 9) + (($disp + 1024) & 1022) + ($cond & 1)}]
    VerifyState bcc1
    set newpc [expr {($pc + $disp) & 65535}]
    VerifyMD $newpc
    run ${fullns}ns
    VerifyState fetch1

    set btbase 0
    switch [expr {$cond >> 1}] {
        1 { set btbase $z }
        2 { set btbase [expr {$n ^ $v}] }
        3 { set btbase [expr {($n ^ $v) | $z}] }
        4 { set btbase $c }
        5 { set btbase [expr {$c | $z}] }
        6 { set btbase $n }
        7 { set btbase $v }
    }
    if {$btbase ^ ($cond & 1)} {
        set pc $newpc
    }
    VerifyMD $pc
}

# do arithmetic opcode, verify result, abort on error
# - assume currently in middle of FETCH1 state
# - returns in middle of next FETCH1 state
# - op = opcode 0..15
# - rd = destination register 0..7
# - ra = source A register 0..7
# - rb = source B register 0..7
proc DoArith {op rd ra rb} {
    global fullns pc pfx

    if {($op == 3) || ($op == 11)} {
        return
    }

    DoOpcode [expr {0x2000 | ($ra << 10) | ($rd << 7) | ($rb << 4) | $op}]
    VerifyState arith1
    set olda [GetGPR $ra]
    set oldb [GetGPR $rb]
    set oldc [ExamNoSt $pfx/psw/C]
    run ${fullns}ns
    VerifyState fetch1

    set newv 0
    set newc $oldc

    switch $op {
        # LSR
         0 {
            set newd [expr {$olda >> 1}]
            set newc [expr {$olda & 1}]
        }
        # ASR
         1 {
            set newd [expr {$olda >> 1}]
            if {$olda >= 32768} {incr newd 32768}
            set newc [expr {$olda & 1}]
        }
        # ROR
         2 {
            set newd [expr {$olda >> 1}]
            if {$oldc} {incr newd 32768}
            set newc [expr {$olda & 1}]
        }
        # MOV
         4 {
            set newd $oldb
        }
        # NEG
         5 {
            set newd [expr {(65536 - $oldb) & 65535}]
            set newv [expr {$newd == 32768}]
        }
        # INC
         6 {
            set newd [expr {($oldb + 1) & 65535}]
            set newv [expr {$newd == 32768}]
        }
        # COM
         7 {
            set newd [expr {65535 - $oldb}]
        }
        # OR
         8 {
            set newd [expr {$olda | $oldb}]
        }
        # AND
         9 {
            set newd [expr {$olda & $oldb}]
        }
        # XOR
        10 {
            set newd [expr {$olda ^ $oldb}]
        }
        # ADD
        12 {
            set sind [expr {(($olda ^ 32768) - 32768) + (($oldb ^ 32768) - 32768)}]
            set newv [expr {($sind < -32768) || ($sind > 32767)}]
            set unsd [expr {$olda + $oldb}]
            set newc [expr {$unsd > 65535}]
            set newd [expr {$unsd & 65535}]
        }
        # SUB
        13 {
            set sind [expr {(($olda ^ 32768) - 32768) - (($oldb ^ 32768) - 32768)}]
            set newv [expr {($sind < -32768) || ($sind > 32767)}]
            set unsd [expr {$olda - $oldb}]
            set newc [expr {$unsd < 0}]
            set newd [expr {($unsd + 65536) & 65535}]
        }
        # ADC
        14 {
            set sind [expr {(($olda ^ 32768) - 32768) + (($oldb ^ 32768) - 32768) + $oldc}]
            set newv [expr {($sind < -32768) || ($sind > 32767)}]
            set unsd [expr {$olda + $oldb + $oldc}]
            set newc [expr {$unsd > 65535}]
            set newd [expr {$unsd & 65535}]
        }
        # SBB
        15 {
            set sind [expr {(($olda ^ 32768) - 32768) - (($oldb ^ 32768) - 32768) - $oldc}]
            set newv [expr {($sind < -32768) || ($sind > 32767)}]
            set unsd [expr {$olda - $oldb - $oldc}]
            set newc [expr {$unsd < 0}]
            set newd [expr {($unsd + 65536) & 65535}]
        }
    }

    if {$rd == 7} {
        VerifyGPR 7 $pc
    } else {
        VerifyGPR $rd $newd
    }

    set newz [expr {$newd ==  0}]
    set newn [expr {$newd >> 15}]
    VerifyCC $newn$newz$newv$newc
}

# load immediate value into register
# - assumes in middle of FETCH1 on entry
# - returns in middle of FETCH1
# num = register number 0..7
# val = value 0..65535
proc DoLdImm {num val} {
    global fullns pc

    DoOpcode [expr {0xDC00 | ($num << 7)}] ;# LDW Rn,0(PC)
    VerifyState load1
    VerifyMD $pc

    run ${fullns}ns
    VerifyState load2

    SendAndStep $val
    if {$num == 7} {
        set pc $val
    } else {
        VerifyState load3
        incr pc 2
        VerifyMD $pc
        run ${fullns}ns
    }
    VerifyState fetch1
    VerifyMD $pc

    VerifyGPR $num $val
}

# Do a WRPS instruction
# - num = register number 0..7
proc DoWRPS {num} {
    global fullns
    DoOpcode [expr {($num << 4) + 4}]
    VerifyState wrps1
    run ${fullns}ns
    VerifyState fetch1
    VerifyPS [expr {[GetGPR $num] | 0x7FF0}]
}

# do an LDA instruction
proc DoLDA {rd disp ra} {
    global fullns pc

    DoOpcode [expr {0x8000 + ($ra << 10) + ($rd << 7) + ($disp & 0x7F)}]
    VerifyState lda1
    set oldra [GetGPR $ra]
    set sumsb [expr {($oldra + $disp) & 65535}]
    set sumis [GetMD]
    if {$sumis != $sumsb} {
        echo lda result $sumis should be $sumsb
        pause
    }
    run ${fullns}ns
    VerifyState fetch1
    set newrd [GetGPR $rd]
    if {$newrd != $sumsb} {
        echo lda register $newrd should be $sumsb
        pause
    }
    if {$rd == 7} {
        set pc $newrd
    }
}

# return random number 0..65535
proc LFSR {} {
    global lfsrseed

    set lfsrpoly 987654321

    incr lfsrseed $lfsrseed
    if {$lfsrseed > 0xFFFFFFFF} {
        set lfsrseed [expr {($lfsrseed & 0xFFFFFFFF) ^ $lfsrpoly}]
    }

    return [expr {$lfsrseed & 65535}]
}

# SCRIPT BEGINS HERE

set lfsrseed 123456789

add wave _CLOCK
add wave _HALT
add wave _MREAD
add wave _MWRITE
add wave _MWORD
add wave _DENA
add wave -hex _MBUS
add wave -hex $pfx/irlat

force _CLOCK -repeat ${fullns}ns 0 0ns, 1 ${halfns}ns

force _DENA  0
force _INTRQ 1
force _RESET 0
run ${fullns}ns
run ${halfns}ns
force _RESET 1
VerifyState reset0
run ${fullns}ns
VerifyState reset1

set pc 0

run ${fullns}ns

for {set reg 0} {$reg < 8} {incr reg} {
    set val [LFSR]
    if {$reg == 7} {set val [expr {$val & 65534}]}
    echo ldimm $reg $val
    DoLdImm $reg $val
}

for {set nzvc 0} {$nzvc < 16} {incr nzvc} {
    set reg [expr {[LFSR] % 7}]
    DoLdImm $reg $nzvc
    DoWRPS $reg
    for {set cond 1} {$cond < 16} {incr cond} {
        set disp [expr {([LFSR] & 1022) - 512}]
        echo branch $nzvc $cond $disp
        DoBranch $cond $disp
    }
}

for {set i 0} {$i < 1000} {incr i} {
    set ra [expr {[LFSR] &  7}]
    set rb [expr {[LFSR] &  7}]
    set rd [expr {[LFSR] &  7}]
    set op [expr {[LFSR] & 15}]
    set aval [LFSR]
    set bval [LFSR]
    set cval [ExamNoSt $pfx/psw/C]
    echo arith $i $op $rd $ra/$aval $rb/$bval $cval
    DoLdImm $ra $aval
    DoLdImm $rb $bval
    DoArith $op $rd $ra $rb
}

for {set ra 0} {$ra < 8} {incr ra} {
    for {set rd 0} {$rd < 8} {incr rd} {
        set disp [expr {([LFSR] & (($rd == 7) ? 126 : 127)) - 64}]
        echo LDA $rd $disp $ra
        DoLDA $rd $disp $ra
    }
}

force _INTRQ 0
DoLdImm 4 0x8005
DoWRPS 4
DoOpcode 0x8000 ;# LDA R0,0(R0), ie, NOP
VerifyState lda1
run ${fullns}ns
VerifyState ireq1
VerifyMD 0xFFFE
run ${fullns}ns
VerifyState ireq2
VerifyMD 0xFFF5
run ${fullns}ns
VerifyState ireq3
VerifyMD 0xFFFC
run ${fullns}ns
VerifyState ireq4
VerifyMD $pc
VerifyPS 0x7FF5
run ${fullns}ns
VerifyState ireq5
VerifyMD 2
set pc 2
run ${fullns}ns
VerifyState fetch1
DoOpcode 0x0002 ;# IRET
VerifyState iret1
VerifyMD 0xFFFC
run ${fullns}ns
VerifyState iret2
SendAndStep 0x1234
VerifyState iret3
VerifyMD 0xFFFE
run ${fullns}ns
VerifyState iret4
SendAndStep 0x800A
VerifyState fetch1
VerifyPS 0xFFFA
VerifyMD 0x1234
force _INTRQ 1

echo SUCCESS
