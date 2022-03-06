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
#  D = data from memory
#  WORD = word mode, pass all 16 bits through
#  SEXT = sign extend mode, sign extend D[7] to QOC[15:8]
#         otherwise, pass zeroes to Q[15:8]
#         (ignored if WORD set)
#  RE = enable outputs
# output:
#  Q = value written to B bus (open collector)
module memread (out Q[15:00], in D[15:00], in WORD, in SEXT, in RE)
{
    wire _re, _signextend;

    _re = ~RE;  # fanin buffois

    # low byte: gate unaltered D to Q when enabled
    Q[07:00] = oc ~(~_re & ~D[7:0]);

    # sign extend bit but not doing WORD and sign bit D[7] is set
    _signextend = ~(SEXT & ~WORD & D[7]);

    # high byte: unaltered D if WORD mode
    #            zeroes for unsigned byte
    #            sign-extend bit for signed byte
    #   RE low = ~_re low: force outputs to ones
    #   signextend high = _signextend low: force outputs to ones
    #   else use high byte data anded with WORD
    Q[15:08] = oc ~(~_re & _signextend & ~(WORD & D[15:8]));
}
