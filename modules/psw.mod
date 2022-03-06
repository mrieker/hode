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

# internal register module
#  inputs:
#   D[15:0] = data to write if WE set
#   IECLR = clear interupt enable immediately
#   ALUN,Z,V,C = as computed by ALU
#   ALUWE = write ALUN,Z,V,C at end of cycle into flipflops
#   WE = write all flipflops from D[15:0] at end of cycle
#   _RE = gate all flipflops onto QOC[15:0] for this cycle
#   CLK = system clock indicating when to write
#   IR[15:0] = instruction register contents
#  outputs:
#   Q = output onto bus enabled by _RE
#   IE = interrupts enabled
#   NZVC = latched condition codes
#   BT = latched condition code + IR contents says branch condition is true (assuming it is a branch)
module pswreg (out Q15, out Q30[3:0], out IE, out N, out Z, out V, out C, out BT,
    in D[15:0], in _IECLR, in ALUN, in ALUZ, in ALUV, in ALUC, in ALUWE, in WE, in _RE, in CLK2, in IR[15:0], in _IR[15:0])
{
    wire _ie, _n, _z, _v, _c;
    wire _ied, _nd, _zd, _vd, _cd;
    wire nv, cchold;
    wire re_a;
    wire _clk1, clk0, _btbase;
    wire ir10, ir11, ir12, _ir10, _ir11, _ir12;

    re_a = ~_RE;

    _clk1 = ~CLK2;
    clk0 = ~_clk1;

    ir10 = ~_IR[10];
    ir11 = ~_IR[11];
    ir12 = ~_IR[12];
    _ir10 = ~IR[10];
    _ir11 = ~IR[11];
    _ir12 = ~IR[12];

    Q15    = oc ~(re_a & _ie);  # interrupts enabled
    Q30[3] = oc ~(re_a &  _n);  # result negative
    Q30[2] = oc ~(re_a &  _z);  # result zero
    Q30[1] = oc ~(re_a &  _v);  # result signed overflow
    Q30[0] = oc ~(re_a &  _c);  # result unsigned overflow (or A < B unsigned)

    cchold = ~(ALUWE | WE);     # cond code do not change if not being written

    # interupts enabled
    _ied = ~(WE & D[15] | ~WE & IE);
    ieff: DFF (_Q: IE, Q: _ie, D: _ied, _PC:1, _PS:_IECLR, T: clk0);

    # result was negative
    _nd = ~(ALUWE & ALUN | WE & D[3] | cchold & N);
    nff:  DFF miniflat (_Q: N, Q: _n, D: _nd, _PC:1, _PS:1, T: clk0);

    # result was zero
    _zd = ~(ALUWE & ALUZ | WE & D[2] | cchold & Z);
    zff:  DFF miniflat (_Q: Z, Q: _z, D: _zd, _PC:1, _PS:1, T: clk0);

    # signed result overflowed
    _vd = ~(ALUWE & ALUV | WE & D[1] | cchold & V);
    vff:  DFF miniflat (_Q: V, Q: _v, D: _vd, _PC:1, _PS:1, T: clk0);

    # unsigned result overflowed
    _cd = ~(ALUWE & ALUC | WE & D[0] | cchold & C);
    cff:  DFF miniflat (_Q: C, Q: _c, D: _cd, _PC:1, _PS:1, T: clk0);

    # determine if branch condition in IR[12:10],IR[0] is true based on NZVC latched in flipflops
    nvxor: xor1 (nv, N, V);                         # A <  B signed

    _btbase = ~(                                    # 000 : never
        (_ir12 & ir11 & nv) |                       # 01x : BLT/BLE
        (ir12 & _ir11 & C)  |                       # 10x : BLO/BLOS
        (~(ir12 & ir11) & ir10 & Z) |               # 001,011,101 : BEQ,BLE,BLOS
        (ir12 & ir11 & _ir10 & N) |                 # 110 : BMI
        (ir12 & ir11 & ir10 & V));                  # 111 : BVS

    btxor: xor1 (BT, _IR[0], _btbase);              # flip if IR[0] set
}

# external module
module psw (
        out bbus15,         # 'B' operand to ALU
        out bbus30[3:0],
        out psw_ie,         # psw interrupt enable bit as saved in flipflops
        out psw_bt,         # psw condition codes match branch condition in instruction register
        out rawcin,         # carry bit going into bottom of ALU
        out rawshrin,       # shift right bit going into top of ALU
        in irbus[15:00],    # positive instruction register
        in _irbus[15:00],   # negative instruction register
        in dbus[15:00],     # result coming out of ALU
        in _psw_ieclr,      # clear interrupt enable flipflop immediately
        in psw_aluwe,       # write psw condition codes at end of cycle
        in psw_we,          # write whole psw register at end of cycle
        in _psw_reb,        # gate psw register contents out to bbus
        in rawcout,         # carry bit out of top of ALU
        in rawvout,         # carry bit out of next to top of ALU
        in rawshrout,       # carry bit out of bottom of ALU
        in asign,           # sign bit of 'A' operand going into ALU
        in clk2,            # positive edge indicates end of cycle
        in irfc)            # performing function indicated in irbus[]
{
    wire alun, aluz, aluv, aluc, pswc;
    wire bnot, cookedcout, cookedvout;
    wire irbus0, irbus1, irbus2, irbus3, _irbus2, _irbus3;

    irbus0 = ~_irbus[0];
    irbus1 = ~_irbus[1];
    irbus2 = ~_irbus[2];
    irbus3 = ~_irbus[3];
    _irbus2 = ~irbus[2];
    _irbus3 = ~irbus[3];

    # this bit gets shifted into top when the op is a right shift
    #  irbus[1:0] = 0: LSR: rawshrin = 0
    #  irbus[1:0] = 1: ASR: rawshrin = sign bit going into ALU
    #  irbus[1:0] = 2: CSR: rawshrin = C-bit as saved in PSW
    rawshrin = asign & irbus0 | pswc & irbus1;

    # B-operand to ALU is being complemented (so we will complement the carry bits too)
    #  1: complements for NEG,COM,SUB,SBB (ie, C bit is a 'borrow' bit)
    #      rawcout,dbus = abus + ~ bbus + rawcin
    #  0: normal for all others (ie, C bit is a 'carry' bit)
    #      rawcout,dbus = abus +   bbus + rawcin
    bnot = irfc & irbus2 & irbus0;

    # carry bit going into bottom of ALU adder
    #  LSR  0:   0  (but not used)
    #  ASR  1:   0  (but not used)
    #  CSR  2:   0  (but not used)
    #       3:   0  (but not used)
    #  MOV  4:   0
    #  NEG  5:  ~0
    #  INC  6:   1
    #  COM  7:  ~1
    #  OR   8:   0  (but not used)
    #  AND  9:   0  (but not used)
    #  XOR 10:   0  (but not used)
    #      11:   0  (but not used)
    #  ADD 12:   0
    #  SUB 13:  ~0
    #  ADC 14:   c  (c = C bit as previously in psw)
    #  SBB 15:  ~c  (c = C bit as previously in psw)
    rcixor: xor1 (X:rawcin, A:bnot, B:irfc & irbus2 & irbus1 & (pswc | _irbus3));

    # cookedcout = ADDs: same as rawcout
    #              SUBs: ~rawcout (so it is a borrow)
    ccoxor: xor1 (X:cookedcout, A:bnot, B:rawcout);

    # cookedvout = indicates signed overflow for add or subtract
    cvoxor: xor1 (X:cookedvout, A:rawcout, B:rawvout);

    # N and Z bits come directly from whatever ALU is outputting for a value
    alun = dbus[15];
    aluz = ~(dbus[15] | dbus[14] | dbus[13] | dbus[12] | dbus[11] | dbus[10] | dbus[09] | dbus[08] |
             dbus[07] | dbus[06] | dbus[05] | dbus[04] | dbus[03] | dbus[02] | dbus[01] | dbus[00]);

    # V bit comes from carry out of bit 14 ^ carry out of bit 15
    aluv = irbus2 & cookedvout;                         # MOV,NEG,COM,ADD,SUB,ADC,SBB
                                                        # all others 0

    # C bit:
    #  LSR,ASR,CSR : abus (as going into ALU) bit 0
    #  MOV,NEG,INC,COM,OR,AND,XOR : unchanged
    #  ADD,ADC : carry out of bit 15
    #  SUB,SBB : flipped carry out of bit 15
    aluc = ~(~(_irbus3 & _irbus2 & rawshrout) &     # LSR,ASR,CSR
             ~(_irbus3 &  irbus2 & pswc)      &     # MOV,NEG,INC,COM
             ~( irbus3 & _irbus2 & pswc)      &     # OR,AND,XOR
             ~( irbus3 &  irbus2 & cookedcout));    # ADD,SUB,ADC,SBB

    # the psw register itself
    pswreg: pswreg (Q15:bbus15, Q30:bbus30, IE:psw_ie, BT:psw_bt, D:dbus, _IECLR:_psw_ieclr, C:pswc,
        ALUN:alun, ALUZ:aluz, ALUV:aluv, ALUC:aluc, ALUWE:psw_aluwe,
        WE:psw_we, _RE:_psw_reb, CLK2:clk2, IR:irbus, _IR:_irbus);
}
