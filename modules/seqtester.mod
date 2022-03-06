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
# test physical seq board
#  raspi$ sudo ./netgen.sh seqtester.mod -sim seqtester.sim

module sequencer (
        out ALU_ADD,
        out ALU_IRFC,
        out ALU_BONLY,
        out BMUX_CON_0,
        out BMUX_CON_2,
        out BMUX_CON__4,
        out BMUX_IRBROF,
        out BMUX_IRLSOF,
        out GPRS_WE7,
        out GPRS_REA7,
        out GPRS_WED,
        out GPRS_REA,
        out GPRS_REB,
        out GPRS_RED2B,
        out _IR_WE,
        out MEM_READ,
        out MEM_WORD,
        out MEM_WRITE,
        out MEM_REB,
        out _PSW_IECLR,
        out PSW_ALUWE,
        out PSW_WE,
        out _PSW_REB,
        out HALT,

        in CLK2,
        in RESET,
        in IR[15:0],
        in _IR[15:0],
        in IRQ,
        in PSW_IE,
        in PSW_BT)
{
    ciow: IOW56 id_c
                      ii ii gi io go oo go oo go oo
                      io go oo go oo go oo go io oo (
                             I1: IRQ,
                             I2: PSW_IE,
                             I3: PSW_BT,
                             I4: RESET,
                             I6: CLK2,
                             I7: 1,
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
                            I21: 1,
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
                            I37: 1,
                            O38: BMUX_CON_0,
                            O39: MEM_WRITE,
                            O40: _PSW_IECLR);

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
}
