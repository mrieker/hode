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

module ledboard (in _ir[15:00])
{
    wire abus[15:00], bbus[15:00];

    abbus: Conn
            oo oo go oo go oo go oo go oo
            oo go oo go oo go oo go oo oo (
                abus[00], bbus[00], abus[08], bbus[08],
                abus[01], bbus[01], abus[09], bbus[09],
                abus[02], bbus[02], abus[10], bbus[10],
                abus[03], bbus[03], abus[11], bbus[11],
                abus[04], bbus[04], abus[12], bbus[12],
                abus[05], bbus[05], abus[13], bbus[13],
                abus[06], bbus[06], abus[14], bbus[14],
                abus[07], bbus[07], abus[15], bbus[15]);

    A: BufLED 16 (abus);
    B: BufLED 16 (bbus);

    wire clk2, reset, irq, psw_ie, psw_bt;
    wire alu_add, alu_irfc, alu_bonly;
    wire bmux_con_0, bmux_con_2, bmux_con__4, bmux_irbrof, bmux_irlsof;
    wire gprs_we7, gprs_rea7, gprs_wed, gprs_rea;
    wire gprs_reb, gprs_red2b, _ir_we;
    wire mem_read, mem_word, mem_write, mem_reb;
    wire _psw_ieclr, psw_aluwe, psw_we, _psw_reb, halt;

    ctla: Conn
            oo oo go io go oo go oo go oo
            io go oo go oo go oo go io oo (
                 O1: irq,
                 O2: psw_ie,
                 O3: psw_bt,
                 O4: reset,
                 O6: clk2,
                 O8: mem_reb,
                O10: psw_aluwe,
                O11: alu_irfc,
                O12: gprs_reb,
                O14: bmux_irbrof,
                O15: gprs_wed,
                O16: mem_read,
                O18: psw_we,
                O19: halt,
                O20: gprs_rea,
                O22: gprs_red2b,
                O24: alu_bonly,
                O25: bmux_irlsof,
                O26: _psw_reb,
                O28: bmux_con__4,
                O29: mem_word,
                O30: alu_add,
                O32: gprs_we7,
                O33: gprs_rea7,
                O34: bmux_con_2,
                O36: _ir_we,
                O38: bmux_con_0,
                O39: mem_write,
                O40: _psw_ieclr);

    IRQ:         BufLED (irq);
    PSW_IE:      BufLED (psw_ie);
    PSW_BT:      BufLED (psw_bt);
    RESET:       BufLED (reset);
    CLK2:        BufLED (clk2);
    MEM_REB:     BufLED (mem_reb);
    PSW_ALUWE:   BufLED (psw_aluwe);
    ALU_IRFC:    BufLED (alu_irfc);
    GPRS_REB:    BufLED (gprs_reb);
    BMUX_IRBROF: BufLED (bmux_irbrof);
    GPRS_WED:    BufLED (gprs_wed);
    MEM_READ:    BufLED (mem_read);
    PSW_WE:      BufLED (psw_we);
    HALT:        BufLED (halt);
    GPRS_REA:    BufLED (gprs_rea);
    GPRS_RED2B:  BufLED (gprs_red2b);
    ALU_BONLY:   BufLED (alu_bonly);
    BMUX_IRLSOF: BufLED (bmux_irlsof);
    _PSW_REB:    BufLED inv (_psw_reb);
    BMUX_CON__4: BufLED (bmux_con__4);
    MEM_WORD:    BufLED (mem_word);
    ALU_ADD:     BufLED (alu_add);
    GPRS_WE7:    BufLED (gprs_we7);
    GPRS_REA7:   BufLED (gprs_rea7);
    BMUX_CON_2:  BufLED (bmux_con_2);
    _IR_WE:      BufLED inv (_ir_we);
    BMUX_CON_0:  BufLED (bmux_con_0);
    MEM_WRITE:   BufLED (mem_write);
    _PSW_IECLR:  BufLED inv (_psw_ieclr);

    wire ir[15:00];

    irbus: Conn
            oo oo go oo go oo go oo go oo
            oo go oo go oo go oo go oo oo (
                ir[00], _ir[00],
                ir[01], _ir[01],
                ir[02], _ir[02],
                ir[03], _ir[03],
                ir[04], _ir[04],
                ir[05], _ir[05],
                ir[06], _ir[06],
                ir[07], _ir[07],
                ir[08], _ir[08],
                ir[09], _ir[09],
                ir[10], _ir[10],
                ir[11], _ir[11],
                ir[12], _ir[12],
                ir[13], _ir[13],
                ir[14], _ir[14],
                ir[15], _ir[15]);

    I: BufLED 16 (ir);

    wire dbus[15:00];
    wire rawcin, rawcmid, rawcout, rawvout;
    wire rawshrin, rawshrmid, rawshrout;

    dbus: Conn
            oo oo go oo go oo go oo go oo
            oi go oo gi oi go oi gi ii ii (
                dbus[15], dbus[07],
                dbus[14], dbus[06],
                dbus[13], dbus[05],
                dbus[12], dbus[04],
                dbus[11], dbus[03],
                dbus[10], dbus[02],
                dbus[09], dbus[01],
                dbus[08], dbus[00],

                O21:rawcin,     # carry into alu[00]
                O24:rawcmid,    # carry outof alu[07]
                O25:rawcout,    # carry outof alu[15]
                O26:rawvout,    # carry outof alu[14]
                O29:rawshrin,   # shift right into alu[15]
                O32:rawshrout,  # shift right outof alu[00]
                O33:rawshrmid); # shift right outof alu[08]

    D: BufLED 16 (dbus);
    RAWCIN:    BufLED (rawcin);
    RAWCMID:   BufLED (rawcmid);
    RAWCOUT:   BufLED (rawcout);
    RAWVOUT:   BufLED (rawvout);
    RAWSHRIN:  BufLED (rawshrin);
    RAWSHRMID: BufLED (rawshrmid);
    RAWSHROUT: BufLED (rawshrout);

    power: Conn pwrcap v v v g g g ();
}
