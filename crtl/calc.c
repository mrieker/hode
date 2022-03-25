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

// calculator program

#include <complex.h>
#include <errno.h>
#include <math.h>
#include <readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint;

typedef ComplexD FnFunc ();
typedef ComplexD OpFunc (ComplexD left, ComplexD rite);

struct Fn {
    char const *name;
    FnFunc *func;
};

struct Op {
    char const *name;
    OpFunc *func;
};

char *lineptr;
ComplexD *savedvals;
uint allsaveds, numsaveds;

bool compute (ComplexD *val_r);
ComplexD evalexpr (uint preced);
ComplexD getval ();
char skipspaces ();
void throwerror (char const *fmt, ...);

int main (int argc, char **argv)
{
    // if present, compute single command-line expression and print result
    if (argc > 1) {

        // make args into one space-separated null-terminated string
        int totlen = 0;
        for (int i = 0; ++ i < argc;) {
            totlen += strlen (argv[i]) + 1;
        }
        lineptr = malloc (totlen);
        totlen = 0;
        for (int i = 0; ++ i < argc;) {
            if (totlen > 0) lineptr[totlen-1] = ' ';
            int len = strlen (argv[i]) + 1;
            memcpy (lineptr + totlen, argv[i], len);
            totlen += len;
        }

        // compute the value
        ComplexD val;
        if (! compute (&val)) return 1;

        // print result without any decoration
        if (val.imag == 0.0) {
            printf ("%.16lg\n", val.real);
        } else if (val.real == 0.0) {
            printf ("j %.16lg\n", val.imag);
        } else {
            printf ("%.16lg j %.16lg\n", val.real, val.imag);
        }

        // success
        return 0;
    }

    // no command-line parameters, read from stdin and print to stdout
    allsaveds = 64;
    numsaveds = 0;
    savedvals = malloc (allsaveds * sizeof *savedvals);

    ReadLine rl;
    int rlrc = rl.open (0);
    char *linebuf = (rlrc < 0) ? malloc (64) : NULL;

    while (1) {

        // read line from stdin, exit loop if eof
        if (rlrc < 0) {
            __size_t siz = allocsz (linebuf);
            if (fgets (linebuf, siz, stdin) == NULL) break;
            int len = strlen (linebuf);
            if (len == 0) break;
            while (linebuf[len-1] != '\n') {
                siz += siz / 2;
                linebuf = realloc (linebuf, siz);
                if (fgets (linebuf + len, siz - len, stdin) == NULL) goto eof2;
                len += strlen (linebuf + len);
            }
            linebuf[len-1] = 0;
        eof2:;
            lineptr = linebuf;
        } else {
            lineptr = rl.read ("> ");
            if (lineptr == NULL) break;
        }

        // skip blank lines
        if (skipspaces () == 0) continue;

        // compute value
        ComplexD val;
        if (compute (&val)) {

            // save result in saved-values array
            if (numsaveds >= allsaveds) {
                allsaveds += allsaveds / 2;
                savedvals  = realloc (savedvals, allsaveds * sizeof *savedvals);
            }
            savedvals[numsaveds] = val;

            // print result along with saved-values array index
            if (val.imag == 0.0) {
                printf (" $%u = %.16lg\n", numsaveds, val.real);
            } else if (val.real == 0.0) {
                printf (" $%u = j %.16lg\n", numsaveds, val.imag);
            } else {
                printf (" $%u = %.16lg j %.16lg\n", numsaveds, val.real, val.imag);
            }

            numsaveds ++;
        }
    }

    return 0;
}

// compute expression value
//  input:
//   lineptr = points to expression string
//  output:
//   returns 0: error message printed
//           1: successful, *val_r = resultant value
bool compute (ComplexD *val_r)
{
    try {
        *val_r = evalexpr (0);
        if (skipspaces () != 0) {
            throwerror ("extra stuff at end of line at <%s>", lineptr);
        }
        return 1;
    } catch (char *err) {
        fprintf (stderr, "%s\n", err);
        free (err);
    } catch (char const *err) {
        fprintf (stderr, "%s\n", err);
    }
    return 0;
}

// +, -, *, /, ** operators
ComplexD opadd (ComplexD left, ComplexD rite) { return left.add (rite); }
ComplexD opsub (ComplexD left, ComplexD rite) { return left.sub (rite); }
ComplexD opmul (ComplexD left, ComplexD rite) { return left.mul (rite); }
ComplexD opdiv (ComplexD left, ComplexD rite) { return left.div (rite); }
ComplexD oppow (ComplexD left, ComplexD rite) { return left.pow (rite); }

Op const optabl0[] = {
    { "+",  opadd },
    { "-",  opsub },
    { NULL, NULL }
};

Op const optabl1[] = {
    { "*",  opmul },
    { "/",  opdiv },
    { NULL, NULL }
};

Op const optabl2[] = {
    { "**", oppow },
    { NULL, NULL }
};

#define NPRECED 3
Op const *const optabls[NPRECED] = { optabl0, optabl1, optabl2 };

// evaluate expression from input string
//  input:
//   lineptr = points to input string
//   preced = 0: handle +,-,*,/,**
//            1: handle *,/,**, stop on +,-
//            2: handle **, stop on +,-,*,/
//  output:
//   returns expression value
//   lineptr = updated past string evaluated
//   throws errors
ComplexD evalexpr (uint preced)
{
    uint precp1 = preced + 1;
    ComplexD leftval = (precp1 < NPRECED) ? evalexpr (precp1) : getval ();

    while (skipspaces () != 0) {
        Op const *op;
        for (op = optabls[preced]; op->name != NULL; op ++) {
            uint len = strlen (op->name);
            if (memcmp (lineptr, op->name, len) == 0) {
                lineptr += len;
                goto goodop;
            }
        }
        break;
    goodop:;

        ComplexD riteval = (precp1 < NPRECED) ? evalexpr (precp1) : getval ();
        leftval = op->func (leftval, riteval);
    }

    return leftval;
}

// named functions
ComplexD fnabs   () { ComplexD a = getval (); return ComplexD::make (a.abs (), 0); }
ComplexD fnacos  () { ComplexD a = getval (); return a.acos (); }
ComplexD fnasin  () { ComplexD a = getval (); return a.asin (); }
ComplexD fnatan  () { ComplexD a = getval (); return a.atan (); }
ComplexD fncos   () { ComplexD a = getval (); return a.cos  (); }
ComplexD fne     () { return ComplexD::make (M_E, 0); }
ComplexD fnim    () { ComplexD a = getval (); return ComplexD::make (a.imag, 0); }
ComplexD fnln    () { ComplexD a = getval (); return a.log (); }
ComplexD fnlog   () { ComplexD a = getval (); return a.log ().div (ComplexD::make (M_LN10, 0)); }
ComplexD fnpi    () { return ComplexD::make (M_PI, 0); }
ComplexD fnquit  () { exit (0); }
ComplexD fnre    () { ComplexD a = getval (); return ComplexD::make (a.real, 0); }
ComplexD fnsin   () { ComplexD a = getval (); return a.sin  (); }
ComplexD fnsqrt  () { ComplexD a = getval (); return a.sqrt (); }
ComplexD fntan   () { ComplexD a = getval (); return a.tan  (); }

Fn const fntabl[] = {
    { "acos",  fnacos  },
    { "asin",  fnasin  },
    { "atan",  fnatan  },
    { "quit",  fnquit  },
    { "sqrt",  fnsqrt  },
    { "abs",   fnabs   },
    { "cos",   fncos   },
    { "log",   fnlog   },
    { "sin",   fnsin   },
    { "tan",   fntan   },
    { "im",    fnim    },
    { "ln",    fnln    },
    { "pi",    fnpi    },
    { "re",    fnre    },
    { "e",     fne     },
    { NULL,    NULL    }
};

// get next value from input string
//  input:
//   lineptr = points to input string
//  output:
//   returns value
//   lineptr = updated past value
//   throws errors
ComplexD getval ()
{
    char *endnum;
    ComplexD val;

    char ch = skipspaces ();
    if (ch == '(') {
        lineptr ++;
        val = evalexpr (0);
        if (skipspaces () != ')') {
            throwerror ("unbalanced () at <%s>", lineptr);
        }
        lineptr ++;
        goto ret;
    }
    if (ch == '-') {
        lineptr ++;
        val = getval ().neg ();
        goto ret;
    }
    if (ch == '$') {
        uint idx = strtoul (lineptr + 1, &endnum, 10);
        if ((errno != 0) || (idx >= numsaveds)) {
            throwerror ("bad saved number at <%s>", lineptr);
        }
        val = savedvals[idx];
        lineptr = endnum;
        goto ret;
    }
    if ((ch >= 'a') && (ch <= 'z')) {
        for (Fn const *fn = fntabl; fn->name != NULL; fn ++) {
            uint len = strlen (fn->name);
            if (strncasecmp (lineptr, fn->name, len) == 0) {
                lineptr += len;
                val = fn->func ();
                goto ret;
            }
        }
    }
    val.real = 0.0;
    val.imag = 0.0;
    if (ch != 'j') {
        errno = 0;
        val.real = strtod (lineptr, &endnum);
        if (errno != 0) throwerror ("unknown operand at <%s>", lineptr);
        lineptr = endnum;
        ch = skipspaces ();
    }
    if (ch == 'j') {
        errno = 0;
        val.imag = strtod (++ lineptr, &endnum);
        if (errno != 0) val.imag = 1.0;
                 else lineptr = endnum;
    }

ret:
    return val;
}

// skip spaces in input string
//  input:
//   lineptr = points to input string
//  output:
//   returns to next non-blank character (or nul if end-of-string)
//   lineptr = advanced to next non-blank character (or nul if end-of-string)
char skipspaces ()
{
    char ch;
    while ((ch = *lineptr) != 0)  {
        if (ch > ' ') break;
        lineptr ++;
    }
    return ch;
}

// format and throw error message
void throwerror (char const *fmt, ...)
{
    char *buf;
    va_list ap;

    va_start (ap, fmt);
    vasprintf (&buf, fmt, ap);
    va_end (ap);
    throw buf;
}
