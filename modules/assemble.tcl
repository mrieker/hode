#
# Read source file and produce array called memquads
#
#  Called as a function:
#    assemble <sourcefilename>
#
#  Called from the command line:
#    tclsh assemble.tcl umul.asm umul.hex
#

# needed by opcode.tcl
proc quietly {a1 a2 a3} {
    global $a2
    set $a2 $a3
}

# get opcode parsing function
source opcode.tcl

##
# Assemble a source file into memquads array
##
proc assemble {asmname} {
    global membase
    global memquads
    global memsize
    global opcodelist
    global passno
    global section
    global sections

    set membase 0
    set memsize 8192

    for {set passno 1} {$passno <= 2} {incr passno} {
        set sections(bootrom) $membase
        set sections(bootram) [expr 0x2000]
        set section bootrom

        # read through top-level source file
        assemblefile $asmname

        array unset sections
    }

    # result is in memquads
}

proc assemblefile {asmname} {
    global labels
    global lineid
    global membase
    global memquads
    global opcodelist
    global passno
    global section
    global sections

    set asmfile [open $asmname r]
    set lineno 0
    while {[gets $asmfile asmline] >= 0} {
        incr lineno
        set lineid "$asmname:$lineno"
        set showaddr "    "
        set showdata "        "

        # trim comment off end of line
        set asmtrim $asmline
        set i [string first # $asmtrim]
        if {$i >= 0} {
            set asmtrim [string range $asmtrim 0 [expr $i-1]]
        }

        # split into words
        set asmsplit [split $asmtrim]

        # remove null strings as a result of redundant spaces
        set numwords 0
        foreach sp $asmsplit {
            if {$sp != ""} {
                set asmwords($numwords) $sp
                incr numwords
            }
        }

        # any words at beginnning ending with ":" are labels
        set wi 0
        while {($wi < $numwords) && ([string index $asmwords($wi) end] == ":")} {
            set lname [string range $asmwords($wi) 0 end-1]
            set labels($lname) $sections($section)
            incr wi
        }

        # check for blank line
        if {$numwords > $wi} {

            # get mnemonic word
            set mne $asmwords($wi)

            if {$mne == ".ascii"} {

                #  .ascii literalstring
                set showaddr [format %04X $sections($section)]
                set k 0
                set starti 0
                for {set j [expr $wi+1]} {$j < $numwords} {incr j} {
                    set strlit $asmwords($j)
                    set strlen [string length $strlit]
                    for {set i $starti} {$i < $strlen} {incr i} {
                        if {$i < 0} {
                            set chrhex 20
                        } else {
                            set strchr [string index $strlit $i]
                            scan $strchr %c chrbin
                            set chrhex [format %02X $chrbin]
                        }
                        if {$k < 4} {
                            set dataleft [string range $showdata 0 [expr 5 - $k * 2]]
                            set datarite [string range $showdata [expr 8 - $k * 2] end]
                            set showdata $dataleft$chrhex$datarite
                            incr k
                        }
                        writemembyte $sections($section) $chrhex
                        incr sections($section)
                    }
                    set starti -1
                }
            } elseif {$mne == ".byte"} {

                #  .byte bytevalues
                set showaddr [format %04X $sections($section)]
                set k 0
                for {set j [expr $wi+1]} {$j < $numwords} {incr j} {
                    set binval [expr [evalasmexpr $asmwords($j)] & 0xFF]
                    set hexval [format %02X $binval]
                    if {$k < 4} {
                        set dataleft [string range $showdata 0 [expr 5 - $k * 2]]
                        set datarite [string range $showdata [expr 8 - $k * 2] end]
                        set showdata $dataleft$hexval$datarite
                        incr k
                    }
                    writemembyte $sections($section) $hexval
                    incr sections($section)
                }
            } elseif {$mne == ".include"} {

                #  .include filename
                assemblefile $asmwords([expr $wi+1])
            } elseif {$mne == ".long"} {

                #  .long longvalues
                set showaddr [format %04X $sections($section)]
                set k 0
                for {set j [expr $wi+1]} {$j < $numwords} {incr j} {
                    set binval [evalasmexpr $asmwords($j)]
                    set hexval [format %08X $binval]
                    if {$k < 1} {
                        set showdata $hexval
                        incr k
                    }
                    writememlong $sections($section) $hexval
                    incr sections($section) 4
                }
            } elseif {$mne == ".org"} {

                #  .org byte-address
                set sections($section) [evalasmexpr $asmwords([expr $wi+1])]
                set showaddr [format %08X $sections($section)]
            } elseif {$mne == ".p2align"} {

                #  .p2align alignment
                set alignment [evalasmexpr $asmwords([expr $wi+1])]
                if {($alignment <= 0) || ($alignment > 5)} {
                    error "$lineid: bad alignment $alignment"
                }
                set alignment [expr 1 << $alignment]
                set sections($section) [expr ($sections($section) + $alignment - 1) & -$alignment]
            } elseif {$mne == ".psect"} {

                #  .psect section-name
                set section $asmwords([expr $wi+1])
                if {![info exists sections($section)]} {
                    set sections($section) 0
                }
            } elseif {$mne == ".quad"} {

                #  .quad hex-data-16-digits
                if {$sections($section) & 7} {
                    error "$lineid: unaligned location [format %08X $sections($section)]"
                }
                set qidx [expr ($sections($section) - $membase) / 8]
                set qval $asmwords([expr $wi+1])
                set memquads($qidx) $qval
                set showaddr [format %08X $sections($section)]
                incr sections($section) 8
            } elseif {$mne == ".set"} {

                #   .set label value
                set lname $asmwords([expr $wi+1])
                set value $asmwords([expr $wi+2])
                set value [evalasmexpr $value]
                set labels($lname) $value
                set showdata [format %08X $value]
            } elseif {$mne == ".word"} {

                #  .word wordvalues
                set showaddr [format %04X $sections($section)]
                set k 0
                for {set j [expr $wi+1]} {$j < $numwords} {incr j} {
                    set binval [expr [evalasmexpr $asmwords($j)] & 0xFFFF]
                    set hexval [format %04X $binval]
                    if {$k < 2} {
                        set dataleft [string range $showdata 0 [expr 3 - $k * 4]]
                        set datarite [string range $showdata [expr 8 - $k * 4] end]
                        set showdata $dataleft$hexval$datarite
                        incr k
                    }
                    writememword $sections($section) $hexval
                    incr sections($section) 2
                }
            } else {

                #  otherwise it is parsed as opcode arg1 arg2 arg3 arg4 arg5
                set arg1 ""
                set arg2 ""
                set arg3 ""
                set arg4 ""
                set arg5 ""
                if {$numwords > $wi+1} { set arg1 $asmwords([expr $wi+1]) }
                if {$numwords > $wi+2} { set arg2 $asmwords([expr $wi+2]) }
                if {$numwords > $wi+3} { set arg3 $asmwords([expr $wi+3]) }
                if {$numwords > $wi+4} { set arg4 $asmwords([expr $wi+4]) }
                if {$numwords > $wi+5} { set arg5 $asmwords([expr $wi+5]) }

                set ophex ""
                foreach opentry $opcodelist {
                    set opcmne [lindex $opentry 1]
                    if {$opcmne == $mne} {
                        switch [llength $opentry] {
                            2 {
                                if {$arg1 != ""} {
                                    error "$lineid: excess operand $arg1 for $mne"
                                }
                                set opnds 0
                            }
                            3 {
                                if {$arg2 != ""} {
                                    error "$lineid: excess operand $arg2 for $mne"
                                }
                                set opnds [expr \
                                        [operandasm [lindex $opentry 2] $arg1]]
                            }
                            4 {
                                if {$arg3 != ""} {
                                    error "$lineid: excess operand $arg3 for $mne"
                                    pause
                                }
                                set opnds [expr \
                                        [operandasm [lindex $opentry 2] $arg1] | \
                                        [operandasm [lindex $opentry 3] $arg2]]
                            }
                            5 {
                                set opnds [expr \
                                        [operandasm [lindex $opentry 2] $arg1] | \
                                        [operandasm [lindex $opentry 3] $arg2] | \
                                        [operandasm [lindex $opentry 4] $arg3]]
                            }
                            7 {
                                set opnds [expr \
                                        [operandasm [lindex $opentry 2] $arg1] | \
                                        [operandasm [lindex $opentry 3] $arg2] | \
                                        [operandasm [lindex $opentry 4] $arg3] | \
                                        [operandasm [lindex $opentry 5] $arg4] | \
                                        [operandasm [lindex $opentry 6] $arg5]]
                            }
                            default {
                                error "$lineid: bad opcode operands for $mne"
                                pause
                            }
                        }
                        set ophex [lindex $opentry 0]
                        set ophex [expr 0x$ophex]
                        set ophex [format "%04X" [expr {$ophex | $opnds}]]
                        break
                    }
                }
                if {$ophex == ""} {
                    error "$lineid: unknown opcode $mne"
                }

                if {$sections($section) & 1} {
                    error "$lineid: unaligned location [format %04X $sections($section)]"
                }

                set showaddr [format %04X $sections($section)]
                set showdata "    $ophex"

                writememword $sections($section) $ophex
                incr sections($section) 2
            }
        }

        if {$passno == 2} {
            puts [format "%4d   %s : %s  %s" $lineno $showdata $showaddr $asmline]
        }
    }
    close $asmfile
}

##
# Parse an assembly-language operand, register, address, whatever
##
proc operandasm {type valu} {
    global lineid
    global passno
    global section
    global sections

    if {$valu == ""} {
        error "$lineid: missing $type operand"
    }
    switch $type {

        # displacement value used by a branch instruction
        "brdisp" {
            set dest [evalasmexpr $valu]
            set disp [expr $dest - $sections($section) - 2]
            if {$disp & 1} {
                if {$passno == 2} {
                    error "$lineid: branch dest $disp not word aligned"
                }
                set disp [expr $disp & -2]
            }
            if {($disp < -0x0200) || ($disp > 0x01FF)} {
                if {$passno == 2} {
                    error "$lineid: branch dest $disp out of range"
                }
            }
            return [expr ($disp & 0x03FE)]
        }

        # offset value used by load and store
        "offs" {
            set offs [evalasmexpr $valu]
            if {($offs < 0xFFC0) && ($offs > 0x003F)} {
                if {$passno == 2} {
                    error "$lineid: offset $offs out of range"
                }
            }
            return [expr {($offs & 0x007F)}]
        }

        # something else like a register
        default {
            return [operand $type $valu]
        }
    }
}

##
# Evaluate an assembly language address expression
##
proc evalasmexpr {valu} {
    global labels
    global lineid
    global passno
    global section
    global sections

    set vallen [string length $valu]
    set result 0
    set operator "+"
    set i 0
    while {$i < $vallen} {
        set valchr [string index $valu $i]
        set valbin 0
        if {($valchr >= "0") && ($valchr <= "9")} {
            set j $i
            while {1} {
                incr i
                if {$i >= $vallen} break
                set valchr [string index $valu $i]
                if {$valchr == "X"} continue
                if {$valchr == "x"} continue
                if {($valchr >= "0") && ($valchr <= "9")} continue
                if {($valchr >= "a") && ($valchr <= "f")} continue
                if {($valchr >= "A") && ($valchr <= "F")} continue
                break
            }
            set valbin [expr [string range $valu $j [expr $i - 1]] + 0]
        } elseif {(($valchr >= "A") && ($valchr <= "Z")) || (($valchr >= "a") && ($valchr <= "z"))} {
            set j $i
            while {1} {
                incr i
                if {$i >= $vallen} break
                set valchr [string index $valu $i]
                if {($valchr >= "0") && ($valchr <= "9")} continue
                if {($valchr >= "a") && ($valchr <= "z")} continue
                if {($valchr >= "A") && ($valchr <= "Z")} continue
                if {$valchr == "_"} continue
                break
            }
            set lname [string range $valu $j [expr $i - 1]]
            if {[info exists labels($lname)]} {
                set valbin $labels($lname)
            } elseif {$passno == 2} {
                error "$lineid: undefined label $lname"
            }
        } elseif {$valchr == "."} {
            set valbin $sections($section)
            incr i
        } elseif {$valchr == "'"} {
            set valbin 0
            set j 0
            while {1} {
                incr i
                if {$i >= $vallen} break
                set valchr [string index $valu $i]
                if {$valchr == "'"} {
                    incr i
                    break
                }
                if {$j == 32} {
                    error "$lineid: char constant too long"
                }
                set valchr [scan $valchr %c]
                set valbin [expr $valbin | ($valchr << $j)]
                incr j 8
            }
        } else {
            error "$lineid: unknown operand character $valchr in $valu"
        }
        set result [expr $result $operator $valbin]
        if {$i >= $vallen} break
        set operator [string index $valu $i]
        if {($operator != "+") && ($operator != "-")} {
            error "$lineid: unknown operator $operator at $i in $valu"
        }
        incr i
    }
    return $result
}

##
# Write a single long to the memory
#  Input:
#   addr = byte address
#   valu = 8-char hexadecimal number
##
proc writememlong {addr valu} {
    for {set i 0} {$i < 4} {incr i} {
        set bytevalu [string range $valu [expr 6 - $i * 2] [expr 7 - $i * 2]]
        writemembyte $addr $bytevalu
        incr addr
    }
}

##
# Write a single word to the memory
#  Input:
#   addr = byte address
#   valu = 4-char hexadecimal number
##
proc writememword {addr valu} {
    writemembyte $addr [string range $valu 2 3]
    incr addr
    writemembyte $addr [string range $valu 0 1]
    incr addr
}

##
# Write a single byte to the memory
#  Input:
#   addr = byte address
#   valu = 2-char hexadecimal number
##
proc writemembyte {addr valu} {
    global lineid
    global membase
    global memquads
    global memsize

    if {($addr < $membase) || ($addr - $membase >= $memsize)} {
        error "$lineid: address [format %04X $addr] out of range"
    }

    set qidx [expr ($addr - $membase) / 8]
    set qval 0000000000000000
    if {[info exists memquads($qidx)]} {
        set qval $memquads($qidx)
    }
    set bidx [expr ($addr - $membase) % 8]
    set qvalleft [string range $qval 0 [expr 13 - $bidx * 2]]
    set qvalrite [string range $qval [expr 16 - $bidx * 2] end]

    set memquads($qidx) $qvalleft$valu$qvalrite
}

# if invoked from the command line
if {[info exists argc] && ($argc > 0)} {
    assemble [lindex $argv 0] 

    set hexname [lindex $argv 1]
    set hexfile [open $hexname w]
    foreach {key val} [array get memquads] {
        puts $hexfile "[format %04X [expr $key * 8]] $val"
    }
    close $hexfile

    exit
}
