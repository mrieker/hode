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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void assertfail (char const *file, int line)
{
    ass_putstr ("assertfail: ");
    ass_putstr (file);
    ass_putchr (' ');
    ass_putint (line);
    ass_putchr ('\n');
    exit (1);
}

void ass_putdbl (double dbl)
{
    if (isnan (dbl)) {
        ass_putstr ("nan");
        return;
    }
    if (dbl < 0.0) {
        ass_putchr ('-');
        dbl = - dbl;
    }
    if (! isfinite (dbl)) {
        ass_putstr ("inf");
    } else if (dbl == 0.0) {
        ass_putstr ("0.0");
    } else if (dbl < 0.001) {
        dbl *= 10000.0;
        int exp = 0;
        while (dbl < 0.001) { exp -= 5; dbl *= 100000.0; }
        while (dbl < 1000.0) { -- exp; dbl *= 10.0; }
        int num = (int) (dbl + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putchr ('e');
        ass_putint (exp);
    } else if (dbl > 1000.0) {
        int exp = 0;
        while (dbl >= 1000000.0) { exp += 5; dbl /= 100000.0; }
        while (dbl >= 1.0) { exp ++; dbl /= 10.0; }
        dbl *= 10000.0;
        int num = (int) (dbl + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putstr ("e+");
        ass_putint (exp);
    } else {
        dbl += 0.00005;
        int i = (int) dbl;
        ass_putint (i);
        ass_putchr ('.');
        for (int j = 4; -- j >= 0;) {
            dbl -= i;
            dbl *= 10.0;
            i = (int) dbl;
            ass_putchr ((char) ('0' + i));
        }
    }
}

void ass_putflt (float flt)
{
    if (isnanf (flt)) {
        ass_putstr ("nan");
        return;
    }
    if (flt < 0.0) {
        ass_putchr ('-');
        flt = - flt;
    }
    if (! isfinitef (flt)) {
        ass_putstr ("inf");
    } else if (flt == 0.0) {
        ass_putstr ("0.0");
    } else if (flt < 0.001) {
        flt *= 10000.0;
        int exp = 0;
        while (flt < 0.001) { exp -= 5; flt *= 100000.0; }
        while (flt < 1000.0) { -- exp; flt *= 10.0; }
        int num = (int) (flt + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putchr ('e');
        ass_putint (exp);
    } else if (flt > 1000.0) {
        int exp = 0;
        while (flt >= 1000000.0) { exp += 5; flt /= 100000.0; }
        while (flt >= 1.0) { exp ++; flt /= 10.0; }
        flt *= 10000.0;
        int num = (int) (flt + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putstr ("e+");
        ass_putint (exp);
    } else {
        flt += 0.00005;
        int i = (int) flt;
        ass_putint (i);
        ass_putchr ('.');
        for (int j = 4; -- j >= 0;) {
            flt -= i;
            flt *= 10.0;
            i = (int) flt;
            ass_putchr ((char) ('0' + i));
        }
    }
}

void ass_putstr (char const *str)
{
    int len = strlen (str);
    ass_putbuf (str, len);
}

void ass_putint (int val)
{
    bool neg = val < 0;
    if (neg) val = - val;
    char num[12];
    int len = 12;
    do {
        num[--len] = val % 10 + '0';
        val /= 10;
    } while (val > 0);
    if (neg) num[--len] = '-';
    ass_putbuf (num + len, 12 - len);
}

void ass_putuint (unsigned int val)
{
    char num[12];
    int len = 12;
    do {
        num[--len] = val % 10 + '0';
        val /= 10;
    } while (val > 0);
    ass_putbuf (num + len, 12 - len);
}

void ass_putptr (void *ptr)
{
    ass_putuinthex ((__size_t) ptr);
}

void ass_putuinthex (unsigned int val)
{
    char num[12];
    int len = 12;
    do {
        char chr = val % 16 + '0';
        if (chr > '9') chr += 'A' - '0' - 10;
        num[--len] = chr;
        val /= 16;
    } while (val > 0);
    ass_putbuf (num + len, 12 - len);
}

void ass_putchr (char chr)
{
    ass_putbuf (&chr, 1);
}

void ass_putbuf (char const *str, int len)
{
    while (len > 0) {
        int rc = write (2, str, len);
        if (rc <= 0) exit (9999);
        str += rc;
        len -= rc;
    }
}
