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
#include <string.h>
#include <unistd.h>

#define BUFSIZ 512

FILE *stdin;
FILE *stdout;
FILE *stderr;

struct FILE {
    int   fd;
    int   wsize;
    int   wused;
    char *wbuff;
    int   wmode;
    int   wchkd;
    int   rsize;
    int   rused;
    int   rnext;
    char *rbuff;
};

FILE *fdopen (int fd, char const *mode)
{
    FILE *stream = calloc (1, sizeof *stream);
    stream->fd = fd;
    stream->wmode = _IOLBF;
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
    if (stream->rbuff != NULL) {
        free (stream->rbuff);
        stream->rbuff = NULL;
    }
    if (stream->wbuff != NULL) {
        fflush (stream);
        free (stream->wbuff);
        stream->wbuff = NULL;
    }
    int rc = close (stream->fd);
    stream->fd = -1;
    free (stream);
    return rc;
}

int fgetc (FILE *stream)
{
    if (stream->rnext >= stream->rused) {
        int rc = ffill (stream);
        if (rc <= 0) return -1;
    }
    char ch = stream->rbuff[stream->rnext++];
    return (unsigned int) (__uint8_t) ch;
}

char *fgets (char *buf, int size, FILE *stream)
{
    if (-- size < 0) return NULL;

    char *ptr = buf;
    while (-- size >= 0) {
        if (stream->rnext >= stream->rused) {
            int rc = ffill (stream);
            if (rc <= 0) {
                if (ptr > buf) break;
                return NULL;
            }
        }
        char ch = stream->rbuff[stream->rnext++];
        *(ptr ++) = ch;
        if (ch == '\n') break;
    }
    *ptr = 0;

    return buf;
}

// read more into read buffer (rbuff)
// returns number of characters read (or 0/-1 for eof/error)
int ffill (FILE *stream)
{
    if (stream->rbuff == NULL) {
        stream->rsize = BUFSIZ;
        stream->rbuff = malloc (BUFSIZ);
    }
    if (stream->rnext >= stream->rused) {
        stream->rused  = 0;
    } else {
        stream->rused -= stream->rnext;
        memmove (stream->rbuff, stream->rbuff + stream->rnext, stream->rused);
    }
    stream->rnext = 0;
    int rc = read (stream->fd, stream->rbuff + stream->rused, stream->rsize - stream->rused);
    if (rc > 0) {
        stream->rused += rc;
    }
    return rc;
}

static int maybeflush (FILE *stream)
{
    switch (stream->wmode) {
    case _IOFBF:
        return 0;
    case _IOLBF:
        if (memchr (stream->wbuff + stream->wchkd, '\n', stream->wused - stream->wchkd) == NULL) {
            stream->wchkd = stream->wused;
            return 0;
        }
    }
    return fflush (stream);
}

int fputc (int chr, FILE *stream)
{
    if (stream->wused >= stream->wsize) {
        int rc = fflush (stream);
        if (rc < 0) return -1;
    }
    stream->wbuff[stream->wused++] = (char) chr;
    return maybeflush (stream);
}

int fputs (char const *buf, FILE *stream)
{
    int len = strlen (buf);
    while (len > 0) {
        int room = stream->wsize - stream->wused;
        if (room <= 0) {
            int rc = fflush (stream);
            if (rc < 0) return -1;
            room = stream->wsize - stream->wused;
        }
        if (room > len) room = len;
        memcpy (stream->wbuff + stream->wused, buf, room);
        stream->wused += room;
        len -= room;
    }
    return maybeflush (stream);
}

int fflush (FILE *stream)
{
    if (stream->wbuff == NULL) {
        stream->wsize = BUFSIZ;
        stream->wbuff = malloc (BUFSIZ);
    }
    if (stream->wused > 0) {
        int rc;
        for (int i = 0; i < stream->wused; i += rc) {
            rc = write (stream->fd, stream->wbuff + i, stream->wused - i);
            if (rc <= 0) {
                return -1;
            }
        }
    }
    stream->wused = 0;
    stream->wchkd = 0;
    return 0;
}

int printf (char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vprintf (fmt, ap);
    va_end (ap);
    return rc;
}

int dprintf (int fd, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vdprintf (fd, fmt, ap);
    va_end (ap);
    return rc;
}

int fprintf (FILE *stream, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vfprintf (stream, fmt, ap);
    va_end (ap);
    return rc;
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

int asprintf (char **buf_r, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vasprintf (buf_r, fmt, ap);
    va_end (ap);
    return rc;
}

int xprintf (xprintfcb_t *entry, void *param, int size, char *buff, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vxprintf (entry, param, size, buff, fmt, ap);
    va_end (ap);
    return rc;
}

int vprintf (char const *fmt, va_list ap)
{
    return vfprintf (stdout, fmt, ap);
}

struct Dcb {
    int fd;
    char buff[BUFSIZ];
};

static int descrcallback (void *param, int *size_r, char **buff_r, bool final)
{
    Dcb *dcb = param;
    char *buf = *buff_r;
    assert (buf == dcb->buff);
    int len = *size_r;
    while (len > 0) {
        int rc = write (dcb->fd, buf, len);
        if (rc <= 0) return -1;
        buf += rc;
        len -= rc;
    }
    *size_r = sizeof dcb->buff;
    return 0;
}

int vdprintf (int fd, char const *fmt, va_list ap)
{
    Dcb dcb;
    dcb.fd = fd;
    return vxprintf (descrcallback, &dcb, sizeof dcb.buff, dcb.buff, fmt, ap);
}

static int streamcallback (void *param, int *size_r, char **buff_r, bool final)
{
    FILE *stream = param;
    stream->wused += *size_r;
    int rc;
    if (final) {
        rc = maybeflush (stream);
    } else {
        *size_r = stream->wsize;
        *buff_r = stream->wbuff;
        rc = fflush (stream);
    }
    return rc;
}

int vfprintf (FILE *stream, char const *fmt, va_list ap)
{
    return vxprintf (streamcallback, stream, stream->wsize - stream->wused, stream->wbuff + stream->wused, fmt, ap);
}

int vsprintf (char *buf, char const *fmt, va_list ap)
{
    return vsnprintf (buf, 0x7FFF, fmt, ap);
}

static int buffercallback (void *param, int *size_r, char **buff_r, bool final)
{
    *buff_r += *size_r;
    *size_r  = *(int *)param -= *size_r;
    return 0;
}

int vsnprintf (char *buf, int len, char const *fmt, va_list ap)
{
    if (-- len < 0) return -1;
    int rc = vxprintf (buffercallback, &len, len, buf, fmt, ap);
    if (rc >= 0) buf[rc] = 0;
             else buf[0] = 0;
    return rc;
}

struct Mcb {
    char *buff;
    int size;
    int used;
};

static int malloccallback (void *param, int *size_r, char **buff_r, bool final)
{
    Mcb *mcb = param;
    mcb->used += *size_r;
    if (mcb->used >= mcb->size) {
        mcb->size += mcb->size / 2;
        mcb->buff  = realloc (mcb->buff, mcb->size);
    }
    *size_r = mcb->size - mcb->used;
    *buff_r = mcb->buff + mcb->used;
    return 0;
}

int vasprintf (char **buf_r, char const *fmt, va_list ap)
{
    Mcb mcb;
    mcb.buff = malloc (64);
    mcb.size = 64;
    mcb.used = 0;
    int rc = vxprintf (malloccallback, &mcb, mcb.size, mcb.buff, fmt, ap);
    if (rc < 0) {
        free (mcb.buff);
        *buf_r = NULL;
        return rc;
    }
    assert (rc < mcb.size);
    mcb.buff[rc] = 0;
    *buf_r = realloc (mcb.buff, rc + 1);
    return rc;
}
