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

# wire everything directly together for fanout counting
#  ./master.sh fanout.mod -gen fanout -report fanout.rep
#  grep -i exceed fanout.rep

module fanout (out clk2)
{
    wire abus[15:00], bbus[15:00], dbus[15:00], irbus[15:00], _irbus[15:00];
    wire midcarry, rawcin, rawcout, rawshrin, rawvout;
    wire alu_add, alu_bonly, alu_irfc;
    wire bmux_con_4, bmux_con2, bmux_con0, bmux_irbrof, bmux_irlsof;
    wire gprs_we7, gprs_rea7, gprs_wed, gprs_rea, gprs_reb, gprs_red2b;
    wire mem_read, mem_reb, mem_word, mem_write;
    wire psw_aluwe, psw_bt, psw_ie, _psw_ieclr, _psw_reb, psw_we;
    wire halt, irq, _ir_we, reset;

    alulo: alueight (S:dbus[07:00], COUT:midcarry,
                     A:abus[07:00], B:bbus[07:00], SHR:abus[08], CIN:rawcin,
                     IR:irbus[3:0], _IR:_irbus[3:0],
                     IRFC:alu_irfc, ADD:alu_add, BONLY:alu_bonly);

    aluhi: alueight (S:dbus[15:00], COUT:rawcout, VOUT:rawvout,
                     A:abus[15:00], B:bbus[15:00], SHR:rawshrin, CIN:midcarry,
                     IR:irbus[3:0], _IR:_irbus[3:0],
                     IRFC:alu_irfc, ADD:alu_add, BONLY:alu_bonly);

    rbc: rasbdcirc (
            bbus:bbus,
            irbus:irbus,
            _irbus:_irbus,
            clk2:clk2,
            reset:reset,
            irq:irq,

            dbus:dbus,
            halt:halt,
            mread:mem_read,
            mword:mem_word,
            mwrite:mem_write,
            mreb:mem_reb,
            con_4:bmux_con_4,
            con2:bmux_con2,
            con0:bmux_con0,
            irbrof:bmux_irbrof,
            irlsof:bmux_irlsof,
            _ir_we:_ir_we,

            psw_ie:psw_ie,
            psw_bt:psw_bt,
            rawcin:rawcin,
            rawshrin:rawshrin,
            _psw_ieclr:_psw_ieclr,
            psw_aluwe:psw_aluwe,
            psw_we:psw_we,
            _psw_reb:_psw_reb,
            rawcout:rawcout,
            rawvout:rawvout,
            rawshrout:abus[00],
            asign:abus[15],
            irfc:alu_irfc);

    r0r1: regpair (
            abus, bbus, dbus, clk2,
            gprs_wed, 0, gprs_rea, 0, gprs_reb, gprs_red2b,
             irbus[12],  irbus[11], irbus[10], _irbus[10],    # reg D selection
             irbus[09],  irbus[08], irbus[07], _irbus[07],    # reg A selection
             irbus[06],  irbus[05], irbus[04], _irbus[04]);   # reg B selection
    r2r3: regpair (
            abus, bbus, dbus, clk2,
            gprs_wed, 0, gprs_rea, 0, gprs_reb, gprs_red2b,
             irbus[12], _irbus[11], irbus[10], _irbus[10],    # reg D selection
             irbus[09], _irbus[08], irbus[07], _irbus[07],    # reg A selection
             irbus[06], _irbus[05], irbus[04], _irbus[04]);   # reg B selection
    r4r5: regpair (
            abus, bbus, dbus, clk2,
            gprs_wed, 0, gprs_rea, 0, gprs_reb, gprs_red2b,
            _irbus[12],  irbus[11], irbus[10], _irbus[10],    # reg D selection
            _irbus[09],  irbus[08], irbus[07], _irbus[07],    # reg A selection
            _irbus[06],  irbus[05], irbus[04], _irbus[04]);   # reg B selection
    r6r7: regpair (
            abus, bbus, dbus, clk2,
            gprs_wed, gprs_we7, gprs_rea, gprs_rea7, gprs_reb, gprs_red2b,
            _irbus[12], _irbus[11], irbus[10], _irbus[10],    # reg D selection
            _irbus[09], _irbus[08], irbus[07], _irbus[07],    # reg A selection
            _irbus[06], _irbus[05], irbus[04], _irbus[04]);   # reg B selection

    seq: sequencer (
                 ALU_ADD:     alu_add,
                 ALU_IRFC:    alu_irfc,
                 ALU_BONLY:   alu_bonly,
                 BMUX_CON_0:  bmux_con0,
                 BMUX_CON_2:  bmux_con2,
                 BMUX_CON__4: bmux_con_4,
                 BMUX_IRBROF: bmux_irbrof,
                 BMUX_IRLSOF: bmux_irlsof,
                 GPRS_WE7:    gprs_we7,
                 GPRS_REA7:   gprs_rea7,
                 GPRS_WED:    gprs_wed,
                 GPRS_REA:    gprs_rea,
                 GPRS_REB:    gprs_reb,
                 GPRS_RED2B:  gprs_red2b,
                 _IR_WE:      _ir_we,
                 MEM_READ:    mem_read,
                 MEM_WORD:    mem_word,
                 MEM_WRITE:   mem_write,
                 MEM_REB:     mem_reb,
                 _PSW_IECLR:  _psw_ieclr,
                 PSW_ALUWE:   psw_aluwe,
                 PSW_WE:      psw_we,
                 _PSW_REB:    _psw_reb,
                 HALT:        halt,

                 CLK2:        clk2,
                 RESET:       reset,
                 IR:          irbus,
                 _IR:         _irbus,
                 IRQ:         irq,
                 PSW_IE:      psw_ie,
                 PSW_BT:      psw_bt);
}
