//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

// sequencer - contains state flipflops, instruction decoder, 'microcode'

module seq (
        ALU_ADD,
        ALU_IRFC,
        ALU_BONLY,
        BMUX_CON_0,
        BMUX_CON_2,
        BMUX_CON__4,
        BMUX_IRBROF,
        BMUX_IRLSOF,
        GPRS_WE7,
        GPRS_REA7,
        GPRS_WED,
        GPRS_REA,
        GPRS_REB,
        GPRS_RED2B,
        IR_WE,
        MEM_READ,
        MEM_WORD,
        MEM_WRITE,
        MEM_REB,
        PSW_IECLR,
        PSW_ALUWE,
        PSW_WE,
        PSW_REB,
        HALT,

        CLOCK,
        RESET,
        IR,
        INTRQ,
        PSWIE,
        PSWBT,

        STATES);

    input CLOCK;
    input RESET;
    input[15:00] IR;
    input INTRQ;
    input PSWIE;
    input PSWBT;
    output[23:00] STATES;

    reg reset0q, reset1q;
    reg ireq1q,  ireq2q,  ireq3q,  ireq4q,  ireq5q;
    reg fetch1q, fetch2q, arith1q, bcc1q,   lda1q;
    reg load1q,  load2q,  load3q,  store1q, store2q;
    reg halt1q,  wrps1q,  rdps1q;
    reg iret1q,  iret2q,  iret3q,  iret4q;

    assign STATES[ 0] = reset0q;
    assign STATES[ 1] = reset1q;
    assign STATES[ 2] = ireq1q;
    assign STATES[ 3] = ireq2q;
    assign STATES[ 4] = ireq3q;
    assign STATES[ 5] = ireq4q;
    assign STATES[ 6] = ireq5q;
    assign STATES[ 7] = fetch1q;
    assign STATES[ 8] = fetch2q;
    assign STATES[ 9] = arith1q;
    assign STATES[10] = bcc1q;
    assign STATES[11] = lda1q;
    assign STATES[12] = load1q;
    assign STATES[13] = load2q;
    assign STATES[14] = load3q;
    assign STATES[15] = store1q;
    assign STATES[16] = store2q;
    assign STATES[17] = halt1q;
    assign STATES[18] = wrps1q;
    assign STATES[19] = rdps1q;
    assign STATES[20] = iret1q;
    assign STATES[21] = iret2q;
    assign STATES[22] = iret3q;
    assign STATES[23] = iret4q;

    // doing arithmetic instruction with rd != pc
    wire rdnotpc     = ~(IR[7] & IR[8] & IR[9]);
    wire _arithnotpc = ~(arith1q & rdnotpc);

    // we are currently doing fetch_2 cycle, so use the incoming instruction to determine which instruction sequence to start
    wire topthreezero = ~(IR[15] | IR[14] | IR[13]);
    wire specialinstr = ~(IR[12] | IR[11] | IR[10] | IR[0]);

    wire _halt1d   = ~(fetch2q & topthreezero & specialinstr & ~IR[2] & ~IR[1]);           // halt instruction  (opcode 000000.......000)
    wire _iret1d   = ~(fetch2q & topthreezero & specialinstr & ~IR[2] &  IR[1]);           // iret instruction  (opcode 000000.......010)
    wire _wrps1d   = ~(fetch2q & topthreezero & specialinstr &  IR[2] & ~IR[1]);           // wrps instruction  (opcode 000000.......100)
    wire _rdps1d   = ~(fetch2q & topthreezero & specialinstr &  IR[2] &  IR[1]);           // rdps instruction  (opcode 000000.......110)
    wire _bcc1d    = ~(fetch2q & topthreezero & ~specialinstr);                            // bcc instruction   (opcode 000___........._ where ___ _ != 000 0)
    wire _arith1d  = ~(fetch2q & ~IR[15] & ~IR[14] & IR[13]);                              // arith instruction (opcode 001.............)
    wire _store1d  = ~(fetch2q & ~IR[15] & IR[14]);                                        // store instruction (opcode 01..............)
    wire _lda1d    = ~(fetch2q & IR[15] & ~IR[14] & ~IR[13]);                              // LDA instruction   (opcode 100.............)
    wire _load1d   = ~(fetch2q & IR[15] & IR[14] | fetch2q & IR[15] & ~IR[14] & IR[13]);   // load instruction  (opcode 11.............. and 101.............)

    // extend load to increment PC after a LDx rd,0(PC) where rd != pc
    wire _loadimm = ~(IR[12] & IR[11] & IR[10] & rdnotpc & ~IR[6] & ~IR[5] & ~IR[4] & ~IR[3] & ~IR[2] & ~IR[1] & ~IR[0]);
    wire _load2s  = ~(load2q &  _loadimm);
    wire _load3d  = ~(load2q & ~_loadimm);

    // doing last cycle of instruction, determine whether fetch~1 or irq~1 is next
    wire instend  = ~(~reset1q & ~iret4q & ~bcc1q & ~arith1q & ~store2q & ~lda1q & _load2s & ~ireq5q & ~wrps1q & ~rdps1q & ~load3q & ~halt1q);
    wire _irqpend = ~(INTRQ & PSWIE);
    wire _ireq1d  = ~(~_irqpend & instend);  // irq_1 is next
    wire _fetch1d = ~( _irqpend & instend);  // fetch_1 is next

    // the 'microcode ROM' - determine control outputs based on current state
    output ALU_ADD,     ALU_BONLY,   ALU_IRFC;
    output BMUX_CON_0,  BMUX_CON_2,  BMUX_CON__4, BMUX_IRBROF, BMUX_IRLSOF;
    output GPRS_REA7,   GPRS_REA,    GPRS_REB,    GPRS_RED2B,  GPRS_WE7,    GPRS_WED;
    output HALT,        IR_WE;
    output MEM_READ,    MEM_REB,     MEM_WORD,    MEM_WRITE;
    output PSW_ALUWE,   PSW_IECLR,   PSW_REB,     PSW_WE;

    assign ALU_ADD     = ~(~fetch1q & ~fetch2q & ~bcc1q & ~store1q & ~lda1q & ~load1q & ~ireq4q & ~load3q);
    assign ALU_BONLY   = ~(ALU_ADD | arith1q);
    assign ALU_IRFC    = arith1q;

    assign BMUX_CON_0  = ~(~reset1q & ~fetch1q & ~ireq4q);
    assign BMUX_CON_2  = ~(~fetch2q & ~ireq1q & ~ireq5q & ~iret3q & ~load3q);
    assign BMUX_CON__4 = ~(~iret1q & ~iret3q & ~ireq1q & ~ireq3q);
    assign BMUX_IRBROF = bcc1q;
    assign BMUX_IRLSOF = ~(~store1q & ~lda1q & ~load1q);

    assign GPRS_REA7   = ~(~fetch1q & ~fetch2q & ~bcc1q & ~ireq4q & ~load3q);
    assign GPRS_REA    = ~(~arith1q & ~store1q & ~lda1q & ~load1q);
    assign GPRS_REB    = ~(~arith1q & ~wrps1q & ~halt1q);
    assign GPRS_RED2B  = store2q;
    assign GPRS_WE7    = ~(~reset1q & ~fetch2q & ~ireq5q & ~iret2q & ~load3q & ~(bcc1q & PSWBT));
    assign GPRS_WED    = ~(_arithnotpc & ~lda1q & ~load2q & ~rdps1q);

    assign HALT        = halt1q;

    assign IR_WE       = fetch2q;

    assign MEM_READ    = ~(~fetch1q & ~iret1q & ~iret3q & ~load1q);
    assign MEM_REB     = ~(~load2q & ~iret2q & ~iret4q);
    assign MEM_WORD    = ~(~(~load1q & ~load2q & ~store1q) & IR[13]);
    assign MEM_WRITE   = ~(~ireq1q & ~ireq3q & ~store1q);

    assign PSW_ALUWE   = arith1q;
    assign PSW_IECLR   = (reset1q | ireq4q);
    assign PSW_REB     = (ireq2q | rdps1q);
    assign PSW_WE      = ~(~reset1q & ~iret4q & ~wrps1q);

    // state flip-flops
    always @ (posedge RESET or posedge CLOCK) begin
        if (RESET) begin
            reset0q <= 1;
            reset1q <= 0;
            ireq1q  <= 0;
            ireq2q  <= 0;
            ireq3q  <= 0;
            ireq4q  <= 0;
            ireq5q  <= 0;
            fetch1q <= 0;
            fetch2q <= 0;
            arith1q <= 0;
            bcc1q   <= 0;
            lda1q   <= 0;
            load1q  <= 0;
            load2q  <= 0;
            load3q  <= 0;
            store1q <= 0;
            store2q <= 0;
            halt1q  <= 0;
            iret1q  <= 0;
            iret2q  <= 0;
            iret3q  <= 0;
            iret4q  <= 0;
            wrps1q  <= 0;
            rdps1q  <= 0;
        end else begin
            reset0q <= 0;
            reset1q <=   reset0q;
            ireq1q  <= ~_ireq1d;
            ireq2q  <=   ireq1q;
            ireq3q  <=   ireq2q;
            ireq4q  <=   ireq3q;
            ireq5q  <=   ireq4q;
            fetch1q <= ~_fetch1d;
            fetch2q <=   fetch1q;
            arith1q <= ~_arith1d;
            bcc1q   <= ~_bcc1d;
            lda1q   <= ~_lda1d;
            load1q  <= ~_load1d;
            load2q  <=   load1q;
            load3q  <= ~_load3d;
            store1q <= ~_store1d;
            store2q <=   store1q;
            halt1q  <= ~_halt1d;
            iret1q  <= ~_iret1d;
            iret2q  <=   iret1q;
            iret3q  <=   iret2q;
            iret4q  <=   iret3q;
            wrps1q  <= ~_wrps1d;
            rdps1q  <= ~_rdps1d;
        end
    end
endmodule
