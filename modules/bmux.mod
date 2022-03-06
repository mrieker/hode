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

# inputs:
#  CON_4 = put constant '-4' on B bus
#  CON2 = put constant '2' on B bus
#  CON2 & CON_4 = put constant '-2' on B bus
#  CON0 = put constant '0' on B bus
#  IR = contents of instruction register
#  IRBROF = put sign-extended branch offset on B bus
#  IRLSOF = put sign-extended load/store offset on B bus
# outputs:
#  BBUS = value output to B bus (open collector)

module bmux (out BBUS[15:00], in CON_4, in CON2, in CON0, in _IR[15:00], in IRBROF, in IRLSOF)
{
    wire _bbus_15_09, con0or2, con0or2orirlsof_08_06, _con_4, irof;

    _con_4 = ~CON_4;
    con0or2 = CON0 | CON2 & _con_4;
    con0or2orirlsof_08_06 = CON0 | CON2 & _con_4 | IRLSOF & _IR[6];
    irof = IRBROF | IRLSOF;
    _bbus_15_09 = con0or2 | IRLSOF & _IR[6] | IRBROF & _IR[9];

    BBUS[15] = oc ~_bbus_15_09;
    BBUS[14] = oc ~_bbus_15_09;
    BBUS[13] = oc ~_bbus_15_09;
    BBUS[12] = oc ~_bbus_15_09;
    BBUS[11] = oc ~_bbus_15_09;
    BBUS[10] = oc ~_bbus_15_09;
    BBUS[09] = oc ~_bbus_15_09;

    BBUS[08] = oc ~(con0or2orirlsof_08_06 | IRBROF & _IR[8]);
    BBUS[07] = oc ~(con0or2orirlsof_08_06 | IRBROF & _IR[7]);
    BBUS[06] = oc ~(con0or2orirlsof_08_06 | IRBROF & _IR[6]);
    BBUS[05] = oc ~(con0or2               | irof   & _IR[5]);
    BBUS[04] = oc ~(con0or2               | irof   & _IR[4]);
    BBUS[03] = oc ~(con0or2               | irof   & _IR[3]);
    BBUS[02] = oc ~(con0or2               | irof   & _IR[2]);
    BBUS[01] = oc ~(CON0 | ~CON2 & CON_4  | irof   & _IR[1]);
    BBUS[00] = oc ~(con0or2 | CON_4       | IRLSOF & _IR[0] | IRBROF);
}
