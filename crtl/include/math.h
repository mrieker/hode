//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html
#ifndef _MATH_H
#define _MATH_H

#define M_PI 3.14159265358979323846
#define M_NAN (0.0/0.0)

int isfinite (double x);
int isinf (double x);
int isnan (double x);
double atan (double x);
double atan2 (double y, double x);
double sqrt (double x);

#define M_PIF ((float)3.14159265358979323846)
#define M_NANF ((float)(0.0/0.0))

int isfinitef (float x);
int isinff (float x);
int isnanf (float x);
float atanf (float x);
float atan2f (float y, float x);
float sqrtf (float x);

#endif
