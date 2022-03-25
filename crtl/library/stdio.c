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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fdfile.h"

FILE *stdin;
FILE *stdout;
FILE *stderr;

FILE *fdopen (int fd, char const *mode)
{
    FDFILE *stream = new FDFILE;
    stream->fd = fd;
    return stream;
}

FILE *fopen (char const *path, char const *mode)
{
    int flags = 0;

    if (mode[0] == 'r') flags = O_RDONLY;
    if (mode[0] == 'w') flags = O_CREAT | O_WRONLY | O_TRUNC;

    int fd = open (path, flags, 0666);
    if (fd < 0) return NULL;
    return fdopen (fd, mode);
}

int fclose (FILE *stream)
{
    int rc = stream->fclose ();
    stream->free ();
    return rc;
}

int fgetc (FILE *stream)
{
    return stream->fgetc ();
}

char *fgets (char *buf, int size, FILE *stream)
{
    return stream->fgets (buf, size);
}

int fputc (int chr, FILE *stream)
{
    return stream->fputc (chr);
}

int fputs (char const *buf, FILE *stream)
{
    return stream->fputs (buf);
}

int fflush (FILE *stream)
{
    return stream->fflush ();
}
