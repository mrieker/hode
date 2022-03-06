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

#include <stdlib.h>

unsigned long strtoul (char const *nptr, char **endptr, unsigned int base)
{
    unsigned long value = 0;

    if (base == 0) {
        if (*nptr == '0') {
            if ((nptr[1] == 'x') || (nptr[1] == 'X')) {
                base = 16;
                nptr += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    while (1) {
        char ch = *nptr;
             if ((ch >= '0') && (ch <= '9')) ch -= '0';
        else if ((ch >= 'A') && (ch <= 'Z')) ch -= 'A' - 10;
        else if ((ch >= 'a') && (ch <= 'z')) ch -= 'a' - 10;
        else break;
        if (ch >= base) break;
        value *= base;
        value += ch;
        nptr ++;
    }
    if (endptr != NULL) *endptr = (char *) nptr;
    return value;
}
