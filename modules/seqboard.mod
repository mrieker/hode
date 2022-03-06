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

module seqboard ()
{
    wire IR[15:00], _IR[15:00];
    wire CLK2, RESET, IRQ, PSW_IE, PSW_BT;
    wire ALU_ADD, ALU_IRFC, ALU_BONLY;
    wire BMUX_CON_0, BMUX_CON_2, BMUX_CON__4, BMUX_IRBROF, BMUX_IRLSOF;
    wire GPRS_WE7, GPRS_REA7, GPRS_WED, GPRS_REA;
    wire GPRS_REB, GPRS_RED2B, _IR_WE;
    wire MEM_READ, MEM_WORD, MEM_WRITE, MEM_REB;
    wire _PSW_IECLR, PSW_ALUWE, PSW_WE, _PSW_REB, HALT;

    # 1st row: ABUS and BBUS
    abbus: Conn ii ii gi ii gi ii gi ii gi ii
                ii gi ii gi ii gi ii gi ii ii ();

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

    dbus: Conn
                      ii ii gi ii gi ii gi ii gi ii
                      ii gi ii gi ii gi ii gi ii ii ();

    ctla: Conn
                      ii ii gi io go oo go oo go oo
                      io go oo go oo go oo go io oo (
                             I1: IRQ,
                             I2: PSW_IE,
                             I3: PSW_BT,
                             I4: RESET,
                             I6: CLK2,
                             O8: MEM_REB,
                            O10: PSW_ALUWE,
                            O11: ALU_IRFC,
                            O12: GPRS_REB,
                            O14: BMUX_IRBROF,
                            O15: GPRS_WED,
                            O16: MEM_READ,
                            O18: PSW_WE,
                            O19: HALT,
                            O20: GPRS_REA,
                            O22: GPRS_RED2B,
                            O24: ALU_BONLY,
                            O25: BMUX_IRLSOF,
                            O26: _PSW_REB,
                            O28: BMUX_CON__4,
                            O29: MEM_WORD,
                            O30: ALU_ADD,
                            O32: GPRS_WE7,
                            O33: GPRS_REA7,
                            O34: BMUX_CON_2,
                            O36: _IR_WE,
                            O38: BMUX_CON_0,
                            O39: MEM_WRITE,
                            O40: _PSW_IECLR);

    power: Conn pwrcap v v v g g g ();

    seq: sequencer (
                 ALU_ADD:     ALU_ADD,
                 ALU_IRFC:    ALU_IRFC,
                 ALU_BONLY:   ALU_BONLY,
                 BMUX_CON_0:  BMUX_CON_0,
                 BMUX_CON_2:  BMUX_CON_2,
                 BMUX_CON__4: BMUX_CON__4,
                 BMUX_IRBROF: BMUX_IRBROF,
                 BMUX_IRLSOF: BMUX_IRLSOF,
                 GPRS_WE7:    GPRS_WE7,
                 GPRS_REA7:   GPRS_REA7,
                 GPRS_WED:    GPRS_WED,
                 GPRS_REA:    GPRS_REA,
                 GPRS_REB:    GPRS_REB,
                 GPRS_RED2B:  GPRS_RED2B,
                 _IR_WE:      _IR_WE,
                 MEM_READ:    MEM_READ,
                 MEM_WORD:    MEM_WORD,
                 MEM_WRITE:   MEM_WRITE,
                 MEM_REB:     MEM_REB,
                 _PSW_IECLR:  _PSW_IECLR,
                 PSW_ALUWE:   PSW_ALUWE,
                 PSW_WE:      PSW_WE,
                 _PSW_REB:    _PSW_REB,
                 HALT:        HALT,

                 CLK2:        CLK2,
                 RESET:       RESET,
                 IR:          IR,
                 _IR:         _IR,
                 IRQ:         IRQ,
                 PSW_IE:      PSW_IE,
                 PSW_BT:      PSW_BT);
}
