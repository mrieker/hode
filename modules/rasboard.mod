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
# board contains:
#  raspberry pi connector
#  instruction register
#  processor status register

module rasbdcirc (
        out bbus[15:00],        # ALU input (open collector)
        out irbus[15:00],       # instruction register
        out _irbus[15:00],
        out clk2,               # clock signal
        out reset,              # reset processor
        out irq,                # interrupt request
        in dbus[15:00],         # ALU output
        in halt,                # HALT instruction being executed
        in mread,               # memory read being requested
        in mword,               # word-sized transfer (not byte)
        in mwrite,              # memory write being requested
        in mreb,                # gate memory read data onto B bus
        in con0,
        in con2,
        in con_4,
        in irbrof,
        in irlsof,
        in _ir_we,              # write instruction register at end of cycle

        out psw_ie,             # psw interrupt enable bit as saved in flipflops
        out psw_bt,             # psw condition codes match branch condition in instruction register
        out rawcin,             # carry bit going into bottom of ALU
        out rawshrin,           # shift right bit going into top of ALU
        in _psw_ieclr,          # clear interrupt enable flipflop immediately
        in psw_aluwe,           # write psw condition codes at end of cycle
        in psw_we,              # write whole psw register at end of cycle
        in _psw_reb,            # gate psw register contents out to bbus
        in rawcout,             # carry bit out of top of ALU
        in rawvout,             # carry bit out of next to top of ALU
        in rawshrout,           # carry bit out of bottom of ALU
        in asign,               # sign bit of 'A' operand going into ALU
        in irfc)                # performing function indicated in irbus[]
{
    wire mqbus[15:00];

    raspi: RasPi (CLK:clk2, IRQ:irq, RES:reset, MQ:mqbus, MD:dbus, MWORD:mword, MREAD:mread, MWRITE:mwrite, HALT:halt);

    ir: insreg (Q:irbus, _Q:_irbus, MQ:mqbus, _WE:_ir_we);

    mrd: memread (Q:bbus, D:mqbus, WORD:mword, SEXT:irbus[14], RE:mreb);

    bmux: bmux (BBUS:bbus, CON_4:con_4, CON2:con2, CON0:con0, _IR:_irbus, IRBROF:irbrof, IRLSOF:irlsof);

    psw: psw (
        bbus15:bbus[15],        # 'B' operand to ALU
        bbus30:bbus[3:0],
        psw_ie:psw_ie,          # psw interrupt enable bit as saved in flipflops
        psw_bt:psw_bt,          # psw condition codes match branch condition in instruction register
        rawcin:rawcin,          # carry bit going into bottom of ALU
        rawshrin:rawshrin,      # shift right bit going into top of ALU
        irbus:irbus,            # positive instruction register
        _irbus:_irbus,          # negative instruction register
        dbus:dbus,              # result coming out of ALU
        _psw_ieclr:_psw_ieclr,  # clear interrupt enable flipflop immediately
        psw_aluwe:psw_aluwe,    # write psw condition codes at end of cycle
        psw_we:psw_we,          # write whole psw register at end of cycle
        _psw_reb:_psw_reb,      # gate psw register contents out to bbus
        rawcout:rawcout,        # carry bit out of top of ALU
        rawvout:rawvout,        # carry bit out of next to top of ALU
        rawshrout:rawshrout,    # carry bit out of bottom of ALU
        asign:asign,            # sign bit of 'A' operand going into ALU
        clk2:clk2,              # positive edge indicates end of cycle
        irfc:irfc);             # performing function indicated in irbus[]
}

module rasboard ()
{
    wire irbus[15:00], _irbus[15:00];
    wire clk2, reset, irq, _ir_we;
    wire bbus[15:00], dbus[15:00];
    wire mword, mread, mwrite, mreb, halt;
    wire con0, con2, con_4, irbrof, irlsof, irfc;
    wire psw_ie, psw_bt, _psw_ieclr, psw_aluwe, psw_we, _psw_reb;
    wire rawcin, rawshrin, rawcout, rawvout, rawshrout, asign;

    abbus: Conn
                       io io gi oi go io gi oi go io
                       io gi oi go io gi oi go io io (
                             O2:bbus[00],  O4:bbus[08],
                             O7:bbus[01], O10:bbus[09],
                            O12:bbus[02], O15:bbus[10],
                            O18:bbus[03], O20:bbus[11],
                            O22:bbus[04], O25:bbus[12],
                            O28:bbus[05], O30:bbus[13],
                            O33:bbus[06], O36:bbus[14],
                            O38:bbus[07], O40:bbus[15],
                            I39:asign);     # asign = abus[15]

    irbus: Conn oo oo go oo go oo go oo go oo oo go oo go oo go oo go oo oo (
                                irbus[00], _irbus[00],
                                irbus[01], _irbus[01],
                                irbus[02], _irbus[02],
                                irbus[03], _irbus[03],
                                irbus[04], _irbus[04],
                                irbus[05], _irbus[05],
                                irbus[06], _irbus[06],
                                irbus[07], _irbus[07],
                                irbus[08], _irbus[08],
                                irbus[09], _irbus[09],
                                irbus[10], _irbus[10],
                                irbus[11], _irbus[11],
                                irbus[12], _irbus[12],
                                irbus[13], _irbus[13],
                                irbus[14], _irbus[14],
                                irbus[15], _irbus[15]);

    dbus: Conn
                      ii ii gi ii gi ii gi ii gi ii     # dbus coming into board
                      oi gi ii gi oi gi ii gi ii ii (   # carry/overflow for psw
                            dbus[15], dbus[07],
                            dbus[14], dbus[06],
                            dbus[13], dbus[05],
                            dbus[12], dbus[04],
                            dbus[11], dbus[03],
                            dbus[10], dbus[02],
                            dbus[09], dbus[01],
                            dbus[08], dbus[00],

                                O21:rawcin,     # carry into bottom of ALU for ADC and SBB
                                I25:rawcout,    # carry out from top of ALU
                                I26:rawvout,    # overflow out from near top of ALU
                                O29:rawshrin,   # shift right into top of ALU
                                I32:rawshrout   # shift right out from bottom of ALU
                            );

    ctla: Conn oo oo go ii gi ii gi ii gi ii ii gi ii gi ii gi ii gi ii ii (
                                 O1:irq,        # raspi control program wants to interrupt CPU
                                 O2:psw_ie,
                                 O3:psw_bt,
                                 O4:reset,      # reset generated by raspi going to sequencer
                                 O6:clk2,       # clock generated by raspi going out to everything
                                 I8:mreb,       # sequencer wants memory read data put on B bus
                                I10:psw_aluwe,
                                I11:irfc,
                                I14:irbrof,
                                I16:mread,      # sequencer wants to read from memory (raspi)
                                I18:psw_we,
                                I19:halt,       # sequencer is executing an HALT instruction
                                I25:irlsof,
                                I26:_psw_reb,
                                I28:con_4,
                                I29:mword,      # sequencer wants a word transfer (not byte) to/from memory (raspi)
                                I34:con2,
                                I36:_ir_we,     # sequencer wants to load instruction register latch this cycle
                                I38:con0,       #
                                I39:mwrite,     # sequencer wants to write to memory (raspi)
                                I40:_psw_ieclr);

    power: Conn pwrcap v v v g g g ();

    rbc: rasbdcirc (
            bbus:bbus,
            irbus:irbus,
            _irbus:_irbus,
            clk2:clk2,
            reset:reset,
            irq:irq,

            dbus:dbus,
            halt:halt,
            mread:mread,
            mword:mword,
            mwrite:mwrite,
            mreb:mreb,
            con0:con0,
            con2:con2,
            con_4:con_4,
            irbrof:irbrof,
            irlsof:irlsof,
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
            rawshrout:rawshrout,
            asign:asign,
            irfc:irfc);
}
