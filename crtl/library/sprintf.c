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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "fdfile.h"

struct BUFFILE : FILE {
    virtual int fput (char const *buf, int len);
};

BUFFILE::BUFFILE ();

int BUFFILE::fput (char const *buf, int len)
{
    if (len > 0) {
        int room = this->wsize - this->wused;
        if (room > len) room = len;
        memcpy (this->wbuff + this->wused, buf, room);
        this->wused += room;
    }
    return len;
}

int sprintf (char *buf, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vsprintf (buf, fmt, ap);
    va_end (ap);
    return rc;
}

int snprintf (char *buf, int len, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vsnprintf (buf, len, fmt, ap);
    va_end (ap);
    return rc;
}

int vsprintf (char *buf, char const *fmt, va_list ap)
{
    return vsnprintf (buf, 0x7FFF, fmt, ap);
}

int vsnprintf (char *buf, int len, char const *fmt, va_list ap)
{
    if (-- len < 0) return -1;
    BUFFILE stream;
    stream.wbuff = buf;
    stream.wsize = len;
    int rc = vfprintf (&stream, fmt, ap);
    if (rc > len) buf[len] = 0;
    else if (rc >= 0) buf[rc] = 0;
    else buf[0] = 0;
    return rc;
}
