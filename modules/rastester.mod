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
# test physical ras board
# must be run on raspi plugged into rasboard being tested
# ...and with the 4 IOW56Paddles plugged into the rasboard
# ./netgen.sh rastester.mod -sim rastester.sim

# this looks just like the rasbdcirc in rasboard.mod used to generate circuitry for the ras circuit board
# except instead of verilog-like code, it does the computations for simulation by being plugged into a physical ras circuit board
module rasbdcirc (
        out bbus[15:00],        # ALU input
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
        in irfc,                # performing function indicated in irbus[]

        out rpi_md[15:00],      # mem address or write data
        out rpi_mword,          # read/write word (not byte)
        out rpi_mread,          # read memory
        out rpi_mwrite,         # write memory
        out rpi_halt,           # cpu has halted
        in rpi_clk2,            # send clock to cpu
        in rpi_irq,             # send interrupt request to cpu
        in rpi_reset,           # send reset to cpu
        in rpi_mq[15:00],       # send mem data to cpu
        in rpi_dena,            # enable rpi_md to go to cpu
        in rpi_qena)            # enable rpi_mq to come from cpu
{
    aiow: IOW56 id_a
                       io io gi oi go io gi oi go io
                       io gi oi go io gi oi go io oo (
                             I1:1,  I3:1,   # 1s tell the iow56paddle to go high-impeadance
                             I6:1,  I8:1,   # ...cuz they are open-drain outputs
                            I11:1, I14:1,
                            I16:1, I19:1,
                            I21:1, I24:1,
                            I26:1, I29:1,
                            I32:1, I34:1,
                            I37:1, O39:asign,
                             O2:bbus[00],  O4:bbus[08],
                             O7:bbus[01], O10:bbus[09],
                            O12:bbus[02], O15:bbus[10],
                            O18:bbus[03], O20:bbus[11],
                            O22:bbus[04], O25:bbus[12],
                            O28:bbus[05], O30:bbus[13],
                            O33:bbus[06], O36:bbus[14],
                            O38:bbus[07], O40:bbus[15]);

    ciow: IOW56 id_c oo oo go ii gi ii gi ii gi ii ii gi ii gi ii gi ii gi ii ii (
                                 O1:irq,        # raspi control program wants to interrupt CPU
                                 O2:psw_ie,
                                 O3:psw_bt,
                                 O4:reset,      # reset generated by raspi going to sequencer
                                 O6:clk2,       # clock generated by raspi going out to everything
                                 I7:1,
                                 I8:mreb,       # sequencer wants memory read data put on B bus
                                I10:psw_aluwe,
                                I11:irfc,
                                I12:1,
                                I14:irbrof,
                                I15:1,
                                I16:mread,      # sequencer wants to read from memory (raspi)
                                I18:psw_we,
                                I19:halt,       # sequencer is executing an HALT instruction
                                I20:1,
                                I21:1,
                                I22:1,
                                I24:1,
                                I25:irlsof,
                                I26:_psw_reb,
                                I28:con_4,
                                I29:mword,      # sequencer wants a word transfer (not byte) to/from memory (raspi)
                                I30:1,
                                I32:1,
                                I33:1,
                                I34:con2,
                                I36:_ir_we,     # sequencer wants to load instruction register latch this cycle
                                I37:1,
                                I38:con0,       #
                                I39:mwrite,     # sequencer wants to write to memory (raspi)
                                I40:_psw_ieclr);

    iiow: IOW56 id_i oo oo go oo go oo go oo go oo oo go oo go oo go oo go oo oo (
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

    diow: IOW56 id_d
                      ii ii gi ii gi ii gi ii gi ii     # dbus coming into board
                      oi ii gi io gi ii gi ii gi ii (   # carry/overflow for psw
                            dbus[15], dbus[07],
                            dbus[14], dbus[06],
                            dbus[13], dbus[05],
                            dbus[12], dbus[04],
                            dbus[11], dbus[03],
                            dbus[10], dbus[02],
                            dbus[09], dbus[01],
                            dbus[08], dbus[00],

                                O21:rawcin,     # carry into bottom of ALU for ADC and SBB
                                I22:1,
                                I23:1,
                                I24:rawcout,    # carry out from top of ALU
                                I26:rawvout,    # overflow out from near top of ALU
                                I27:1,
                                O28:rawshrin,   # shift right into top of ALU
                                I30:1,
                                I31:rawshrout,  # shift right out from bottom of ALU
                                I32:1,
                                I34:1,
                                I35:1,
                                I36:1,
                                I38:1,
                                I39:1,
                                I40:1
                            );

    # access the raspi connector on the physical board via this raspi's gpio pins
    gpio: GPIO (MD:rpi_md, MWORD:rpi_mword, MREAD:rpi_mread, MWRITE:rpi_mwrite, HALT:rpi_halt,
        CLK:rpi_clk2, IRQ:rpi_irq, RES:rpi_reset, MQ:rpi_mq, DENA:rpi_dena, QENA:rpi_qena);
}
