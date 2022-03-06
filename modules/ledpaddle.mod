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

# make LED paddle board that will work on any A,C,I,D connector

module ledpaddle (out xled[15:00], out yled[15:00])
{
    wire xbus[15:00], ybus[15:00];

    xybus: Conn
            ii ii gi ii gi ii gi ii gi ii
            ii gi ii gi ii gi ii gi ii ii (
                    xbus[00], ybus[00],
                    xbus[01], ybus[01],
                    xbus[02], ybus[02],
                    xbus[03], ybus[03],
                    xbus[04], ybus[04],
                    xbus[05], ybus[05],
                    xbus[06], ybus[06],
                    xbus[07], ybus[07],
                    xbus[08], ybus[08],
                    xbus[09], ybus[09],
                    xbus[10], ybus[10],
                    xbus[11], ybus[11],
                    xbus[12], ybus[12],
                    xbus[13], ybus[13],
                    xbus[14], ybus[14],
                    xbus[15], ybus[15]);

    xled[00] = led:X00 ~ xbus[00];
    xled[01] = led:X01 ~ xbus[01];
    xled[02] = led:X02 ~ xbus[02];
    xled[03] = led:X03 ~ xbus[03];
    xled[04] = led:X04 ~ xbus[04];
    xled[05] = led:X05 ~ xbus[05];
    xled[06] = led:X06 ~ xbus[06];
    xled[07] = led:X07 ~ xbus[07];
    xled[08] = led:X08 ~ xbus[08];
    xled[09] = led:X09 ~ xbus[09];
    xled[10] = led:X10 ~ xbus[10];
    xled[11] = led:X11 ~ xbus[11];
    xled[12] = led:X12 ~ xbus[12];
    xled[13] = led:X13 ~ xbus[13];
    xled[14] = led:X14 ~ xbus[14];
    xled[15] = led:X15 ~ xbus[15];

    yled[00] = led:Y00 ~ ybus[00];
    yled[01] = led:Y01 ~ ybus[01];
    yled[02] = led:Y02 ~ ybus[02];
    yled[03] = led:Y03 ~ ybus[03];
    yled[04] = led:Y04 ~ ybus[04];
    yled[05] = led:Y05 ~ ybus[05];
    yled[06] = led:Y06 ~ ybus[06];
    yled[07] = led:Y07 ~ ybus[07];
    yled[08] = led:Y08 ~ ybus[08];
    yled[09] = led:Y09 ~ ybus[09];
    yled[10] = led:Y10 ~ ybus[10];
    yled[11] = led:Y11 ~ ybus[11];
    yled[12] = led:Y12 ~ ybus[12];
    yled[13] = led:Y13 ~ ybus[13];
    yled[14] = led:Y14 ~ ybus[14];
    yled[15] = led:Y15 ~ ybus[15];

    pwrin:  Conn vg ();
    pwrout: Conn vg ();
}
