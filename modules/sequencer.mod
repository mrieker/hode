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
#   halt.1:     HALT ; GPR.REB ; ALU.BONLY

#   iret:       0 0 0 0 0 0 _ _ _ _ _ _ _ 0 1 0
#   iret.1:     BMUX.CON-4 ; ALU.BONLY ; MEM.READ ; MEM.WORD
#   iret.2:     MEM.REB ; MEM.WORD ; ALU.BONLY ; GPRS.WE7
#   iret.3:     BMUX.CON-4 ; BMUX.CON2 ; ALU.BONLY ; MEM.READ ; MEM.WORD
#   iret.4:     MEM.REB ; MEM.WORD ; ALU.BONLY ; PSW.WE


#   wrps:       0 0 0 0 0 0 _ _ _ b b b _ 1 0 0
#   wrps.1:     GPRS.REB ; ALU.BONLY ; PSW.WE
#                   if being used to enable interrupts,
#                     interrupts won't be recognized until end of next instruction executed
#                     the IE signal going into the sequencer doesn't get set until a few
#                     nanoseconds into the beginning of the next fetch.1 state


#   rdps:       0 0 0 0 0 0 d d d _ _ _ _ 1 1 0
#   rdps.1:     PSW.REB ; ALU.BONLY ; GPRS.WED

#   --------------------


#   Arith Function Codes
#   --------------------
#   0000   0  LSR     d = a >>> 1               NZ0C
#   0001   1  ASR     d = a >>  1               NZ0C
#   0010   2  ROR     d = a >>> 1 | C << 15     NZ0C
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

#   Branch Codes (see psw.mod _btbase = ...)
#   ----------------------------------------
#   000 0   never (special instructions)
#   000 1   always  br          0001 + offset
#   001 0   Z=1     beq         0400 + offset
#   001 1   Z=0     bne         0401 + offset
#   010 0   N^V=1   blt         0800 + offset
#   010 1   N^V=0   bge         0801 + offset
#   011 0   N^V+Z=1 ble         0C00 + offset
#   011 1   N^V+Z=0 bgt         0C01 + offset
#   100 0   C=1     bcs,blo     1000 + offset
#   100 1   C=0     bcc,bhis    1001 + offset
#   101 0   C+Z=1   blos        1400 + offset
#   101 1   C+Z=0   bhi         1401 + offset
#   110 0   N=1     bmi         1800 + offset
#   110 1   N=0     bpl         1801 + offset
#   111 0   V=1     bvs         1C00 + offset
#   111 1   V=0     bvc         1C01 + offset

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
    wire _res_a, _res_b, _res_c, _res_d, _res_e, _res_f, _res_g, _res_h;
    wire _clk1, clk0_a, clk0_b, clk0_c, clk0_d, clk0_e;

    wire reset0q, reset1q, _reset1q;
    wire _ireq1d, ireq1q, ireq2q, ireq3q, ireq4q, ireq5q;
    wire _ireq1q, _ireq2q, _ireq3q, _ireq4q, _ireq5q;
    wire _fetch1d, fetch1q, fetch2q, fetch2q_a, fetch2q_b, _fetch1q, _fetch2q, _fetch2q_a;
    wire _arith1d, arith1q, _arith1q;
    wire _iret1d, iret1q, iret2q, iret3q, iret4q;
    wire _iret1q, _iret2q, _iret3q, _iret4q;
    wire instend;
    wire _bcc1d, bcc1q, _bcc1q;
    wire _store1d, store1q, store2q, _store1q, _store2q;
    wire _lda1d, lda1q, _lda1q;
    wire _load1d, load1q, load2q, load3q, _load1q, _load2q, _load3q;
    wire _loadimm, _load2s, _load3d;
    wire _wrps1d, _rdps1d, wrps1q, rdps1q, _wrps1q, _rdps1q;
    wire _halt1d, halt1q, _halt1q;
    wire _irqpend, rdnotpc, _arithnotpc;
    wire specialinstr, topthreezero;
    wire ir2, _ir2;

    _res_a = ~RESET;
    _res_b = ~RESET;
    _res_c = ~RESET;
    _res_d = ~RESET;
    _res_e = ~RESET;
    _res_f = ~RESET;
    _res_g = ~RESET;
    _res_h = ~RESET;

    _clk1  = ~CLK2;
    clk0_a = ~_clk1;
    clk0_b = ~_clk1;
    clk0_c = ~_clk1;
    clk0_d = ~_clk1;
    clk0_e = ~_clk1;

    _fetch2q_a = ~fetch2q;
    fetch2q_a = ~_fetch2q;
    fetch2q_b = ~_fetch2q;

    ir2 = ~_IR[2];
    _ir2 = ~IR[2];

    # resetting processor
    reset0: DFF led1 (Q: reset0q, _PS: _res_a, _PC: _reset1q, T:0, D:0);
    reset1: DFF led1 (_PS:1, T: clk0_a, _PC: _res_a, Q: reset1q, _Q: _reset1q, D: reset0q);

    # - interrupt request
    ireq1:  DFF led0 (_PC:1, T: clk0_a, _PS: _res_a, Q: _ireq1q, _Q: ireq1q, D: _ireq1d);
    ireq2:  DFF led1 (_PS:1, T: clk0_a, _PC: _res_b, Q: ireq2q, _Q: _ireq2q, D: ireq1q);
    ireq3:  DFF led1 (_PS:1, T: clk0_a, _PC: _res_b, Q: ireq3q, _Q: _ireq3q, D: ireq2q);
    ireq4:  DFF led1 (_PS:1, T: clk0_b, _PC: _res_b, Q: ireq4q, _Q: _ireq4q, D: ireq3q);
    ireq5:  DFF led1 (_PS:1, T: clk0_a, _PC: _res_c, Q: ireq5q, _Q: _ireq5q, D: ireq4q);

    # - fetch instruction
    fetch1: DFF led0 (_PC:1, T: clk0_b, _PS: _res_c, Q: _fetch1q, _Q: fetch1q, D: _fetch1d);
    fetch2: DFF led1 (_PS:1, T: clk0_b, _PC: _res_c, Q: fetch2q, _Q: _fetch2q, D: fetch1q);

    # - arithmetic instruction
    arith1: DFF led0 (_PC:1, T: clk0_d, _PS: _res_f, Q: _arith1q, _Q: arith1q, D: _arith1d);

    # - interrupt return instruction
    iret1:  DFF led0 (_PC:1, T: clk0_d, _PS: _res_g, Q: _iret1q, _Q: iret1q, D: _iret1d);
    iret2:  DFF led1 (_PS:1, T: clk0_e, _PC: _res_h, Q: iret2q, _Q: _iret2q, D: iret1q);
    iret3:  DFF led1 (_PS:1, T: clk0_e, _PC: _res_h, Q: iret3q, _Q: _iret3q, D: iret2q);
    iret4:  DFF led1 (_PS:1, T: clk0_e, _PC: _res_h, Q: iret4q, _Q: _iret4q, D: iret3q);

    # - branch conditional instruction
    bcc1:   DFF led0 (_PC:1, T: clk0_d, _PS: _res_f, Q: _bcc1q, _Q: bcc1q, D: _bcc1d);

    # - store instruction
    store1: DFF led0 (_PC:1, T: clk0_c, _PS: _res_e, Q: _store1q, _Q: store1q, D: _store1d);
    store2: DFF led1 (_PS:1, T: clk0_c, _PC: _res_e, Q: store2q, _Q: _store2q, D: store1q);

    # - load address instruction
    lda1:   DFF led0 (_PC:1, T: clk0_d, _PS: _res_e, Q: _lda1q, _Q: lda1q, D: _lda1d);

    # - load instruction
    load1:  DFF led0 (_PC:1, T: clk0_c, _PS: _res_d, Q: _load1q, _Q: load1q, D: _load1d);
    load2:  DFF led1 (_PS:1, T: clk0_b, _PC: _res_d, Q: load2q, _Q: _load2q, D: load1q);
    load3:  DFF led0 (_PC:1, T: clk0_c, _PS: _res_d, Q: _load3q, _Q: load3q, D: _load3d);

    # - write and read processor status instruction
    wrps1:  DFF led0 (_PC:1, T: clk0_e, _PS: _res_g, Q: _wrps1q, _Q: wrps1q, D: _wrps1d);
    rdps1:  DFF led0 (_PC:1, T: clk0_e, _PS: _res_g, Q: _rdps1q, _Q: rdps1q, D: _rdps1d);

    # - halt instruction
    halt1:  DFF led0 (_PC:1, T: clk0_d, _PS: _res_f, Q: _halt1q, _Q: halt1q, D: _halt1d);

    # doing arithmetic instruction with rd != pc
    rdnotpc     = ~(IR[7] & IR[8] & IR[9]);
    _arithnotpc = ~(arith1q & rdnotpc);

    # we are currently doing fetch_2 cycle, so use the incoming instruction to determine which instruction sequence to start
    topthreezero = ~(IR[15] | IR[14] | IR[13]);
    specialinstr = ~(IR[12] | IR[11] | IR[10] | IR[0]);

    _halt1d   = ~(fetch2q_a & topthreezero & specialinstr & _ir2 & _IR[1]);             # halt instruction  (opcode 000000.......000)
    _iret1d   = ~(fetch2q_a & topthreezero & specialinstr & _ir2 &  IR[1]);             # iret instruction  (opcode 000000.......010)
    _wrps1d   = ~(fetch2q_a & topthreezero & specialinstr &  ir2 & _IR[1]);             # wrps instruction  (opcode 000000.......100)
    _rdps1d   = ~(fetch2q_a & topthreezero & specialinstr &  ir2 &  IR[1]);             # rdps instruction  (opcode 000000.......110)
    _bcc1d    = ~(fetch2q_a & topthreezero & ~specialinstr);                            # bcc instruction   (opcode 000___........._ where ___ _ != 000 0)
    _arith1d  = ~(fetch2q_a & _IR[15] & _IR[14] & IR[13]);                              # arith instruction (opcode 001.............)
    _store1d  = ~(fetch2q_b & _IR[15] & IR[14]);                                        # store instruction (opcode 01..............)
    _lda1d    = ~(fetch2q_b & IR[15] & _IR[14] & _IR[13]);                              # LDA instruction   (opcode 100.............)
    _load1d   = ~(fetch2q_b & IR[15] & IR[14] | fetch2q_b & IR[15] & _IR[14] & IR[13]); # load instruction  (opcode 11.............. and 101.............)

    # extend load to increment PC after a LDx rd,0(PC) where rd != pc
    _loadimm = ~(IR[12] & IR[11] & IR[10] & rdnotpc & _IR[6] & _IR[5] & _IR[4] & _IR[3] & _ir2 & _IR[1] & _IR[0]);
    _load2s  = ~(load2q &  _loadimm);
    _load3d  = ~(load2q & ~_loadimm);

    # doing last cycle of instruction, determine whether fetch_1 or irq_1 is next
    instend  = ~(_reset1q & _iret4q & _bcc1q & _arith1q & _store2q & _lda1q & _load2s & _ireq5q & _wrps1q & _rdps1q & _load3q & _halt1q);
    _irqpend = ~(IRQ & PSW_IE);
    _ireq1d  = ~(~_irqpend & instend);  # irq_1 is next
    _fetch1d = ~( _irqpend & instend);  # fetch_1 is next

    # the 'microcode ROM' - determine control outputs based on current state
    ALU_ADD     = ~(_fetch1q & _fetch2q_a & _bcc1q & _store1q & _lda1q & _load1q & _ireq4q & _load3q);
    ALU_BONLY   = ~(ALU_ADD | arith1q);
    ALU_IRFC    = arith1q;

    BMUX_CON_0  = ~(_reset1q & _fetch1q & _ireq4q);
    BMUX_CON_2  = ~(_fetch2q_a & _ireq1q & _ireq5q & _iret3q & _load3q);
    BMUX_CON__4 = ~(_iret1q & _iret3q & _ireq1q & _ireq3q);
    BMUX_IRBROF = ~_bcc1q;
    BMUX_IRLSOF = ~(_store1q & _lda1q & _load1q);

    GPRS_REA7   = ~(_fetch1q & _fetch2q_a & _bcc1q & _ireq4q & _load3q);
    GPRS_REA    = ~(_arith1q & _store1q & _lda1q & _load1q);
    GPRS_REB    = ~(_arith1q & _wrps1q & _halt1q);
    GPRS_RED2B  = store2q;
    GPRS_WE7    = ~(_reset1q & _fetch2q_a & _ireq5q & _iret2q & _load3q & ~(bcc1q & PSW_BT));
    GPRS_WED    = ~(_arithnotpc & _lda1q & _load2q & _rdps1q);

    HALT        = halt1q;

    _IR_WE      = ~fetch2q;

    MEM_READ    = ~(_fetch1q & _iret1q & _iret3q & _load1q);
    MEM_REB     = ~(_load2q & _iret2q & _iret4q);
    MEM_WORD    = ~(~(_load1q & _load2q & _store1q) & IR[13]);
    MEM_WRITE   = ~(_ireq1q & _ireq3q & _store1q);

    PSW_ALUWE   = ~_arith1q;    # '~_' for fanout
    _PSW_IECLR  = ~(reset1q | ireq4q);
    _PSW_REB    = ~(ireq2q | rdps1q);
    PSW_WE      = ~(_reset1q & _iret4q & _wrps1q);
}
