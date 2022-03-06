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

# internal module to add a single bit
#  input:
#   AENA = 0: use 0 for A operand; else use A for A operand
#   BNOT = 0: use B as is for B operand; else use ~B for B operand
#   A = A operand
#   B = B operand
#   CIN = carry input
#  output:
#   AORB  = (A & AENA) | (B ^ BNOT)
#   AXORB = (A & AENA) ^ (B ^ BNOT)
#   AANDB = (A & AENA) & (B ^ BNOT)
#   SUM   = (A & AENA) ^ (B ^ BNOT) ^ CIN
#   COUT  = (A & AENA) & (B ^ BNOT) | (((A & AENA) | (B ^ BNOT)) & CIN)  [ (a & b) | ((a | b) & c) ]

# IR[3:0]                              gcin      COUT   AENA

#   0000    LSR     d =   a >>> 1                a<0>
#   0001    ASR     d =   a >>  1                a<0>
#   0010    CSR     d = c:a >>> 1                a<0>
#   0011                                         a<0>

#   0100    MOV     d = 0 + b            0       cin      0     BONLY
#   0101    NEG     d = 0 - b            1       cin      0
#   0110    INC     d = 0 + b + 1        1       cin      0
#   0111    COM     d = 0 - b - 1        0       cin      0

#   1000    OR      d = a | b                    cin
#   1001    AND     d = a & b                    cin
#   1010    XOR     d = a ^ b                    cin
#   1011                                         cin

#   1100    ADD     d = a + b            0       cout     1     ADD
#   1101    SUB     d = a - b            0      ~cout     1
#   1110    ADC     d = a + b + c       CIN      cout     1
#   1111    SBB     d = a - b - c       CIN     ~cout     1


# external ALU module (combinational only)
#  input:
#   A = 'A' operand
#   B = 'B' operand
#   SHR = next significant 'A' bit
#   cin = carry input
#   bnot = complements B input to adder, else use B
#   _aena = enables A input to adder, else use 0
#    doaddsub = use adder output    (A & AENA) ^ (B ^ BNOT) ^ CIN
#    doingior = use OR output       (A & AENA) | (B ^ BNOT)
#    doingand = use AND output      (A & AENA) & (B ^ BNOT)
#    doingxor = use XOR output      (A & AENA) ^ (B ^ BNOT)
#    doingshr = use SHR input       SHR
#  output:
#   q = output
#   aorb = or output
#   aandb = and output
#   cout = carry output             (A & AENA) & (B ^ BNOT) | (((A & AENA) | (B ^ BNOT)) & CIN)
module aluone (out q, out aorb, out aandb, out cout, in a, in b, in shr, in cin, in bnot, in _aena,
                in doaddsub, in doingior, in doingand, in doingxor, in doingshr)
{
    wire axorb, sum;
    wire ena;
    wire nandedb, nottedb;
    wire anandnottedb, axornottedb;
    wire sumnand;

    # check for A enabled, if not use 0
    #  'a' is open-collector
    #  '~_aena' will get wire-anded with 'a' and provide a pull-up resistor
    ena = a;
    ena = ~_aena;

    # maybe flip B, ie, nottedb = BNOT xor B
    nandedb = ~(bnot & b);
    nottedb = ~( ~(bnot & nandedb) & ~(nandedb & b) );

    # nothing sneaky for A or B
    aorb = ena | nottedb;

    # compute axornottedb = a xor nottedb
    anandnottedb = ~(ena & nottedb);
    axornottedb  = ~( ~(ena & anandnottedb) & ~(anandnottedb & nottedb) );

    # use that to generate AND and XOR outputs
    aandb   = ~ anandnottedb;
    axorb   =   axornottedb;

    # compute SUM = axornottedb xor CIN
    sumnand = ~(axornottedb & cin);
    sum     = ~( ~(axornottedb & sumnand) & ~(sumnand & cin) );

    # compute COUT = A & nottedb | (a | nottedb) & CIN
    cout    = aandb | aorb & cin;

    # select which one based on 'doing's
    q = doaddsub & sum   |
        doingand & aandb |
        doingior & aorb  |
        doingxor & axorb |
        doingshr & shr;
}

# 8-bit ALU
module alueight (out S[7:0], out COUT, out VOUT,
                in A[7:0], in B[7:0], in SHR, in CIN,
                in IR[3:0], in _IR[3:0],
                in IRFC, in ADD, in BONLY)
{
    wire _bnot, bnot_a, bnot_b;                     # complements B input to adder, else use B
    wire _aena;                                     # enables A input to adder, else use 0
    wire doaddsub, doingand, doingior, doingshr, doingxor;
    wire cout0, cout1, cout2, cin3, cout3, cout4, cout5, cin6, cout6, cout7;
    wire _bonly, irfc, _irfc;
    wire ir2, ir3, _ir2, _ir3;
    wire aorb0, aorb1, aorb2, aorb3, aorb4, aorb5, aorb6, aorb7;
    wire aandb0, aandb1, aandb2, aandb3, aandb4, aandb5, aandb6, aandb7;

    ir2 = ~_IR[2];
    ir3 = ~_IR[3];
    _ir2 = ~IR[2];
    _ir3 = ~IR[3];

    _irfc  = ~IRFC;
    irfc   = ~_irfc;
    _bonly = ~BONLY;
    _aena  = ~(ADD | ir3 & _bonly | doingshr);                  # LSR,ASR,CSR,OR,AND,XOR,ADD,SUB,ADC,SBB
    _bnot  = ~(IR[0] & irfc & ir2);                             # NEG,COM,SUB,SBB
    bnot_a = ~_bnot;
    bnot_b = ~_bnot;
    doaddsub = irfc & ir2 | ADD;                                # MOV,NEG,INC,COM,ADD,SUB,ADC,SBB
    doingior = irfc & ir3 & _ir2 & _IR[1] & _IR[0] | BONLY;     # OR
    doingand = irfc & ir3 & _ir2 & _IR[1] &  IR[0];             # AND
    doingxor = irfc & ir3 & _ir2 &  IR[1] & _IR[0];             # XOR
    doingshr = irfc & _ir3 & _ir2;                              # LSR,ASR,CSR

    alubit0: aluone (q:S[0], aorb:aorb0, aandb:aandb0, cout:cout0, a:A[0], b:B[0], shr:A[1], cin:CIN,   bnot:bnot_a, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);
    alubit1: aluone (q:S[1], aorb:aorb1, aandb:aandb1, cout:cout1, a:A[1], b:B[1], shr:A[2], cin:cout0, bnot:bnot_a, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);
    alubit2: aluone (q:S[2], aorb:aorb2, aandb:aandb2, cout:cout2, a:A[2], b:B[2], shr:A[3], cin:cout1, bnot:bnot_a, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);

    cin3 = aandb2 | aorb2 & aandb1 | aorb2 & aorb1 & aandb0 | aorb2 & aorb1 & aorb0 & CIN;

    alubit3: aluone (q:S[3], aorb:aorb3, aandb:aandb3, cout:cout3, a:A[3], b:B[3], shr:A[4], cin:cin3,  bnot:bnot_a, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);
    alubit4: aluone (q:S[4], aorb:aorb4, aandb:aandb4, cout:cout4, a:A[4], b:B[4], shr:A[5], cin:cout3, bnot:bnot_b, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);
    alubit5: aluone (q:S[5], aorb:aorb5, aandb:aandb5, cout:cout5, a:A[5], b:B[5], shr:A[6], cin:cout4, bnot:bnot_b, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);

    cin6 = aandb5 | aorb5 & aandb4 | aorb5 & aorb4 & aandb3 | aorb5 & aorb4 & aorb3 & cin3;

    alubit6: aluone (q:S[6], aorb:aorb6, aandb:aandb6, cout:cout6, a:A[6], b:B[6], shr:A[7], cin:cin6,  bnot:bnot_b, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);
    alubit7: aluone (q:S[7], aorb:aorb7, aandb:aandb7, cout:cout7, a:A[7], b:B[7], shr:SHR,  cin:cout6, bnot:bnot_b, _aena:_aena,
                    doaddsub:doaddsub, doingior:doingior, doingand:doingand, doingxor:doingxor, doingshr:doingshr);

    VOUT = cout6;
    COUT = aandb7 | aorb7 & aandb6 | aorb7 & aorb6 & cin6;
}

# 8-bit ALU board
#       slice       15:08       07:00         : which of the two boards this one is
#       ain     abus[15:08]  abus[07:00]      : which bits of abus[] this board reads
#       bin     bbus[15:08]  bbus[07:00]      : which bits of bbus[] this board reads
#       dout    dbus[15:08]  dbus[07:00]      : which bits of dbus[] this board writes
#       shr         shr16      ain[08]        : where this board gets its shift right input bit from
#       cin         cin08       cin00         : where this board gets its raw carry input bit from
#       cout        cin16       cin08         : where this board puts its raw carry output bit to
#       vout        vout         nc           : where this board puts its raw overflow output bit to
#
# jumpers on physical board:
#   all to the outside for low byte
#   all to the inside for high byte
module aluboard ()
{
    wire abus[15:00], bbus[15:00];  # operands from gprs going to ALU
    wire dbus[15:00];               # dbus: result from ALU (going to gprs and memory)
    wire ir[15:00], _ir[15:00];     # ir: coming from instruction register
    wire irfc, add, bonly;          # control signals coming from sequencer

    wire ain[7:0], bin[7:0], dout[7:0], shrin, cin, cout, vout;
    wire rawcin, rawcmid, rawcout, rawvout;
    wire rawshrin, rawshrmid, rawshrout;

    abbus: Conn
                        ii ii gi ii gi ii gi ii gi ii
                        ii gi ii gi ii gi ii gi ii ii (
                            abus[00], bbus[00], abus[08], bbus[08],
                            abus[01], bbus[01], abus[09], bbus[09],
                            abus[02], bbus[02], abus[10], bbus[10],
                            abus[03], bbus[03], abus[11], bbus[11],
                            abus[04], bbus[04], abus[12], bbus[12],
                            abus[05], bbus[05], abus[13], bbus[13],
                            abus[06], bbus[06], abus[14], bbus[14],
                            abus[07], bbus[07], abus[15], bbus[15]);

    absel: Conn
                        nnn
                        oio    # abus00  abus08
                        oio    # bbus00  bbus08
                        oio    # abus01  abus09
                        oio    # bbus01  bbus09
                        oio    # abus02  abus10
                        oio    # bbus02  bbus10
                        oio    # abus03  abus11
                        oio    # bbus03  bbus11
                        nnn
                        nnn
                        oio    # abus04  abus12
                        oio    # bbus04  bbus12
                        oio    # abus05  abus13
                        oio    # bbus05  bbus13
                        oio    # abus06  abus14
                        oio    # bbus06  bbus14
                        oio    # abus07  abus15
                        oio    # bbus07  bbus15
                        nnn (
                            abus[00], ain[0], abus[08],
                            bbus[00], bin[0], bbus[08],
                            abus[01], ain[1], abus[09],
                            bbus[01], bin[1], bbus[09],
                            abus[02], ain[2], abus[10],
                            bbus[02], bin[2], bbus[10],
                            abus[03], ain[3], abus[11],
                            bbus[03], bin[3], bbus[11],
                            abus[04], ain[4], abus[12],
                            bbus[04], bin[4], bbus[12],
                            abus[05], ain[5], abus[13],
                            bbus[05], bin[5], bbus[13],
                            abus[06], ain[6], abus[14],
                            bbus[06], bin[6], bbus[14],
                            abus[07], ain[7], abus[15],
                            bbus[07], bin[7], bbus[15]);

    irbus: Conn
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

    dbus: Conn
                      oo oo go oo go oo go oo go oo
                      ii go oo gi ii go oi gi ii ii (
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
                            O24:rawcmid,    # carry outof alu[07]
                            O25:rawcout,    # carry outof alu[15]
                            O26:rawvout,    # carry outof alu[14]
                            I28:rawshrmid,  # shift right into alu[07]
                            I29:rawshrin,   # shift right into alu[15]
                            O32:rawshrout,  # shift right outof alu[00]
                            O33:rawshrmid); # shift right outof alu[08]

    dsel: Conn
                        nnn ioi ioi ioi ioi ioi ioi ioi ioi nnn

                        nnn     # lobyte <<  **  >> hibyte
                        oio     # rawcin    cin     rawcmid     carry going into bottom of ALU
                        ioi     # rawcmid   cout    rawcout     carry coming outof top of ALU
                        noi     #           vout    rawvout     overflow coming outof next to top of ALU
                        oio     # rawshrmid shrin   rawshrin    shift right going into top of ALU
                        ioi     # rawshrout shrout  rawshrmid   shift right coming outof bottom of ALU
                        nnn
                        nnn
                        nnn
                        nnn (
                             I4:dbus[07],  O5:dout[7],  I6:dbus[15],
                             I7:dbus[06],  O8:dout[6],  I9:dbus[14],
                            I10:dbus[05], O11:dout[5], I12:dbus[13],
                            I13:dbus[04], O14:dout[4], I15:dbus[12],
                            I16:dbus[03], O17:dout[3], I18:dbus[11],
                            I19:dbus[02], O20:dout[2], I21:dbus[10],
                            I22:dbus[01], O23:dout[1], I24:dbus[09],
                            I25:dbus[00], O26:dout[0], I27:dbus[08],

                            O34:rawcin,     I35:cin,    O36:rawcmid,
                            I37:rawcmid,    O38:cout,   I39:rawcout,
                                            O41:vout,   I42:rawvout,
                            O43:rawshrmid,  I44:shrin,  O45:rawshrin,
                            I46:rawshrout,  O47:ain[0], I48:rawshrmid);

    ctla: Conn
                      ii ii gi ii gi ii gi ii gi ii
                      ii gi ii gi ii gi ii gi ii ii (
                            I30:add,
                            I11:irfc,
                            I24:bonly);

    power: Conn pwrcap v v v g g g ();

    bpu: PullUp 8 (bin);

    alueight: alueight (S:dout, COUT:cout, VOUT:vout,
                    A:ain, B:bin, SHR:shrin, CIN:cin,
                    IR:ir[3:0], _IR:_ir[3:0],
                    IRFC:irfc, ADD:add, BONLY:bonly);
}
