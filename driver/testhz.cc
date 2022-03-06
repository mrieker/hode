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

// measure cpu clock frequency
// cc -Wall -O2 -o testhz.`uname -m` testhz.cc rdcyc.cc
// ./testhz.`uname -m`

#include <stdio.h>
#include <time.h>

#include "rdcyc.h"

int main ()
{
    rdcycinit ();
    time_t start = time (NULL);
    while (time (NULL) == start) { }
    uint32_t beg = rdcyc ();
    ++ start;
    while (time (NULL) == start) { }
    uint32_t end = rdcyc ();
    printf ("%u\n", end - beg);
    return 0;
}
