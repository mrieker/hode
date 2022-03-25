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

#ifndef _FDFILE_H
#define _FDFILE_H

#include <stdio.h>

struct FILE {
    int   wsize;
    int   wused;
    char *wbuff;
    int   wmode;
    int   wchkd;
    int   rsize;
    int   rused;
    int   rnext;
    char *rbuff;

    FILE ();
    virtual ~FILE ();

    int fclose ();
    int fgetc ();
    char *fgets (char *buf, int size);
    int fputc (int chr);
    int fputs (char const *buf);
    int maybeflush ();
    int ffill ();
    int fflush ();

    virtual int fput (char const *buf, int len);

    virtual void free ();
    virtual int close ();
    virtual int read (char *buff, int size);
    virtual int write (char *buff, int size);
};

struct FDFILE : FILE {
    int fd;

    FDFILE ();

    virtual void free ();
    virtual int close ();
    virtual int read (char *buff, int size);
    virtual int write (char *buff, int size);
};

#endif
