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
module pb2board ()
{
    power: Conn pwrcap v v v g g g ();

    wire tffad, tffat, tffa_pc, tffa_ps, tffaq, tffa_q;
    tffaff: DFF led1 (D:tffad, T:tffat, _PC:tffa_pc, _PS:tffa_ps, Q:tffaq, _Q:tffa_q);
    tffaio: Conn iiiioo (tffad, tffat, tffa_pc, tffa_ps, tffaq, tffa_q);

    wire tffbd, tffbt, tffb_pc, tffb_ps, tffbq, tffb_q;
    tffbff: DFF led0 (D:tffbd, T:tffbt, _PC:tffb_pc, _PS:tffb_ps, Q:tffbq, _Q:tffb_q);
    tffbio: Conn iiiioo (tffbd, tffbt, tffb_pc, tffb_ps, tffbq, tffb_q);

    wire tffcd, tffct, tffcq, tffc_q;
    tffcff: DFF mini (D:tffcd, T:tffct, _PC:1, _PS:1, Q:tffcq, _Q:tffc_q);
    tffcio: Conn iioo (tffcd, tffct, tffcq, tffc_q);

    wire tffdd, tffdt, tffdq, tffd_q;
    tffdff: DFF mini (D:tffdd, T:tffdt, _PC:1, _PS:1, Q:tffdq, _Q:tffd_q);
    tffdio: Conn iioo (tffdd, tffdt, tffdq, tffd_q);

    wire nand0a, nand0b, nand0c, nand0x;
    nand0x = ~(nand0a & nand0b & nand0c);
    nand0io: Conn oiii (nand0x, nand0a, nand0b, nand0c);

    wire aoi1aa, aoi1ab, aoi1ac;
    wire aoi1ba, aoi1bb, aoi1bc;
    wire aoi1ca, aoi1cb, aoi1cc;
    wire aoi1x;
    aoi1x = ~(
            aoi1aa & aoi1ab & aoi1ac |
            aoi1ba & aoi1bb & aoi1bc |
            aoi1ca & aoi1cb & aoi1cc);
    aoi1io: Conn oniiiniiiniii (
            aoi1x,
            aoi1aa, aoi1ab, aoi1ac,
            aoi1ba, aoi1bb, aoi1bc,
            aoi1ca, aoi1cb, aoi1cc);

    wire nor1a, nor1b, nor1c, nor1x;
    nor1x = ~(nor1a | nor1b | nor1c);
    nor1io: Conn oiii (nor1x, nor1a, nor1b, nor1c);

    wire nand1a, nand1b, nand1c, nand1x;
    nand1x = ~(nand1a & nand1b & nand1c);
    nand1io: Conn oiii (nand1x, nand1a, nand1b, nand1c);

    wire nand2a, nand2b, nand2c, nand2x;
    nand2x = ~(nand2a & nand2b & nand2c);
    nand2io: Conn oiii (nand2x, nand2a, nand2b, nand2c);

    wire nand3a, nand3b, nand3c, nand3x;
    nand3x = ~(nand3a & nand3b & nand3c);
    nand3io: Conn oiii (nand3x, nand3a, nand3b, nand3c);

    wire clk2, irq, reset, mq[3:0], md[3:0], mword, mread, mwrite, halt;

    raspi: RasPi test4 (
            CLK:clk2,
            IRQ:irq,
            RES:reset,
            MQ:mq,
            MD:md,
            MWORD:mword,
            MREAD:mread,
            MWRITE:mwrite,
            HALT:halt);

    rascm: Conn iiii (mword, mread, mwrite, halt);

    rasst: Conn ooo  (clk2, reset, irq);

    rasmd: Conn iiii (md[0], md[1], md[2], md[3]);

    rasmq: Conn oooo (mq[0], mq[1], mq[2], mq[3]);
}
