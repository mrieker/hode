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
//+++2003-11-18
//    Copyright (C) 2001,2002,2003  Mike Rieker, Beverly, MA USA
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
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//---2003-11-18

#ifndef _VXPRINTF_H
#define _VXPRINTF_H

#include <stdio.h>

typedef  __int8_t   Byte;
typedef  __int16_t  Word;
typedef  __int32_t  Long;
typedef  __int64_t  Quad;

typedef __uint8_t  uByte;
typedef __uint16_t uWord;
typedef __uint32_t uLong;
typedef __uint64_t uQuad;

#define Par     vxprintf_Par
#define putint  vxprintf_putint
#define putsign vxprintf_putsign
#define putfc   vxprintf_putfc
#define putst   vxprintf_putst
#define putch   vxprintf_putch

/* Get unsigned short/long/int into 'unum' */

#define GETUNSIGNEDNUM vxprintf_getunum (p)

/* Get short/long/int into '(u/s)num', put the sign in 'negative' */

#define GETSIGNEDNUM vxprintf_getsnum (p)

/* Get floating point into fnum and expon:  fnum normalised in range 1.0 (inclusive) to 10.0 (exclusive) such that original_value = fnum * 10 ** expon */

#define GETFLOATNUM(scale) if (! vxprintf_getfnum (p, scale)) goto infnan

/* Parameter block passed to various 'put' routines */

typedef struct { int minwidth;          /* mininum output field width */
                 int precision;         /* number of decimal places to output */
                 int negative;          /* number being output requires a '-' sign */
                 int altform;           /* output it in alternative format */
                 int zeropad;           /* right justify, zero fill */
                 int leftjust;          /* left justify, blank fill */
                 int posblank;          /* if positive, output a space for the sign */
                 int plussign;          /* if positive, output a '+' for the sign */
                 int intsize;           /* integer size: 1=Byte, 2=Word, 4=Long, 8=Quad */
                 int fltsize;           /* float size: 4=float, 8=double */

                 char *ncp;             /* start of integer string to output */
                 char *nce;             /* end of integer string to output */

                 char *ob;              /* start of output buffer */
                 char *oe;              /* end of output buffer */
                 char *op;              /* next available byte in ob */
                 char const *nfe;       /* number format error */

                 va_list  ap;
                 uQuad  unum;
                 double fnum;
                 int   expon;

                 int numout;            /* total number of characters output so far */
                 xprintfcb_t *entry;    /* output callback routine */
                 void *param;           /* output callback routine parameter */                 
               } Par;

/* 'Put' routines output common stuff to the output buffer */

#define PUTLEFTFILL(__n) do { if (!(p -> leftjust) && !(p -> zeropad)) PUTFC (p -> minwidth - (__n), ' '); } while (0)
#define PUTLEFTZERO(__n) do { if (!(p -> leftjust) &&  (p -> zeropad)) PUTFC (p -> minwidth - (__n), '0'); } while (0)
#define PUTRIGHTFILL     PUTFC (p -> minwidth, ' ')

#define PUTINT(__alt)  do { sts = putint (p, __alt);   if (sts < 0) return (sts); } while (0)
#define PUTSIGN        do { sts = putsign (p);         if (sts < 0) return (sts); } while (0)
#define PUTFC(__s,__c) do { sts = putfc (p, __s, __c); if (sts < 0) return (sts); } while (0)
#define PUTST(__s,__b) do { sts = putst (p, __s, __b); if (sts < 0) return (sts); } while (0)
#define PUTCH(__c)     do { sts = putch (p, __c);      if (sts < 0) return (sts); } while (0)

/* Round normalised floating point number 'floatnum' to the given number of digits of precision following the decimal point */

#define ROUNDFLOAT(prec) vxprintf_roundflt (p, prec)

/* Get integer in format string.  If *, use next integer argument, otherwise use decimal string */

#define GETFMTINT(fi) fp = vxprintf_getfmtint (p, fp, &fi)

int vxprintf_fp (char fc, Par *p, va_list ap, va_list *ap_r);
int putint  (Par *p, char const *alt);
int putsign (Par *p);
int putfc   (Par *p, int s, char c);
int putst   (Par *p, int s, char const *b);
int putch   (Par *p, char c);
void vxprintf_getunum (Par *p);
void vxprintf_getsnum (Par *p);
int  vxprintf_getfnum (Par *p, bool scale);
void vxprintf_roundflt (Par *p, int prec);
char const *vxprintf_getfmtint (Par *p, char const *fp, int *fi_r);

#endif
