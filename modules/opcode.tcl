
quietly set opcodelist [list \
    [list 0000 halt regb] \
    [list 0002 iret] \
    [list 0004 wrps regb] \
    [list 0006 rdps regd] \
    [list 0001 br   brdisp] \
    [list 0400 beq  brdisp] \
    [list 0401 bne  brdisp] \
    [list 0800 blt  brdisp] \
    [list 0801 bge  brdisp] \
    [list 0C00 ble  brdisp] \
    [list 0C01 bgt  brdisp] \
    [list 1000 bcs  brdisp] \
    [list 1001 bcc  brdisp] \
    [list 1000 blo  brdisp] \
    [list 1001 bhis brdisp] \
    [list 1400 blos brdisp] \
    [list 1401 bhi  brdisp] \
    [list 1800 bmi  brdisp] \
    [list 1801 bpl  brdisp] \
    [list 1C00 bvs  brdisp] \
    [list 1C01 bvc  brdisp] \
    [list 2000 lsr  regd rega] \
    [list 2001 asr  regd rega] \
    [list 2002 csr  regd rega] \
    [list 2004 mov  regd regb] \
    [list 2005 neg  regd regb] \
    [list 2006 inc  regd regb] \
    [list 2007 com  regd regb] \
    [list 2008 or   regd rega regb] \
    [list 2009 and  regd rega regb] \
    [list 200A xor  regd rega regb] \
    [list 200C add  regd rega regb] \
    [list 200D sub  regd rega regb] \
    [list 200E adc  regd rega regb] \
    [list 200F sbb  regd rega regb] \
    [list 4000 stw  regd offs rega] \
    [list 6000 stb  regd offs rega] \
    [list 8000 lda  regd offs rega] \
    [list A000 ldbu regd offs rega] \
    [list C000 ldw  regd offs rega] \
    [list E000 ldbs regd offs rega]]

##
# Decode assembly line to opcode value
#  Input:
#   mne = mnemonic string from assembly source line
#   arg1,2,3,4,5 = operand strings from assembly source line
#  Output:
#   returns opcode value
##
proc opcode {mne {arg1 ""} {arg2 ""} {arg3 ""} {arg4 ""} {arg5 ""}} {
    global opcodelist
    foreach opentry $opcodelist {
        set opcmne [lindex $opentry 1]
        if {$opcmne == $mne} {
            switch [llength $opentry] {
                2 {
                    if {$arg1 != ""} {
                        error "excess operand $arg1 for $mne"
                        pause
                    }
                    set opnds 0
                }
                3 {
                    if {$arg2 != ""} {
                        error "excess operand $arg2 for $mne"
                        pause
                    }
                    set opnds [expr \
                            [operand [lindex $opentry 2] $arg1]]
                }
                4 {
                    if {$arg3 != ""} {
                        error "excess operand $arg3 for $mne"
                        pause
                    }
                    set opnds [expr \
                            [operand [lindex $opentry 2] $arg1] | \
                            [operand [lindex $opentry 3] $arg2]]
                }
                5 {
                    if {$arg4 != ""} {
                        error "excess operand $arg4 for $mne"
                        pause
                    }
                    set opnds [expr \
                            [operand [lindex $opentry 2] $arg1] | \
                            [operand [lindex $opentry 3] $arg2] | \
                            [operand [lindex $opentry 4] $arg3]]
                }
                7 {
                    set opnds [expr \
                            [operand [lindex $opentry 2] $arg1] | \
                            [operand [lindex $opentry 3] $arg2] | \
                            [operand [lindex $opentry 4] $arg3] | \
                            [operand [lindex $opentry 5] $arg4] | \
                            [operand [lindex $opentry 6] $arg5]]
                }
            }
            set opchex [lindex $opentry 0]
            set opchex [expr 0x$opchex]
            return [format "%04X" [expr {$opchex | $opnds}]]
        }
    }
}

##
# Decode assembly language operand
#  Input:
#   type = from opcodes list
#   valu = from assembly source line
##
proc operand {type valu} {
    if {$valu == ""} {
        error "missing $type operand"
        pause
    }
    switch $type {
        "brdisp" {
            if {($valu & 1) != 0} {
                error "odd brdisp $valu"
                pause
            }
            return [expr {$valu & 0x03FE}]
        }
        "offs" {
            return [expr {$valu & 0x007F}]
        }
        "regd" {
            return [expr {[genregnumber $valu] <<  7}]
        }
        "rega" {
            return [expr {[genregnumber $valu] << 10}]
        }
        "regb" {
            return [expr {[genregnumber $valu] <<  4}]
        }
        default {
            error "bad operand type $type"
            pause
        }
    }
}

##
# Decode general register number string
#  Input:
#   valu = from assembly source line
##
proc genregnumber {valu} {
    switch $valu {
         "r0" { return  0 }
         "r1" { return  1 }
         "r2" { return  2 }
         "r3" { return  3 }
         "r4" { return  4 }
         "r5" { return  5 }
         "r6" { return  6 }
         "r7" { return  7 }
         "sp" { return  6 }
         "pc" { return  7 }
        default {
            error "bad general register $valu"
            pause
        }
    }
}
