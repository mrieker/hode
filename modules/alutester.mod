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
# test physical alu board
# ./netgen.sh alutester.mod -sim aluboard.sim

# this version assumes the physical alu board is configured for the high byte
# ...as the low byte does not pass the VOUT signal to a connector

module alueight (out S[7:0], out COUT, out VOUT,
                in A[7:0], in B[7:0], in SHR, in CIN,
                in IR[7:0], in _IR[7:0],
                in IRFC, in ADD, in BONLY)
{
    wire abus[15:00], bbus[15:00];  # operands from gprs going to ALU
    wire dbus[15:00];               # dbus: result from ALU (going to gprs and memory)
    wire ir[15:00], _ir[15:00];     # ir: coming from instruction register
    wire irfc, add, bonly;          # control signals coming from sequencer

    wire rawcin, rawcmid, rawcout, rawvout;
    wire rawshrin, rawshrmid, rawshrout;

    S[7:0]      = dbus[15:08];  # physical circuit board outputs result on the dbus[15:08] pins
    COUT        = rawcout;      # physical circuit board outputs carry bit on rawcout pin
    VOUT        = rawvout;      # physical circuit board outputs overflow bit on rawvout pin
    abus[15:08] = A[7:0];       # physical circuit board gets A operand on abus[15:08] pins
    bbus[15:08] = B[7:0];       # physical circuit board gets B operand on bbus[15:08] pins
    rawshrin    = SHR;          # physical circuit board gets shift-right-carry-in on rawshrin pin
    rawcmid     = CIN;          # physical circuit board gets carry-in on rawcmid pin
    ir          = IR;
    _ir         = _IR;
    irfc        = IRFC;
    add         = ADD;
    bonly       = BONLY;

    abus[07:00] = 0xFF;         # these values go to the low-byte alu board
    bbus[07:00] = 0xFF;         # ...so we don't care what goes there
    rawcin      = 1;            # ...except use 1s cuz that's easier for the io-warrior-56 boards

    aiow: IOW56 id_a
                        ii ii gi ii gi ii gi ii gi ii
                        ii gi ii gi ii gi ii gi ii ii (
                            A[00], B[00], abus[08], bbus[08],
                            A[01], B[01], abus[09], bbus[09],
                            A[02], B[02], abus[10], bbus[10],
                            A[03], B[03], abus[11], bbus[11],
                            A[04], B[04], abus[12], bbus[12],
                            A[05], B[05], abus[13], bbus[13],
                            A[06], B[06], abus[14], bbus[14],
                            A[07], B[07], abus[15], bbus[15]);

    iiow: IOW56 id_i
                        ii      #  ir[00]   _ir[00]
                        ii      #  ir[01]   _ir[01]
                        gi      #            ir[02]
                        ii      # _ir[02]    ir[03]
                        gi      #           _ir[03]
                        ii      #  ir[04]   _ir[04]
                        gi      #            ir[05]
                        ii      # _ir[05]    ir[06]
                        gi      #           _ir[06]
                        ii      #  ir[07]   _ir[07]

                        ii      #  ir[08]   _ir[08]
                        gi      #            ir[09]
                        ii      # _ir[09]    ir[10]
                        gi      #           _ir[10]
                        ii      #  ir[11]   _ir[11]
                        gi      #            ir[12]
                        ii      # _ir[12]    ir[13]
                        gi      #           _ir[13]
                        ii      #  ir[14]   _ir[14]
                        ii (    #  ir[15]   _ir[15]
                            ir[00], _ir[00], ir[01], _ir[01], ir[02], _ir[02], ir[03], _ir[03],
                            ir[04], _ir[04], ir[05], _ir[05], ir[06], _ir[06], ir[07], _ir[07],
                            ir[08], _ir[08], ir[09], _ir[09], ir[10], _ir[10], ir[11], _ir[11],
                            ir[12], _ir[12], ir[13], _ir[13], ir[14], _ir[14], ir[15], _ir[15]);

    diow: IOW56 id_d
                      oo oo go oo go oo go oo go oo
                      ii oo go ii go oo gi ii gi ii (
                            dbus[15], dbus[07],
                            dbus[14], dbus[06],
                            dbus[13], dbus[05],
                            dbus[12], dbus[04],
                            dbus[11], dbus[03],
                            dbus[10], dbus[02],
                            dbus[09], dbus[01],
                            dbus[08], dbus[00],

                            I21:rawcin,     # carry into alu[00]
                            I22:rawcmid,    # carry into alu[08]
                            O23:rawcmid,    # carry outof alu[07]
                            O24:rawcout,    # carry outof alu[15]
                            O26:rawvout,    # carry outof alu[14]
                            I27:rawshrmid,  # shift right into alu[07]
                            I28:rawshrin,   # shift right into alu[15]
                            O30:abus[15],   # sign bit of A operand going into ALU
                            O31:rawshrout,  # shift right outof alu[00]
                            O32:rawshrmid,  # shift right outof alu[08]
                            I34:1,
                            I35:1,
                            I36:1,
                            I38:1,
                            I39:1,
                            I40:1);

    ciow: IOW56 id_c
                      ii ii gi ii gi ii gi ii gi ii
                      ii gi ii gi ii gi ii gi ii ii (
                             I1:1,
                             I2:1,
                             I3:1,
                             I4:1,
                             I6:1,
                             I7:1,
                             I8:1,
                            I10:1,
                            I11:irfc,
                            I12:1,
                            I14:1,
                            I15:1,
                            I16:1,
                            I18:1,
                            I19:1,
                            I20:1,
                            I21:1,
                            I22:1,
                            I24:bonly,
                            I25:1,
                            I26:1,
                            I28:1,
                            I29:1,
                            I30:add,
                            I32:1,
                            I33:1,
                            I34:1,
                            I36:1,
                            I37:1,
                            I38:1,
                            I39:1,
                            I40:1);
}
