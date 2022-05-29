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

// gate memory read data onto BBUS
//  input:
//   D = data from memory
//   WORD = data is word sized; else byte sized
//   RE = enable gating onto output; else output 0
//   SEXT = sign-extend byte; else zero-extend byte; ignored if WORD mode
//  output:
//   Q = resultant output

module memrd (D, WORD, RE, SEXT, Q);
    input[15:00] D;
    input WORD, RE, SEXT;

    output[15:00] Q;

    assign Q[07:00] = RE ? D[07:00] : 8'b0;
    assign Q[15:08] = RE & WORD ? D[15:08] : RE & SEXT ? {8 {D[07]}} : 8'b0;
endmodule
