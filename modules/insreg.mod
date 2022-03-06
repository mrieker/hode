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

# instruction register
#  input:
#    MQ = instruction from memory
#    WE = load register at end of cycle
module insreg (out Q[15:00], out _Q[15:00], in MQ[15:00], in _WE)
{
    wire g_a, g_b, g_c, g_d;

    # WE is direct output of state flipflop so just use a latch
    # WE drops at end of FETCH_2 and so holds the data

    g_a = ~_WE;
    g_b = ~_WE;
    g_c = ~_WE;
    g_d = ~_WE;

    ir1512: DLat 4 (Q:Q[15:12], _Q:_Q[15:12], D:MQ[15:12], G:g_a);
    ir1108: DLat 4 (Q:Q[11:08], _Q:_Q[11:08], D:MQ[11:08], G:g_b);
    ir0704: DLat 4 (Q:Q[07:04], _Q:_Q[07:04], D:MQ[07:04], G:g_c);
    ir0300: DLat 4 (Q:Q[03:00], _Q:_Q[03:00], D:MQ[03:00], G:g_d);
}
