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

// general purpose registers (R0..R7)
//  input:
//   D = data input to write to regisyers
//   CLK = write registers on low-to-high transition
//   WED = write register IR[9:7] on next low-to-high clock
//   WE7 = write R7 (PC) on next low-to-high clock
//   REA = gate register IR[12:10] onto QA output
//   REA7 = gate R7 (PC) onto QA output
//   REB = gate register IR[6:4] onto QB output
//   RED2B = gate register IR[9:7] onto QB output
//  output:
//   QA = as selected by REA/REA7 or 0
//   QB = as selected by REB/RED2B or 0

module gprs (D, CLK, WED, WE7, REA, REA7, REB, RED2B, IR, QA, QB);
    input[15:00] D, IR;
    input CLK, WED, WE7, REA, REA7, REB, RED2B;
    output[15:00] QA, QB;

    reg[15:00] r0q, r1q, r2q, r3q, r4q, r5q, r6q, r7q;

    always @ (posedge CLK) begin
        if (WED & ~IR[9] & ~IR[8] & ~IR[7]) r0q <= D;
        if (WED & ~IR[9] & ~IR[8] &  IR[7]) r1q <= D;
        if (WED & ~IR[9] &  IR[8] & ~IR[7]) r2q <= D;
        if (WED & ~IR[9] &  IR[8] &  IR[7]) r3q <= D;
        if (WED &  IR[9] & ~IR[8] & ~IR[7]) r4q <= D;
        if (WED &  IR[9] & ~IR[8] &  IR[7]) r5q <= D;
        if (WED &  IR[9] &  IR[8] & ~IR[7]) r6q <= D;
        if (WED &  IR[9] &  IR[8] &  IR[7] | WE7) r7q <= D;
    end

    assign QA =
            r0q & {16 {REA & ~IR[12] & ~IR[11] & ~IR[10]}} |
            r1q & {16 {REA & ~IR[12] & ~IR[11] &  IR[10]}} |
            r2q & {16 {REA & ~IR[12] &  IR[11] & ~IR[10]}} |
            r3q & {16 {REA & ~IR[12] &  IR[11] &  IR[10]}} |
            r4q & {16 {REA &  IR[12] & ~IR[11] & ~IR[10]}} |
            r5q & {16 {REA &  IR[12] & ~IR[11] &  IR[10]}} |
            r6q & {16 {REA &  IR[12] &  IR[11] & ~IR[10]}} |
            r7q & {16 {REA &  IR[12] &  IR[11] &  IR[10] | REA7}};

    assign QB =
            r0q & {16 {REB & ~IR[6] & ~IR[5] & ~IR[4] | RED2B & ~IR[9] & ~IR[8] & ~IR[7]}} |
            r1q & {16 {REB & ~IR[6] & ~IR[5] &  IR[4] | RED2B & ~IR[9] & ~IR[8] &  IR[7]}} |
            r2q & {16 {REB & ~IR[6] &  IR[5] & ~IR[4] | RED2B & ~IR[9] &  IR[8] & ~IR[7]}} |
            r3q & {16 {REB & ~IR[6] &  IR[5] &  IR[4] | RED2B & ~IR[9] &  IR[8] &  IR[7]}} |
            r4q & {16 {REB &  IR[6] & ~IR[5] & ~IR[4] | RED2B &  IR[9] & ~IR[8] & ~IR[7]}} |
            r5q & {16 {REB &  IR[6] & ~IR[5] &  IR[4] | RED2B &  IR[9] & ~IR[8] &  IR[7]}} |
            r6q & {16 {REB &  IR[6] &  IR[5] & ~IR[4] | RED2B &  IR[9] &  IR[8] & ~IR[7]}} |
            r7q & {16 {REB &  IR[6] &  IR[5] &  IR[4] | RED2B &  IR[9] &  IR[8] &  IR[7]}};

endmodule
