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

// generate misc BBUS values
// combinational logic only
//  input:
//   IR = instruction register
//   CON_0 = output a 0
//   CON_2 = output a 2
//   CON__4 = output a -4
//   IRBROF = output sign-extended branch offset
//   IRLSOF = output sign-extended liad/store offset
//  output:
//   Q = resultant output (0 if nothing selected)

module bmux (IR, CON_0, CON_2, CON__4, IRBROF, IRLSOF, Q);
    input[15:00] IR;
    input CON_0, CON_2, CON__4, IRBROF, IRLSOF;
    output[15:00] Q;

    wire[15:00] irbrof = { {6 {IR[09]}}, IR[09:01], 1'b0 };
    wire[15:00] irlsof = { {9 {IR[06]}}, IR[06:00] };

    assign Q = (CON_2 ? 16'h0002 : 16'h0) | (CON__4 ? 16'hFFFC : 16'h0) | (IRBROF ? irbrof : 16'h0) | (IRLSOF ? irlsof : 16'h0);
endmodule
