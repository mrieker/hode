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

# hook all the boards together with jumpers for simulation

# put clk2 as an output so simulated raspi gets stepSimWork() called first
#  so it will output the STARTING STATE messages at beginning of cycle
module master (out clk2, out conapins[31:00], out concpins[31:00], out conipins[31:00], out condpins[31:00])
{
    # boards stacked as in real life
    alulo: aluboard ();
    aluhi: aluboard ();
    rasbd: rasboard ();
    r0r1:  regboard ();
    r2r3:  regboard ();
    r4r5:  regboard ();
    r6r7:  regboard ();
    seqbd: seqboard ();

    clk2 = clk2/rasbd;

    conapins[00] =  I1/abbus/alulo;
    conapins[01] =  I2/abbus/alulo;
    conapins[02] =  I3/abbus/aluhi;
    conapins[03] =  I4/abbus/aluhi;
    conapins[04] =  I6/abbus/alulo;
    conapins[05] =  I7/abbus/alulo;
    conapins[06] =  I8/abbus/aluhi;
    conapins[07] = I10/abbus/aluhi;
    conapins[08] = I11/abbus/alulo;
    conapins[09] = I12/abbus/alulo;
    conapins[10] = I14/abbus/aluhi;
    conapins[11] = I15/abbus/aluhi;
    conapins[12] = I16/abbus/alulo;
    conapins[13] = I18/abbus/alulo;
    conapins[14] = I19/abbus/aluhi;
    conapins[15] = I20/abbus/aluhi;
    conapins[16] = I21/abbus/alulo;
    conapins[17] = I22/abbus/alulo;
    conapins[18] = I24/abbus/aluhi;
    conapins[19] = I25/abbus/aluhi;
    conapins[20] = I26/abbus/alulo;
    conapins[21] = I28/abbus/alulo;
    conapins[22] = I29/abbus/aluhi;
    conapins[23] = I30/abbus/aluhi;
    conapins[24] = I32/abbus/alulo;
    conapins[25] = I33/abbus/alulo;
    conapins[26] = I34/abbus/aluhi;
    conapins[27] = I36/abbus/aluhi;
    conapins[28] = I37/abbus/alulo;
    conapins[29] = I38/abbus/alulo;
    conapins[30] = I39/abbus/aluhi;
    conapins[31] = I40/abbus/aluhi;

    concpins[00] =  O1/ctla/rasbd;      # irq
    concpins[01] =  O2/ctla/rasbd;      # psw_ie
    concpins[02] =  O3/ctla/rasbd;      # psw_bt
    concpins[03] =  O4/ctla/rasbd;      # reset
    concpins[04] =  O6/ctla/rasbd;      # clk2
    concpins[05] = 1;
    concpins[06] =  O8/ctla/seqbd;      # mem_reb
    concpins[07] = O10/ctla/seqbd;      # psw_aluwe
    concpins[08] = O11/ctla/seqbd;      # alu_irfc
    concpins[09] = O12/ctla/seqbd;      # gprs_reb
    concpins[10] = O14/ctla/seqbd;      # bmux_irbrof
    concpins[11] = O15/ctla/seqbd;      # gprs_wed
    concpins[12] = O16/ctla/seqbd;      # mem_read
    concpins[13] = O18/ctla/seqbd;      # psw_we
    concpins[14] = O19/ctla/seqbd;      # halt
    concpins[15] = O20/ctla/seqbd;      # gprs_rea
    concpins[16] = 1;
    concpins[17] = O22/ctla/seqbd;      # gprs_red2b
    concpins[18] = O24/ctla/seqbd;      # alu_bonly
    concpins[19] = O25/ctla/seqbd;      # bmux_irlsof
    concpins[20] = O26/ctla/seqbd;      # _psw_re
    concpins[21] = O28/ctla/seqbd;      # bmux_con__4
    concpins[22] = O29/ctla/seqbd;      # mem_word
    concpins[23] = O30/ctla/seqbd;      # alu_add
    concpins[24] = O32/ctla/seqbd;      # gprs_we7
    concpins[25] = O33/ctla/seqbd;      # gprs_rea7
    concpins[26] = O34/ctla/seqbd;      # bmux_con_2
    concpins[27] = O36/ctla/seqbd;      # _ir_we
    concpins[28] = 1;
    concpins[29] = O38/ctla/seqbd;      # bmux_con_0
    concpins[30] = O39/ctla/seqbd;      # mem_write
    concpins[31] = O40/ctla/seqbd;      # _psw_ieclr

    conipins[00] =  O1/irbus/rasbd;
    conipins[01] =  O2/irbus/rasbd;
    conipins[02] =  O3/irbus/rasbd;
    conipins[03] =  O4/irbus/rasbd;
    conipins[04] =  O6/irbus/rasbd;
    conipins[05] =  O7/irbus/rasbd;
    conipins[06] =  O8/irbus/rasbd;
    conipins[07] = O10/irbus/rasbd;
    conipins[08] = O11/irbus/rasbd;
    conipins[09] = O12/irbus/rasbd;
    conipins[10] = O14/irbus/rasbd;
    conipins[11] = O15/irbus/rasbd;
    conipins[12] = O16/irbus/rasbd;
    conipins[13] = O18/irbus/rasbd;
    conipins[14] = O19/irbus/rasbd;
    conipins[15] = O20/irbus/rasbd;
    conipins[16] = O21/irbus/rasbd;
    conipins[17] = O22/irbus/rasbd;
    conipins[18] = O24/irbus/rasbd;
    conipins[19] = O25/irbus/rasbd;
    conipins[20] = O26/irbus/rasbd;
    conipins[21] = O28/irbus/rasbd;
    conipins[22] = O29/irbus/rasbd;
    conipins[23] = O30/irbus/rasbd;
    conipins[24] = O32/irbus/rasbd;
    conipins[25] = O33/irbus/rasbd;
    conipins[26] = O34/irbus/rasbd;
    conipins[27] = O36/irbus/rasbd;
    conipins[28] = O37/irbus/rasbd;
    conipins[29] = O38/irbus/rasbd;
    conipins[30] = O39/irbus/rasbd;
    conipins[31] = O40/irbus/rasbd;

    condpins[00] =  O1/dbus/aluhi;
    condpins[01] =  O2/dbus/alulo;
    condpins[02] =  O3/dbus/aluhi;
    condpins[03] =  O4/dbus/alulo;
    condpins[04] =  O6/dbus/aluhi;
    condpins[05] =  O7/dbus/alulo;
    condpins[06] =  O8/dbus/aluhi;
    condpins[07] = O10/dbus/alulo;
    condpins[08] = O11/dbus/aluhi;
    condpins[09] = O12/dbus/alulo;
    condpins[10] = O14/dbus/aluhi;
    condpins[11] = O15/dbus/alulo;
    condpins[12] = O16/dbus/aluhi;
    condpins[13] = O18/dbus/alulo;
    condpins[14] = O19/dbus/aluhi;
    condpins[15] = O20/dbus/alulo;
    condpins[16] = O21/dbus/rasbd;
    condpins[17] = O24/dbus/alulo;
    condpins[18] = O24/dbus/alulo;
    condpins[19] = O25/dbus/alulo;
    condpins[20] = O26/dbus/alulo;
    condpins[21] = O33/dbus/alulo;
    condpins[22] = O29/dbus/rasbd;
    condpins[23] = 1;
    condpins[24] = O32/dbus/alulo;
    condpins[25] = O33/dbus/alulo;
    condpins[26] = 1;
    condpins[27] = 1;
    condpins[28] = 1;
    condpins[29] = 1;
    condpins[30] = 1;
    condpins[31] = 1;

    # simulate stacking the boards
    interconnect abbus/alulo, abbus/aluhi, abbus/rasbd, abbus/r0r1, abbus/r2r3, abbus/r4r5, abbus/r6r7, abbus/seqbd;
    interconnect irbus/alulo, irbus/aluhi, irbus/rasbd, irbus/r0r1, irbus/r2r3, irbus/r4r5, irbus/r6r7, irbus/seqbd;
    interconnect  dbus/alulo,  dbus/aluhi,  dbus/rasbd,  dbus/r0r1,  dbus/r2r3,  dbus/r4r5,  dbus/r6r7,  dbus/seqbd;
    interconnect  ctla/alulo,  ctla/aluhi,  ctla/rasbd,  ctla/r0r1,  ctla/r2r3,  ctla/r4r5,  ctla/r6r7,  ctla/seqbd;

    # jumpers for alulo board
    jumper  I5/absel/alulo =  O4/absel/alulo;   # ain[0] = abus[00]
    jumper  I8/absel/alulo =  O7/absel/alulo;   # ain[4] = abus[04]
    jumper I11/absel/alulo = O10/absel/alulo;   # ain[1] = abus[01]
    jumper I14/absel/alulo = O13/absel/alulo;   # ain[5] = abus[05]
    jumper I17/absel/alulo = O16/absel/alulo;   # ain[2] = abus[02]
    jumper I20/absel/alulo = O19/absel/alulo;   # ain[6] = abus[06]
    jumper I23/absel/alulo = O22/absel/alulo;   # ain[3] = abus[03]
    jumper I26/absel/alulo = O25/absel/alulo;   # ain[7] = abus[07]

    jumper I35/absel/alulo = O34/absel/alulo;   # bin[0] = bbus[00]
    jumper I38/absel/alulo = O37/absel/alulo;   # bin[4] = bbus[04]
    jumper I41/absel/alulo = O40/absel/alulo;   # bin[1] = bbus[01]
    jumper I44/absel/alulo = O43/absel/alulo;   # bin[5] = bbus[05]
    jumper I47/absel/alulo = O46/absel/alulo;   # bin[2] = bbus[02]
    jumper I50/absel/alulo = O49/absel/alulo;   # bin[6] = bbus[06]
    jumper I53/absel/alulo = O52/absel/alulo;   # bin[3] = bbus[03]
    jumper I56/absel/alulo = O55/absel/alulo;   # bin[7] = bbus[07]

    jumper  I4/dsel/alulo =  O5/dsel/alulo;     # dbus[00] = dout[0]
    jumper  I7/dsel/alulo =  O8/dsel/alulo;     # dbus[04] = dout[4]
    jumper I10/dsel/alulo = O11/dsel/alulo;     # dbus[01] = dout[1]
    jumper I13/dsel/alulo = O14/dsel/alulo;     # dbus[05] = dout[5]
    jumper I16/dsel/alulo = O17/dsel/alulo;     # dbus[02] = dout[2]
    jumper I19/dsel/alulo = O20/dsel/alulo;     # dbus[06] = dout[6]
    jumper I22/dsel/alulo = O23/dsel/alulo;     # dbus[03] = dout[3]
    jumper I25/dsel/alulo = O26/dsel/alulo;     # dbus[07] = dout[7]

    jumper  I6/dsel/alulo = 1;                  # dbus[08] = 1 going into the wired-and
    jumper  I9/dsel/alulo = 1;                  # dbus[12] = 1 ...with aluhi writing dbus[15:08]
    jumper I12/dsel/alulo = 1;                  # dbus[09] = 1
    jumper I15/dsel/alulo = 1;                  # dbus[13] = 1
    jumper I18/dsel/alulo = 1;                  # dbus[10] = 1
    jumper I21/dsel/alulo = 1;                  # dbus[14] = 1
    jumper I24/dsel/alulo = 1;                  # dbus[11] = 1
    jumper I27/dsel/alulo = 1;                  # dbus[15] = 1

    jumper I35/dsel/alulo = O34/dsel/alulo;     # cin = rawcin          carry-in to ALU-lo comes from PSW<C> bit
    jumper I37/dsel/alulo = O38/dsel/alulo;     # rawcmid = cout        mid-carry comes from ALU-lo carry-out
    jumper I39/dsel/alulo = 1;                  # rawcout = 1           for wire-and with aluhi writing rawcout
    jumper I42/dsel/alulo = 1;                  # rawvout = 1           for wire-and with aluhi writing rawvout
    jumper I44/dsel/alulo = O43/dsel/alulo;     # shrin = rawshrmid     shr-in to ALU-lo cones from mid-shr
    jumper I46/dsel/alulo = O47/dsel/alulo;     # rawshrout = ain[0]    shr-out from ALU[0] comes from ain[0]
    jumper I48/dsel/alulo = 1;                  # rawshrmid = 1         for wire-and with aluhi writing rawshrmid

    # not an actual jumper, but accounts for I22/dbus & O24/dbus being connected together on both aluhi and alulo boards
    #  forwards the mid-carry coming out of alulo on to aluhi
    jumper I22/dbus/aluhi = O24/dbus/alulo;     # rawcmid/aluhi = rawcmid/alulo

    # not an actual jumper, but accounts for I28/dbus & O33/dbus being connected together on both aluhi and alulo boards
    #  forwards the mid-shr coming out of aluhi on to alulo
    jumper I28/dbus/alulo = O33/dbus/aluhi;     # rawshrmid/alulo = rawshrmid/aluhi

    # jumpers for aluhi board
    jumper  I5/absel/aluhi =  O6/absel/aluhi;   # ain[0] = abus[00]
    jumper  I8/absel/aluhi =  O9/absel/aluhi;   # ain[4] = abus[04]
    jumper I11/absel/aluhi = O12/absel/aluhi;   # ain[1] = abus[01]
    jumper I14/absel/aluhi = O15/absel/aluhi;   # ain[5] = abus[05]
    jumper I17/absel/aluhi = O18/absel/aluhi;   # ain[2] = abus[02]
    jumper I20/absel/aluhi = O21/absel/aluhi;   # ain[6] = abus[06]
    jumper I23/absel/aluhi = O24/absel/aluhi;   # ain[3] = abus[03]
    jumper I26/absel/aluhi = O27/absel/aluhi;   # ain[7] = abus[07]

    jumper I35/absel/aluhi = O36/absel/aluhi;   # bin[0] = bbus[00]
    jumper I38/absel/aluhi = O39/absel/aluhi;   # bin[4] = bbus[04]
    jumper I41/absel/aluhi = O42/absel/aluhi;   # bin[1] = bbus[01]
    jumper I44/absel/aluhi = O45/absel/aluhi;   # bin[5] = bbus[05]
    jumper I47/absel/aluhi = O48/absel/aluhi;   # bin[2] = bbus[02]
    jumper I50/absel/aluhi = O51/absel/aluhi;   # bin[6] = bbus[06]
    jumper I53/absel/aluhi = O54/absel/aluhi;   # bin[3] = bbus[03]
    jumper I56/absel/aluhi = O57/absel/aluhi;   # bin[7] = bbus[07]

    jumper  I4/dsel/aluhi = 1;                  # dbus[00] = 1 going into the wired-and
    jumper  I7/dsel/aluhi = 1;                  # dbus[04] = 1 ...with alulo writing dbus[07:00]
    jumper I10/dsel/aluhi = 1;                  # dbus[01] = 1
    jumper I13/dsel/aluhi = 1;                  # dbus[05] = 1
    jumper I16/dsel/aluhi = 1;                  # dbus[02] = 1
    jumper I19/dsel/aluhi = 1;                  # dbus[06] = 1
    jumper I22/dsel/aluhi = 1;                  # dbus[03] = 1
    jumper I25/dsel/aluhi = 1;                  # dbus[07] = 1

    jumper  I6/dsel/aluhi =  O5/dsel/aluhi;     # dbus[08] = dout[0]
    jumper  I9/dsel/aluhi =  O8/dsel/aluhi;     # dbus[13] = dout[4]
    jumper I12/dsel/aluhi = O11/dsel/aluhi;     # dbus[09] = dout[1]
    jumper I15/dsel/aluhi = O14/dsel/aluhi;     # dbus[13] = dout[5]
    jumper I18/dsel/aluhi = O17/dsel/aluhi;     # dbus[10] = dout[2]
    jumper I21/dsel/aluhi = O20/dsel/aluhi;     # dbus[14] = dout[6]
    jumper I24/dsel/aluhi = O23/dsel/aluhi;     # dbus[11] = dout[3]
    jumper I27/dsel/aluhi = O26/dsel/aluhi;     # dbus[15] = dout[7]

    jumper I35/dsel/aluhi = O36/dsel/aluhi;     # cin = rawcmid         carry-in to ALU-hi comes from mid-carry
    jumper I37/dsel/aluhi = 1;                  # rawcmid = 1           for wire-and with alulo writing rawcmid
    jumper I39/dsel/aluhi = O38/dsel/aluhi;     # rawcout = cout        top carry to PSW<C> comes from ALU-hi carry-out
    jumper I42/dsel/aluhi = O41/dsel/aluhi;     # rawvout = vout        overflow to PSW<V> comes from ALU-hi overflow
    jumper I44/dsel/aluhi = O45/dsel/aluhi;     # shrin = rawshrin      shr-in to ALU-hi cones from top shr bit (possibly PSW<C>)
    jumper I46/dsel/aluhi = 1;                  # rawshrout = 1         for wire-and with alulo writing rawshrout
    jumper I48/dsel/aluhi = O47/dsel/aluhi;     # rawshrmid = ain[8]    shr-mid comes from ain[8]

    # jumpers for r0r1 board
    jumper I20/irsel/r0r1 = O19/irsel/r0r1;     # _ir05 =  IR[05]
    jumper I23/irsel/r0r1 = O22/irsel/r0r1;     # _ir06 =  IR[06]
    jumper I35/irsel/r0r1 = O34/irsel/r0r1;     # _ir08 =  IR[08]
    jumper I38/irsel/r0r1 = O37/irsel/r0r1;     # _ir09 =  IR[09]
    jumper I44/irsel/r0r1 = O43/irsel/r0r1;     # _ir11 =  IR[11]
    jumper I47/irsel/r0r1 = O46/irsel/r0r1;     # _ir12 =  IR[12]
    jumper I47/ctsa/r0r1  = 0;                  # weodd  = gnd
    jumper I50/ctsa/r0r1  = 0;                  # reaodd = gnd

    # jumpers for r2r3 board
    jumper I20/irsel/r2r3 = O21/irsel/r2r3;     # _ir05 = _IR[05]
    jumper I23/irsel/r2r3 = O22/irsel/r2r3;     # _ir06 =  IR[06]
    jumper I35/irsel/r2r3 = O36/irsel/r2r3;     # _ir08 = _IR[08]
    jumper I38/irsel/r2r3 = O37/irsel/r2r3;     # _ir09 =  IR[09]
    jumper I44/irsel/r2r3 = O45/irsel/r2r3;     # _ir11 = _IR[11]
    jumper I47/irsel/r2r3 = O46/irsel/r2r3;     # _ir12 =  IR[12]
    jumper I47/ctsa/r2r3  = 0;                  # weodd  = gnd
    jumper I50/ctsa/r2r3  = 0;                  # reaodd = gnd

    # jumpers for r4r5 board
    jumper I20/irsel/r4r5 = O19/irsel/r4r5;     # _ir05 =  IR[05]
    jumper I23/irsel/r4r5 = O24/irsel/r4r5;     # _ir06 = _IR[06]
    jumper I35/irsel/r4r5 = O34/irsel/r4r5;     # _ir08 =  IR[08]
    jumper I38/irsel/r4r5 = O39/irsel/r4r5;     # _ir09 = _IR[09]
    jumper I44/irsel/r4r5 = O43/irsel/r4r5;     # _ir11 =  IR[11]
    jumper I47/irsel/r4r5 = O48/irsel/r4r5;     # _ir12 = _IR[12]
    jumper I47/ctsa/r4r5  = 0;                  # weodd  = gnd
    jumper I50/ctsa/r4r5  = 0;                  # reaodd = gnd

    # jumpers for r6r7 board
    jumper I20/irsel/r6r7 = O21/irsel/r6r7;     # _ir05 = _IR[05]
    jumper I23/irsel/r6r7 = O24/irsel/r6r7;     # _ir06 = _IR[06]
    jumper I35/irsel/r6r7 = O36/irsel/r6r7;     # _ir08 = _IR[08]
    jumper I38/irsel/r6r7 = O39/irsel/r6r7;     # _ir09 = _IR[09]
    jumper I44/irsel/r6r7 = O45/irsel/r6r7;     # _ir11 = _IR[11]
    jumper I47/irsel/r6r7 = O48/irsel/r6r7;     # _ir12 = _IR[12]
    jumper I47/ctsa/r6r7  = O46/ctsa/r6r7;      # weodd  = we7
    jumper I50/ctsa/r6r7  = O49/ctsa/r6r7;      # reaodd = rea7
}
