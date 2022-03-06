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
##    http:##www.gnu.org/licenses/gpl-2.0.html

# test with
#  ./master.sh -sim sequencer.sim

#   reset:
#   reset.1:    BMUX.CON0 ; ALU.BONLY ; GPRS.WE7 ; PSW.WE ; PSW.IECLR


#   fetch:
#   fetch.1:    GPRS.REA7 ; BMUX.CON0 ; ALU.ADD ; MEM.READ ; MEM.WORD
#   fetch.2:    GPRS.REA7 ; BMUX.CON2 ; ALU.ADD ; GPRS.WE7 ; IR.WE


#   bcc:        0 0 0 _ _ _ x x x x x x x x x _
#   bcc.1:      GPRS.REA7 ; BMUX.IRBROF ; ALU.ADD ; GPRS.WE7 = PSW.BT

#   arith:      0 0 1 a a a d d d b b b _ _ _ _
#   arith.1:    GPRS.REA ; GPRS.REB ; ALU.IRFC ; GPRS.WED ; PSW.ALUWE


#   STW:        0 1 0 a a a d d d x x x x x x x
#   STB:        0 1 1 a a a d d d x x x x x x x
#   store.1:    GPRS.REA ; BMUX.IRLSOF ; ALU.ADD ; MEM.WRITE
#   store.2:    GPRS.RED2B ; ALU.BONLY


#   LDA:        1 0 0 a a a d d d x x x x x x x
#   lda.1:      GPRS.REA ; BMUX.IRLSOF ; ALU.ADD ; GPRS.WED


#   LDBU:       1 0 1 a a a d d d x x x x x x x
#   LDW:        1 1 0 a a a d d d x x x x x x x
#   LDBS:       1 1 1 a a a d d d x x x x x x x
#   load.1:     GPRS.REA ; BMUX.IRLSOF ; ALU.ADD ; MEM.READ
#   load.2:     MEM.REB ; ALU.BONLY ; GPRS.WED
#   load.3:     [offs=0 & rega=7] GPRS.REA7 ; BMUX.CON2 ; ALU.ADD ; GPRS.WE7


#   ireq:
#   ireq.1:     BMUX.CON-4 ; BMUX.CON2 ; ALU.BONLY ; MEM.WRITE ; MEM.WORD
#   ireq.2:     PSW.REB ; ALU.BONLY
#   ireq.3:     BMUX.CON-4 ; ALU.BONLY ; MEM.WRITE ; MEM.WORD
#   ireq.4:     PSW.IECLR ; GPRS.REA7 ; BMUX.CON0 ; ALU.ADD
#   ireq.5:     BMUX.CON2 ; ALU.BONLY ; GPRS.WE7


#   --------------------

#       misc instructions
#               0 0 0 0 0 0 _ _ _ _ _ _ _ _ _ 0

#   halt:       0 0 0 0 0 0 _ _ _ b b b _ 0 0 0
#   halt.1:     HALT ; GPRS.REB ; ALU.BONLY

#   iret:       0 0 0 0 0 0 _ _ _ _ _ _ _ 0 1 0
#   iret.1:     BMUX.CON-4 ; ALU.BONLY ; MEM.READ ; MEM.WORD
#   iret.2:     MEM.REB ; MEM.WORD ; ALU.BONLY ; GPRS.WE7
#   iret.3:     BMUX.CON-4 ; BMUX.CON2 ; ALU.BONLY ; MEM.READ ; MEM.WORD
#   iret.4:     MEM.REB ; MEM.WORD ; ALU.BONLY ; PSW.WE

#   wrps:       0 0 0 0 0 0 _ _ _ b b b _ 1 0 0
#   wrps.1:     GPRS.REB ; ALU.BONLY ; PSW.WE

#   rdps:       0 0 0 0 0 0 d d d _ _ _ _ 1 1 0
#   rdps.1:     PSW.REB ; ALU.BONLY ; GPRS.WED

#   --------------------


#   Arith Function Codes
#   --------------------

#   0000   0  LSR     d = a >>> 1               NZVC
#   0001   1  ASR     d = a >>  1               NZVC
#   0010   2  CSR     d = a >>> 1 | C << 15     NZVC
#   0011   3
#   0100   4  MOV     d =   b                   NZ0
#   0101   5  NEG     d = - b                   NZV
#   0110   6  INC     d =   b + 1               NZV
#   0111   7  COM     d = ~ b                   NZ0
#   1000   8  OR      d = a | b                 NZ0
#   1001   9  AND     d = a & b                 NZ0
#   1010  10  XOR     d = a ^ b                 NZ0
#   1011  11
#   1100  12  ADD     d = a + b                 NZVC
#   1101  13  SUB     d = a - b                 NZVC
#   1110  14  ADC     d = a + b + c             NZVC
#   1111  15  SBB     d = a - b - c             NZVC

#   Branch Codes
#   ------------
#   000 0   never (special instructions)
#   000 1   always  br
#   001 0   Z=1     beq
#   001 1   Z=0     bne
#   010 0   N^V=1   blt
#   010 1   N^V=0   bge
#   011 0   N^V+Z=1 ble
#   011 1   N^V+Z=0 bgt
#   100 0   C=1     bcs,blo
#   100 1   C=0     bcc,bhis
#   101 0   C+Z=1   blos
#   101 1   C+Z=0   bhi
#   110 0   N=1     bmi
#   110 1   N=0     bpl
#   111 0   V=1     bvs
#   111 1   V=0     bvc

module sequencer (
        out ALU_ADD,
        out ALU_IRFC,
        out ALU_BONLY,
        out BMUX_CON_0,
        out BMUX_CON_2,
        out BMUX_CON__4,
        out BMUX_IRBROF,
        out BMUX_IRLSOF,
        out GPRS_WE7,
        out GPRS_REA7,
        out GPRS_WED,
        out GPRS_REA,
        out GPRS_REB,
        out GPRS_RED2B,
        out _IR_WE,
        out MEM_READ,
        out MEM_WORD,
        out MEM_WRITE,
        out MEM_REB,
        out _PSW_IECLR,
        out PSW_ALUWE,
        out PSW_WE,
        out _PSW_REB,
        out HALT,

        in CLK2,
        in RESET,
        in IR[15:0],
        in _IR[15:0],
        in IRQ,
        in PSW_IE,
        in PSW_BT)
{
    wire _res_a, _res_b, _res_c, _res_d, _res_e;
    wire _clk1, clk0_a, clk0_b, clk0_c;

    wire reset0q, reset1q, _reset1q;

    wire _ireq1d;
    wire ireq1q, ireq2q, ireq3q, ireq4q, ireq5q;
    wire _ireq1q, _ireq2q, _ireq3q, _ireq4q, _ireq5q;

    wire _fetch1d;
    wire fetch1q, fetch2q;
    wire _fetch1q, _fetch2q;

    wire exec1d, exec2d, exec3d, _exec4d;
    wire exec1q, exec2q, exec3q, exec4q;
    wire _exec1q, _exec2q, _exec3q, _exec4q;
    wire exec1q_a, exec1q_b, exec1q_c;

    wire _irqpend, instend;

    wire specialinstr, topthreezero;
    wire haltinstr, wrpsinstr, rdpsinstr;
    wire _iretinstr, iretinstr_a, iretinstr_b;
    wire bccinstr, arithinstr;
    wire storeinstr, ldainstr, loadinstr, meminstr, loadimm;

    _res_a = ~RESET;
    _res_b = ~RESET;
    _res_c = ~RESET;
    _res_d = ~RESET;
    _res_e = ~RESET;

    _clk1  = ~CLK2;
    clk0_a = ~_clk1;
    clk0_b = ~_clk1;
    clk0_c = ~_clk1;

    # resetting processor
    reset0: DFF led1 (Q: reset0q, _PS: _res_a, _PC: _reset1q, T:0, D:0);
    reset1: DFF led1 (_PS:1, T: clk0_a, _PC: _res_a, Q: reset1q, _Q: _reset1q, D: reset0q);

    # - interrupt request
    ireq1:  DFF led0 (_PC:1, T: clk0_a, _PS: _res_a, Q: _ireq1q, _Q: ireq1q, D: _ireq1d);
    ireq2:  DFF led1 (_PS:1, T: clk0_a, _PC: _res_b, Q: ireq2q, _Q: _ireq2q, D: ireq1q);
    ireq3:  DFF led1 (_PS:1, T: clk0_a, _PC: _res_b, Q: ireq3q, _Q: _ireq3q, D: ireq2q);
    ireq4:  DFF led1 (_PS:1, T: clk0_b, _PC: _res_b, Q: ireq4q, _Q: _ireq4q, D: ireq3q);
    ireq5:  DFF led1 (_PS:1, T: clk0_b, _PC: _res_c, Q: ireq5q, _Q: _ireq5q, D: ireq4q);

    # - fetch instruction
    fetch1: DFF led0 (_PC:1, T: clk0_b, _PS: _res_c, Q: _fetch1q, _Q: fetch1q, D: _fetch1d);
    fetch2: DFF led1 (_PS:1, T: clk0_b, _PC: _res_c, Q: fetch2q, _Q: _fetch2q, D: fetch1q);

    # - execute instruction
    exec1:  DFF led1 (_PS:1, T: clk0_c, _PC: _res_d, Q: exec1q, _Q: _exec1q, D: exec1d);
    exec2:  DFF led1 (_PS:1, T: clk0_c, _PC: _res_d, Q: exec2q, _Q: _exec2q, D: exec2d);
    exec3:  DFF led1 (_PS:1, T: clk0_c, _PC: _res_e, Q: exec3q, _Q: _exec3q, D: exec3d);
    exec4:  DFF led0 (_PC:1, T: clk0_c, _PS: _res_e, Q: _exec4q, _Q: exec4q, D: _exec4d);

    # we output different controls based on what instruction was fetched
    topthreezero = ~(IR[15] | IR[14] | IR[13]);
    specialinstr = ~(IR[12] | IR[11] | IR[10] | IR[0]);

    haltinstr   = topthreezero & specialinstr & _IR[2] & _IR[1];    # halt instruction  (opcode 000000.......000)
    _iretinstr  = ~(topthreezero & specialinstr & _IR[2] &  IR[1]); # iret instruction  (opcode 000000.......010)
    wrpsinstr   = topthreezero & specialinstr &  IR[2] & _IR[1];    # wrps instruction  (opcode 000000.......100)
    rdpsinstr   = topthreezero & specialinstr &  IR[2] &  IR[1];    # rdps instruction  (opcode 000000.......110)
    bccinstr    = topthreezero & ~specialinstr;                     # bcc instruction   (opcode 000___........._ where ___ _ != 000 0)
    arithinstr  = _IR[15] & _IR[14] & IR[13];                       # arith instruction (opcode 001.............)
    storeinstr  = _IR[15] &  IR[14];                                # store instruction (opcode 01..............)
    ldainstr    =  IR[15] & _IR[14] & _IR[13];                      # LDA instruction   (opcode 100.............)
    loadinstr   =  IR[15] &  IR[14] | IR[15] & _IR[14] & IR[13];    # load instruction  (opcode 11.............. and 101.............)
    meminstr    = ~(_IR[15] & _IR[14]);                             # store/LDA/load instruction

    iretinstr_a = ~_iretinstr;
    iretinstr_b = ~_iretinstr;

    # extend load to increment PC after a LDx rd,0(PC)
    loadimm = IR[12] & IR[11] & IR[10] & _IR[6] & _IR[5] & _IR[4] & _IR[3] & _IR[2] & _IR[1] & _IR[0];

    # doing last cycle of instruction, determine whether fetch_1 or irq_1 is next
    instend = reset1q | exec1q & ~exec2d | exec2q & ~exec3d | exec3q & _exec4d | exec4q | ireq5q;

    _irqpend = ~(IRQ & PSW_IE);
    _ireq1d  = ~(~_irqpend & instend);  # irq_1 is next
    _fetch1d = ~( _irqpend & instend);  # fetch_1 is next

    # how many states to execute instruction depends on the instruction
     exec1d = fetch2q;
     exec2d = ~(_exec1q | (~storeinstr & ~loadinstr & _iretinstr));
     exec3d = ~(_exec2q | ~(loadinstr & loadimm | iretinstr_a));
    _exec4d = ~(exec3q & iretinstr_a);

    exec1q_a = ~_exec1q;
    exec1q_b = ~_exec1q;
    exec1q_c = ~_exec1q;

    # the 'microcode ROM' - determine control outputs based on current state
    ALU_ADD     = ~(_ireq4q & _fetch1q & _fetch2q & ~(exec1q_a & bccinstr) & ~(exec1q_a & meminstr) & ~(exec3q & loadinstr));
    ALU_BONLY   = ~(ALU_ADD | ALU_IRFC);
    ALU_IRFC    = exec1q_a & arithinstr;

    BMUX_CON_0  = ~(_reset1q & _fetch1q & _ireq4q);
    BMUX_CON_2  = fetch2q | exec3q & loadinstr | ireq1q | ireq5q | exec3q & iretinstr_a;
    BMUX_CON__4 = ireq1q | ireq3q | exec1q_a & iretinstr_a | exec3q & iretinstr_a;
    BMUX_IRBROF = exec1q_a & bccinstr;
    BMUX_IRLSOF = exec1q_a & meminstr;

    GPRS_REA7   = fetch1q | fetch2q | exec1q_a & bccinstr | exec3q & loadinstr & loadimm | ireq4q;
    GPRS_REA    = exec1q_a & arithinstr | exec1q_a & meminstr;
    GPRS_REB    = exec1q_b & arithinstr | exec1q_b & wrpsinstr | exec1q_b & haltinstr;
    GPRS_RED2B  = exec2q & storeinstr;
    GPRS_WE7    = reset1q | ireq5q | fetch2q | exec1q_b & bccinstr & PSW_BT | exec2q & iretinstr_b | exec3q & loadinstr;
    GPRS_WED    = exec1q_b & arithinstr | exec1q_b & ldainstr | exec2q & loadinstr | exec1q_b & rdpsinstr;

    HALT        = exec1q_b & haltinstr;

    _IR_WE      = _fetch2q;

    MEM_READ    = fetch1q | exec1q_c & loadinstr | exec1q_c & iretinstr_b | exec3q & iretinstr_b;
    MEM_REB     = exec2q & loadinstr | exec2q & iretinstr_b | exec4q;
    MEM_WORD    = ~(exec1q_c & meminstr & IR[13]) & ~(exec2q & meminstr & IR[13]);
    MEM_WRITE   = exec1q_c & storeinstr | ireq1q | ireq3q;

    PSW_ALUWE   = ALU_IRFC;
    _PSW_IECLR  = ~(reset1q | ireq4q);
    _PSW_REB    = ~(ireq2q | exec1q_c & rdpsinstr);
    PSW_WE      = reset1q | exec4q & iretinstr_a | exec1q_c & wrpsinstr;
}
