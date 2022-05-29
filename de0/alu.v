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

// 16-bit arithmetic logic unit
// combinational logic only
//  input:
//   A = A operand
//   B = B operand
//   CIN = carry input
//   ADD = perform addition A + B
//   BONLY = forward B to output
//   IRFC = perform function given by IR[3:0]
//   IR = instruction register
//  output:
//   D = result
//   N = result is negative
//   Z = result is zero
//   V = function generating signed overflow
//   C = function generating unsigned overflow

//   0000   0  LSR     d = a >>> 1               NZ0C
//   0001   1  ASR     d = a >>  1               NZ0C
//   0010   2  ROR     d = a >>> 1 | C << 15     NZ0C
//   0011   3
//   0100   4  MOV     d =   b                   NZ0
//   0101   5  NEG     d = - b                   NZV
//   0110   6  INC     d =   b + 1               NZV
//   0111   7  COM     d = ~ b                   NZ0
//   1000   8  OR      d = a | b                 NZ0
//   1001   9  AND     d = a & b                 NZ0
//   1010  10  XOR     d = a ^ b                 NZ0
//   1011  11
//   1100  12  ADD     d = a + b                 NZVC
//   1101  13  SUB     d = a - b                 NZVC
//   1110  14  ADC     d = a + b + c             NZVC
//   1111  15  SBB     d = a - b - c             NZVC

module alu (A, B, CIN, ADD, BONLY, IRFC, IR, D, N, Z, V, C);
    input[15:00] A, B, IR;
    input CIN, ADD, BONLY, IRFC;
    output[15:00] D;
    output N, Z, V, C;

    wire aena     = ADD | IRFC & IR[3];                     // OR,AND,XOR,ADD,SUB,ADC,SUB
    wire bnot     = IRFC & IR[00];                          // NEG,COM,AND,SUB,SBB
    wire doaddsub = ADD | IRFC & IR[2];                     // MOV,NEG,INC,COM,ADD,SUB,ADC,SBB
    wire doingxor = IRFC & (IR[3:0] == 4'b1010);            // XOR
    wire doingand = IRFC & (IR[3:0] == 4'b1001);            // AND
    wire doingor  = BONLY | IRFC & (IR[3:0] == 4'b1000);    // OR
    wire doingshr = IRFC & (IR[3:2] == 2'b00);              // LSR,ASR,ROR

    wire[15:00] andeda  = A & {16 {aena}};
    wire[15:00] nottedb = B ^ {16 {bnot}};
    wire rawcin = (((IR[3:1] == 3'b011) | ((IR[3:1] == 3'b111) & CIN)) & IRFC) ^ bnot;
    wire[15:00] sumlo = { 1'b0, andeda[14:00] } + { 1'b0, nottedb[14:00] } + { 14'b0, rawcin };
    wire[1:0]   sumhi = { 1'b0, andeda[15] } + { 1'b0, nottedb[15] } + { 1'b0, sumlo[15] };
    wire[15:00] sum   = { sumhi[0], sumlo[14:00] };
    wire[15:00] shr   = { (A[15] & IR[0]) | (CIN & IR[1]), A[15:01] };

    assign D = ({16 {doaddsub}} & sum) |
                    ({16 {doingxor}} & (andeda ^ B)) |
                    ({16 {doingand}} & (andeda & B)) |
                    ({16 {doingor }} & (andeda | B)) |
                    ({16 {doingshr}} & shr);

    assign N = D[15];
    assign Z = D[15:00] == 16'b0;
    assign V = doaddsub & (sumlo[15] ^ sumhi[1]);
    assign C = (IR[3:2] == 2'b00) ? A[00] :
               (IR[3:2] == 2'b11) ? sumhi[1] ^ bnot :
               CIN;
endmodule
