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

// top-level module that interfaces wirh raspi
// inverts signals cuz raspictl program assumes that
// also switches mbus on command by raspi

module de0 (_CLOCK, _DENA, _INTRQ, _RESET, _HALT, _MREAD, _MWORD, _MWRITE, _MBUS, LEDS, STATES);
    input _CLOCK, _DENA, _INTRQ, _RESET;
    output _HALT, _MREAD, _MWORD, _MWRITE;
    inout[15:00] _MBUS;
    output[7:0] LEDS;
    output[23:00] STATES;

    wire halt, mread, mword, mwrite;
    wire[15:00] md;

    assign _HALT   = ~ halt;
    assign _MREAD  = ~ mread;
    assign _MWORD  = ~ mword;
    assign _MWRITE = ~ mwrite;
    assign _MBUS   = _DENA ? 16'hZZZZ : ~ md;
    assign LEDS    = md[7:0];

    sys sys (.CLOCK (~ _CLOCK), .INTRQ (~ _INTRQ), .RESET (~ _RESET), .MQ (~ (_MBUS & {16 {_DENA}})),
            .HALT (halt), .MREAD (mread), .MWORD (mword), .MWRITE (mwrite), .MD (md), .STATES (STATES));
endmodule
