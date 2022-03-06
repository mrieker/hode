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
# test physical register circuit board
# ./netgen.sh regtester.mod -sim regpair.sim

# assumes the physical circuit board is jumpered for r6r7

module regpair (
    out ABUS[15:00], out BBUS[15:00], in DBUS[15:00], in CLK2,
    in WED, in WEODD, in REA, in REAODD, in REB, in RED2B,
    in _ir12, in _ir11, in ir10, in _ir10,    # reg D selection
    in  _ir9, in  _ir8, in  ir7, in  _ir7,    # reg A selection
    in  _ir6, in  _ir5, in  ir4, in  _ir4)    # reg B selection
{
    wire IR[15:00], _IR[15:00], WE7, REA7;

     IR[10] =  ir10;
    _IR[10] = _ir10;
     IR[07] =  ir7;
    _IR[07] = _ir7;
     IR[04] =  ir4;
    _IR[04] = _ir4;

    # assume phys pcb jumpered for r6r7
    WE7   = WEODD;
    REA7  = REAODD;
    _IR[05] =   _ir5;
    _IR[06] =   _ir6;
    _IR[08] =   _ir8;
    _IR[09] =   _ir9;
    _IR[11] =   _ir11;
    _IR[12] =   _ir12;
     IR[05] = ~ _ir5;
     IR[06] = ~ _ir6;
     IR[08] = ~ _ir8;
     IR[09] = ~ _ir9;
     IR[11] = ~ _ir11;
     IR[12] = ~ _ir12;

    # use 1s for don't care bits to go easy on io-warrior-56 chips
    _IR[00] = 1; _IR[01] = 1; _IR[02] = 1; _IR[03] = 1; _IR[13] = 1; _IR[14] = 1; _IR[15] = 1;
     IR[00] = 1;  IR[01] = 1;  IR[02] = 1;  IR[03] = 1;  IR[13] = 1;  IR[14] = 1;  IR[15] = 1;

    aiow: IOW56 id_a
                       oo oo go oo go oo go oo go oo
                       oo go oo go oo go oo go oo oo (
                            ABUS[00], BBUS[00], ABUS[08], BBUS[08],
                            ABUS[01], BBUS[01], ABUS[09], BBUS[09],
                            ABUS[02], BBUS[02], ABUS[10], BBUS[10],
                            ABUS[03], BBUS[03], ABUS[11], BBUS[11],
                            ABUS[04], BBUS[04], ABUS[12], BBUS[12],
                            ABUS[05], BBUS[05], ABUS[13], BBUS[13],
                            ABUS[06], BBUS[06], ABUS[14], BBUS[14],
                            ABUS[07], BBUS[07], ABUS[15], BBUS[15]);

    iiow: IOW56 id_i
                       ii ii gi ii gi ii gi ii gi ii
                       ii gi ii gi ii gi ii gi ii ii (
                            IR[00], _IR[00],
                            IR[01], _IR[01],
                            IR[02], _IR[02],
                            IR[03], _IR[03],
                            IR[04], _IR[04],
                            IR[05], _IR[05],
                            IR[06], _IR[06],
                            IR[07], _IR[07],
                            IR[08], _IR[08],
                            IR[09], _IR[09],
                            IR[10], _IR[10],
                            IR[11], _IR[11],
                            IR[12], _IR[12],
                            IR[13], _IR[13],
                            IR[14], _IR[14],
                            IR[15], _IR[15]);

    diow: IOW56 id_d
                      ii ii gi ii gi ii gi ii gi ii
                      ii gi ii gi ii gi ii gi ii ii (
                            DBUS[15], DBUS[07],
                            DBUS[14], DBUS[06],
                            DBUS[13], DBUS[05],
                            DBUS[12], DBUS[04],
                            DBUS[11], DBUS[03],
                            DBUS[10], DBUS[02],
                            DBUS[09], DBUS[01],
                            DBUS[08], DBUS[00],
                            1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1);

    ciow: IOW56 id_c
                ii ii gi ii gi ii gi ii gi ii
                ii gi ii gi ii gi ii gi ii ii (
                                     I1:1,
                                     I2:1,
                                     I3:1,
                                     I4:1,
                                     I6:CLK2,
                                     I7:1,
                                     I8:1,
                                    I10:1,
                                    I11:1,
                                    I12:REB,
                                    I14:1,
                                    I15:WED,
                                    I16:1,
                                    I18:1,
                                    I19:1,
                                    I20:REA,
                                    I21:1,
                                    I22:RED2B,
                                    I24:1,
                                    I25:1,
                                    I26:1,
                                    I28:1,
                                    I29:1,
                                    I30:1,
                                    I32:WE7,
                                    I33:REA7,
                                    I34:1,
                                    I36:1,
                                    I37:1,
                                    I38:1,
                                    I39:1,
                                    I40:1);
}
