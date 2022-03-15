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
#define M_INF (1.0/0.0)

bool isfinite (double x);
bool isinf (double x);
bool isnan (double x);

double acos (double x);
double asin (double x);
double atan (double x);
double atan2 (double y, double x);
double cos (double x);
double exp (double x);
double fabs (double x);
double log (double x);
double pow (double x, double y);
double sin (double x);
double sqrt (double x);
double tan (double x);

#define M_PIF ((float)3.14159265358979323846)
#define M_NANF ((float)(0.0/0.0))
#define M_INFF ((float)(1.0/0.0))

bool isfinitef (float x);
bool isinff (float x);
bool isnanf (float x);

float acosf (float x);
float asinf (float x);
float atanf (float x);
float atan2f (float y, float x);
float cosf (float x);
float expf (float x);
float fabsf (float x);
float logf (float x);
float powf (float x, float y);
float sinf (float x);
float sqrtf (float x);
float tanf (float x);

#endif
