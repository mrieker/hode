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

# single register
# 16-bit DFF with dual outputs
#  input:
#   dbus = output from ALU
#   _wt = write register flipflops at end of cycle
#         change only during second half of cycle
#   _clk1 = clock cycle
#   ir12,ir11,ir10 = register A select
#   ir9,ir8,ir7 = register D select
#   ir6,ir5,ir4 = register B select
#   REA = gate onto abus if A select all ones
#   REB = gate onto bbus if B select all ones
#   RED2B = gate onto bbus if D select all ones
#   REAODD = gate onto abus (ignore selects)
#  output:
#   abus = A bus to ALU
#   bbus = B bus to ALU
module register (out abus[15:00], out bbus[15:00], in dbus[15:00], in _wt, in _clk1,
        in ir12, in ir11, in ir10, in ir9, in ir8, in ir7, in ir6, in ir5, in ir4,
        in REA, in REB, in RED2B, in REAODD)
{
    wire clk0_a, clk0_b, clk0_c, clk0_d;
    wire _rea, rea_a, rea_b;
    wire _reb, reb_a, reb_b;
    wire q[15:00], _q[15:00];

    clk0_a = ~(_clk1 | _wt);
    bit15: DFF miniflat (Q:q[15], _Q:_q[15], _PC:1, _PS:1, T:clk0_a, D:dbus[15]);
    bit07: DFF miniflat (Q:q[07], _Q:_q[07], _PC:1, _PS:1, T:clk0_a, D:dbus[07]);
    bit14: DFF miniflat (Q:q[14], _Q:_q[14], _PC:1, _PS:1, T:clk0_a, D:dbus[14]);
    bit06: DFF miniflat (Q:q[06], _Q:_q[06], _PC:1, _PS:1, T:clk0_a, D:dbus[06]);

    clk0_b = ~(_clk1 | _wt);
    bit13: DFF miniflat (Q:q[13], _Q:_q[13], _PC:1, _PS:1, T:clk0_b, D:dbus[13]);
    bit05: DFF miniflat (Q:q[05], _Q:_q[05], _PC:1, _PS:1, T:clk0_b, D:dbus[05]);
    bit12: DFF miniflat (Q:q[12], _Q:_q[12], _PC:1, _PS:1, T:clk0_b, D:dbus[12]);
    bit04: DFF miniflat (Q:q[04], _Q:_q[04], _PC:1, _PS:1, T:clk0_b, D:dbus[04]);

    clk0_c = ~(_clk1 | _wt);
    bit11: DFF miniflat (Q:q[11], _Q:_q[11], _PC:1, _PS:1, T:clk0_c, D:dbus[11]);
    bit03: DFF miniflat (Q:q[03], _Q:_q[03], _PC:1, _PS:1, T:clk0_c, D:dbus[03]);
    bit10: DFF miniflat (Q:q[10], _Q:_q[10], _PC:1, _PS:1, T:clk0_c, D:dbus[10]);
    bit02: DFF miniflat (Q:q[02], _Q:_q[02], _PC:1, _PS:1, T:clk0_c, D:dbus[02]);

    clk0_d = ~(_clk1 | _wt);
    bit09: DFF miniflat (Q:q[09], _Q:_q[09], _PC:1, _PS:1, T:clk0_d, D:dbus[09]);
    bit01: DFF miniflat (Q:q[01], _Q:_q[01], _PC:1, _PS:1, T:clk0_d, D:dbus[01]);
    bit08: DFF miniflat (Q:q[08], _Q:_q[08], _PC:1, _PS:1, T:clk0_d, D:dbus[08]);
    bit00: DFF miniflat (Q:q[00], _Q:_q[00], _PC:1, _PS:1, T:clk0_d, D:dbus[00]);

    _rea  = ~(ir12 & ir11 & ir10 & REA | REAODD);
    _reb  = ~(ir6 & ir5 & ir4 & REB | ir9 & ir8 & ir7 & RED2B);

    abus[15] = oc ~(_q[15] & rea_a);
    abus[07] = oc ~(_q[07] & rea_a);
    abus[14] = oc ~(_q[14] & rea_a);
    abus[06] = oc ~(_q[06] & rea_a);
    abus[13] = oc ~(_q[13] & rea_a);
    abus[05] = oc ~(_q[05] & rea_a);
    abus[12] = oc ~(_q[12] & rea_a);
    abus[04] = oc ~(_q[04] & rea_a);
    rea_a = ~_rea;

    bbus[15] = oc ~(_q[15] & reb_a);
    bbus[07] = oc ~(_q[07] & reb_a);
    bbus[14] = oc ~(_q[14] & reb_a);
    bbus[06] = oc ~(_q[06] & reb_a);
    bbus[13] = oc ~(_q[13] & reb_a);
    bbus[05] = oc ~(_q[05] & reb_a);
    bbus[12] = oc ~(_q[12] & reb_a);
    bbus[04] = oc ~(_q[04] & reb_a);
    reb_a = ~_reb;

    rea_b = ~_rea;
    abus[11] = oc ~(_q[11] & rea_b);
    abus[03] = oc ~(_q[03] & rea_b);
    abus[10] = oc ~(_q[10] & rea_b);
    abus[02] = oc ~(_q[02] & rea_b);
    abus[09] = oc ~(_q[09] & rea_b);
    abus[01] = oc ~(_q[01] & rea_b);
    abus[08] = oc ~(_q[08] & rea_b);
    abus[00] = oc ~(_q[00] & rea_b);

    reb_b = ~_reb;
    bbus[11] = oc ~(_q[11] & reb_b);
    bbus[03] = oc ~(_q[03] & reb_b);
    bbus[10] = oc ~(_q[10] & reb_b);
    bbus[02] = oc ~(_q[02] & reb_b);
    bbus[09] = oc ~(_q[09] & reb_b);
    bbus[01] = oc ~(_q[01] & reb_b);
    bbus[08] = oc ~(_q[08] & reb_b);
    bbus[00] = oc ~(_q[00] & reb_b);
}

# an even/odd pair of 16-bit register latches
#  input:
#   DBUS = data to write to registers
#   CLK2 = system clock
#   WED = write register selected by IR[12:10]
#   WEODD = write odd register
#   REA = gate register selected by IR[09:07] onto ABUS
#   REAODD = gate odd register onto ABUS
#   REB = gate register selected by IR[06:04] onto BBUS
#   RED2B = gate register selected by IR[12:10] onto BBUS
#  output:
#   ABUS = register selected by IR[09:07] when enabled by REA
#          or odd register when enabled by REAODD
#   BBUS = register selected by IR[06:04] when enabled by REB
#          or register selected by IR[12:10] when enabled by RED2B
module regpair (
    out ABUS[15:00], out BBUS[15:00], in DBUS[15:00], in CLK2,
    in WED, in WEODD, in REA, in REAODD, in REB, in RED2B,
    in _ir12, in _ir11, in ir10, in _ir10,    # reg A selection
    in  _ir9, in  _ir8, in  ir7, in  _ir7,    # reg D selection
    in  _ir6, in  _ir5, in  ir4, in  _ir4)    # reg B selection
{
    wire clk0, _clk1, _clk_1, _wdevn, _wdodd, _wtevn, _wtodd;
    wire ir12, ir11, ir9, ir8, ir6, ir5, ir7_a, _ir7_a;

    _clk1 = ~CLK2;
    clk0 = ~_clk1;
    _clk_1 = ~clk0;

    ir12 = ~_ir12;
    ir11 = ~_ir11;
    ir9  = ~_ir9;
    ir8  = ~_ir8;
    ir6  = ~_ir6;
    ir5  = ~_ir5;

    ir7_a = ~_ir7;
    _ir7_a = ~ir7;

    # decide if register should be written at end of this cycle
    _wdevn = ~(WED & ir9 & ir8 & _ir7_a);
    _wdodd = ~(WED & ir9 & ir8 &  ir7_a | WEODD);

    # latch that decision a little bit after the clock to the registers drops
    # ...so the clk0_{a,b,c,d} gates won't glitch
    wevn: DFF (Q:_wtevn, _PC:1, _PS:1, T:_clk_1, D:_wdevn);
    wodd: DFF (Q:_wtodd, _PC:1, _PS:1, T:_clk_1, D:_wdodd);

    revn: register (ABUS, BBUS, DBUS, _wtevn, _clk1,
        ir12, ir11, _ir10, ir9, ir8, _ir7_a, ir6, ir5, _ir4,
        REA, REB, RED2B, 0);

    rodd: register (ABUS, BBUS, DBUS, _wtodd, _clk1,
        ir12, ir11,  ir10, ir9, ir8,  ir7_a, ir6, ir5,  ir4,
        REA, REB, RED2B, REAODD);
}

# 8 word-wide registers (testing only)
# dual open-collector outputs
# select registers via ir fields
# or use overrides for R7
module registers (out ABUS[15:00], out BBUS[15:00],
    in DBUS[15:00], in IR[15:00], in _IR[15:00],
    in REA7, in REA, in REB, in RED2B,
    in WE7, in WED, in CLK2)
{
    r0r1: regpair (ABUS, BBUS, DBUS, CLK2, WED,   0, REA,    0, REB, RED2B,  IR[12],  IR[11], IR[10], _IR[10],  IR[9],  IR[8], IR[7], _IR[7],  IR[6],  IR[5], IR[4], _IR[4]);
    r2r3: regpair (ABUS, BBUS, DBUS, CLK2, WED,   0, REA,    0, REB, RED2B,  IR[12], _IR[11], IR[10], _IR[10],  IR[9], _IR[8], IR[7], _IR[7],  IR[6], _IR[5], IR[4], _IR[4]);
    r4r5: regpair (ABUS, BBUS, DBUS, CLK2, WED,   0, REA,    0, REB, RED2B, _IR[12],  IR[11], IR[10], _IR[10], _IR[9],  IR[8], IR[7], _IR[7], _IR[6],  IR[5], IR[4], _IR[4]);
    r6r7: regpair (ABUS, BBUS, DBUS, CLK2, WED, WE7, REA, REA7, REB, RED2B, _IR[12], _IR[11], IR[10], _IR[10], _IR[9], _IR[8], IR[7], _IR[7], _IR[6], _IR[5], IR[4], _IR[4]);
}
