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

// connect the various modules together to form processor
//  input:
//   CLOCK = state transitions happen on low-to-high transitions
//   RESET = asynchronous processor reset
//   INTRQ = interrupt request
//   MQ = data being read from memory
//  output:
//   HALT = processor is executing an halt instruction
//          asserted just after low-to-high low-to-high transition
//          clock should then be held low until ready to resume processing if ever
//          negated just after next clock low-to-high transition
//   MREAD = processor is requesting read of memory location
//           asserted just after clock low-to-high transition, ie, when address is being sent out on MD
//           negated just after next clock low-to-high transition, ie, when data will be transferred in on MQ
//           memory address will appear on MD by end of this cycle
//           data must be valid on MQ by end of next cycle
//   MWORD = MREAD/MWRITE is word-sized; else byte-sized
//   MWRITE = processor is requesting write of memory location
//            asserted just after clock low-to-high transition, ie, when address is being sent out on MD
//            negated just after next clock low-to-high transition, ie, when data will be sent out on MD
//            memory address will appear on MD by end of this cycle
//            memory data will appear on MD by end of next cycle
//   MD = memory address or data being written to memory

module sys (CLOCK, RESET, INTRQ, MQ, HALT, MREAD, MWORD, MWRITE, MD, STATES);
    input CLOCK, RESET, INTRQ;
    input[15:00] MQ;
    output HALT, MREAD, MWORD, MWRITE;
    output[15:00] MD;
    output[23:00] STATES;

    reg[15:00] irlat;
    wire[15:00] alud, bmuxq, gprsa, gprsb, memrdq, pswq;
    wire aluadd, alubonly, aluc, aluirfc, alun, aluv, aluz;
    wire bmuxcon0, bmuxcon2, bmuxcon_4, bmuxirbrof, bmuxirlsof;
    wire gprsrea7, gprsrea, gprsreb, gprsred2b, gprswe7, gprswed;
    wire irwe, memreb, memword, pswaluwe, pswbt, pswc, pswie, pswieclr, pswreb, pswwe;

    assign MD = alud;
    assign MWORD = memword;

    wire[15:00] alub = bmuxq | gprsb | memrdq | pswq;

    alu alu (.A (gprsa), .B (alub), .CIN (pswc), .ADD (aluadd), .BONLY (alubonly), .IRFC (aluirfc), .IR (irlat),
            .D (alud), .N (alun), .Z (aluz), .V (aluv), .C (aluc));

    bmux bmux (.IR (irlat), .CON_0 (bmuxcon0), .CON_2 (bmuxcon2), .CON__4 (bmuxcon_4), .IRBROF (bmuxirbrof), .IRLSOF (bmuxirlsof), .Q (bmuxq));

    gprs gprs (.D (alud), .CLK (CLOCK), .WED (gprswed), .WE7 (gprswe7), .REA (gprsrea), .REA7 (gprsrea7), .REB (gprsreb), .RED2B (gprsred2b), .IR (irlat),
            .QA (gprsa), .QB (gprsb));

    always @ (*) begin
        if (irwe) begin
            irlat = MQ;
        end
    end

    memrd memrd (.D (MQ), .WORD (memword), .RE (memreb), .SEXT (irlat[14]), .Q (memrdq));

    psw psw (.D (alud), .CLOCK (CLOCK), .WE (pswwe), .IECLR (pswieclr), .ALUWE (pswaluwe), .ALUN (alun), .ALUZ (aluz), .ALUV (aluv), .ALUC (aluc),
            .RE (pswreb), .IR (irlat), .Q (pswq), .IE (pswie), .C (pswc), .BT (pswbt));

    seq seq (.IR (irlat), .CLOCK (CLOCK), .RESET (RESET), .INTRQ (INTRQ), .PSWIE (pswie), .PSWBT (pswbt),
            .ALU_ADD (aluadd), .ALU_BONLY (alubonly), .ALU_IRFC (aluirfc), .BMUX_CON_0 (bmuxcon0), .BMUX_CON_2 (bmuxcon2), .BMUX_CON__4 (bmuxcon_4),
            .BMUX_IRBROF (bmuxirbrof), .BMUX_IRLSOF (bmuxirlsof), .GPRS_REA7 (gprsrea7), .GPRS_REA (gprsrea), .GPRS_REB (gprsreb), .GPRS_RED2B (gprsred2b),
            .GPRS_WE7 (gprswe7), .GPRS_WED (gprswed), .HALT (HALT), .IR_WE (irwe), .MEM_READ (MREAD), .MEM_REB (memreb), .MEM_WORD (memword),
            .MEM_WRITE (MWRITE), .PSW_ALUWE (pswaluwe), .PSW_IECLR (pswieclr), .PSW_REB (pswreb), .PSW_WE (pswwe), .STATES (STATES));

endmodule
