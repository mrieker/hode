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
# circuit board what holds two registers

# jumpers at top of board:
#  R0/R1:          R2/R3:          R4/R5:          R6/R7:
#   ** **  **       *- *-  *-       -* -*  -*       -- --  --
#   ** **  **       ** **  **       ** **  **       ** **  **
#   -- --  --       -* -*  -*       *- *-  *-       ** **  **

# jumpers at bottom of board:
#  R6/R7:          other:
#   --              **
#   **              **
#   **              --

module regboard ()
{
    wire ABUS[15:00], BBUS[15:00], DBUS[15:00], IR[15:00], _IR[15:00];
    wire _ir05, _ir06, _ir08, _ir09, _ir11, _ir12;

    wire CLK2, WED, REA, REB, RED2B, WE7, REA7;
    wire weodd, reaodd;

    abbus: Conn
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

    irbus: Conn
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

    irsel: Conn
                       nnn nnn nnn nnn nnn nnn
                                oio     # r0r1/r4r5 <<  *  >> r2r3/r6r7
                                oio     # r0r1/r2r3 <<  *  >> r4r5/r6r7
                       nnn nnn nnn
                                oio     # r0r1/r4r5 <<  *  >> r2r3/r6r7
                                oio     # r0r1/r2r3 <<  *  >> r4r5/r6r7
                       nnn
                                oio     # r0r1/r4r5 <<  *  >> r2r3/r6r7
                                oio     # r0r1/r2r3 <<  *  >> r4r5/r6r7
                       nnn nnn nnn nnn (
                            O19:IR[05], I20:_ir05, O21:_IR[05],
                            O22:IR[06], I23:_ir06, O24:_IR[06],
                            O34:IR[08], I35:_ir08, O36:_IR[08],
                            O37:IR[09], I38:_ir09, O39:_IR[09],
                            O43:IR[11], I44:_ir11, O45:_IR[11],
                            O46:IR[12], I47:_ir12, O48:_IR[12]);

    dbus: Conn
                      ii ii gi ii gi ii gi ii gi ii
                      ii gi ii gi ii gi ii gi ii ii (
                            DBUS[15], DBUS[07],
                            DBUS[14], DBUS[06],
                            DBUS[13], DBUS[05],
                            DBUS[12], DBUS[04],
                            DBUS[11], DBUS[03],
                            DBUS[10], DBUS[02],
                            DBUS[09], DBUS[01],
                            DBUS[08], DBUS[00]);

    ctla: Conn ii ii gi ii gi ii gi ii gi ii ii gi ii gi ii gi ii gi ii ii (
                                     I6:CLK2,
                                    I32:WE7,
                                    I33:REA7,
                                    I15:WED,
                                    I20:REA,
                                    I12:REB,
                                    I22:RED2B);

    # jump oi for r6r7 board, jump ig for others
    ctsa: Conn
                        nnn nnn nnn nnn nnn nnn nnn nnn nnn nnn
                        nnn nnn nnn nnn nnn oig oig nnn nnn nnn (
                            O46:WE7,  I47:weodd,
                            O49:REA7, I50:reaodd);

    power: Conn pwrcap v v v g g g ();

    regpair: regpair (
            ABUS, BBUS, DBUS, CLK2,
            WED, weodd, REA, reaodd, REB, RED2B,
            _ir12, _ir11, IR[10], _IR[10],    # reg A selection
            _ir09, _ir08, IR[07], _IR[07],    # reg D selection
            _ir06, _ir05, IR[04], _IR[04]);   # reg B selection
}
