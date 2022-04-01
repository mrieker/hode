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
    char name[6];
    FnFunc *func;
};

struct Op {
    char name[4];
    OpFunc *func;
};

struct Var {
    Var *next;
    ComplexD valu;
    char name[1];
    void print ();
};

char *lineptr;
char const prompt[] = "\n> ";
ComplexD *savedvals;
double d2r = 1.0;
double r2d = 1.0;
FILE *logfile;
uint allsaveds, numsaveds;
Var *variables;

bool compute (ComplexD *val_r);
ComplexD evalexpr (uint preced);
ComplexD getval ();
char skipspaces ();
uint skipvarname ();
void throwerror (char const *fmt, ...);
void printval (ComplexD val);
void lprintf (FILE *stream, char const *fmt, ...);

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
        printval (val);

        // success
        return 0;
    }

    // no command-line parameters, read from stdin and print to stdout
    logfile = fopen ("calc.log", "a");
    if (logfile == NULL) {
        fprintf (stderr, "error creating logfile calc.log: %s\n", strerror (errno));
    } else {
        fprintf (logfile, "\n--------\n");
    }

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
            lineptr = linebuf = rl.read (prompt);
            if (linebuf == NULL) break;
        }

        // maybe log the input line
        if (logfile != NULL) {
            fprintf (logfile, "%s%s\n", prompt, lineptr);
        }

        // skip blank lines
        // don't save in readline history
        if (skipspaces () == 0) {
            if (rlrc >= 0) free (linebuf);
            continue;
        }

        // maybe save in readline history
        if (rlrc >= 0) rl.save (linebuf);

        // check for varname=expression
        ComplexD val;
        char *vareq = strchr (lineptr, '=');
        if (vareq != NULL) {
            char *varname = lineptr;
            uint varnamelen = skipvarname ();
            if (varnamelen > 0) {

                // optional spaces before equal sign
                skipspaces ();
                if (lineptr == vareq) {

                    // compute expression before altering any definitions
                    ++ lineptr;
                    if (compute (&val)) {

                        // search definitions for same name
                        Var *var;
                        for (var = variables; var != NULL; var = var->next) {
                            uint len = strlen (var->name);
                            if ((len == varnamelen) && (memcmp (var->name, varname, len) == 0)) goto savevar;
                        }

                        // make a new var and link in list
                        var = malloc (varnamelen + sizeof *var);
                        var->next = variables;
                        memcpy (var->name, varname, varnamelen);
                        var->name[varnamelen] = 0;
                        variables = var;

                        // save value in var and print
                    savevar:;
                        var->valu = val;
                        var->print ();
                    }
                    continue;
                }
            }
            fprintf (stderr, "bad variable name <%.*s>\n", vareq - varname, varname);
            continue;
        }

        // compute value
        if (compute (&val)) {

            // save result in saved-values array
            if (numsaveds >= allsaveds) {
                allsaveds += allsaveds / 2;
                savedvals  = realloc (savedvals, allsaveds * sizeof *savedvals);
            }
            savedvals[numsaveds] = val;

            // print result along with saved-values array index
            lprintf (stdout, " $%u = ", numsaveds);
            printval (val);

            numsaveds ++;
        }
    }

    if (logfile != NULL) {
        fprintf (logfile, "%s*EOF*\n", prompt);
        fclose (logfile);
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
        lprintf (stderr, "%s\n", err);
        free (err);
    } catch (char const *err) {
        if (err[0] != 0) lprintf (stderr, "%s\n", err);
    }
    return 0;
}

// +, -, *, /, ** operators
ComplexD opadd (ComplexD left, ComplexD rite) { return left.add (rite); }
ComplexD opsub (ComplexD left, ComplexD rite) { return left.sub (rite); }
ComplexD opmul (ComplexD left, ComplexD rite) { return left.mul (rite); }
ComplexD opdiv (ComplexD left, ComplexD rite) { return left.div (rite); }
ComplexD oppow (ComplexD left, ComplexD rite) { return left.pow (rite); }
ComplexD oplog (ComplexD left, ComplexD rite) { return rite.log ().div (left.log ()); }

Op const optabl0[] = {
    { "+",  opadd },
    { "-",  opsub },
    { "",   NULL }
};

Op const optabl1[] = {
    { "*",  opmul },
    { "/",  opdiv },
    { "",   NULL }
};

Op const optabl2[] = {
    { "**", oppow },
    { "//", oplog },
    { "",   NULL }
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
        for (op = optabls[preced]; op->name[0] != 0; op ++) {
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
ComplexD fnacos  () { ComplexD a = getval (); return a.acos ().mulr (r2d); }
ComplexD fnarg   () { ComplexD a = getval (); return ComplexD::make (a.arg () * r2d, 0); }
ComplexD fnasin  () { ComplexD a = getval (); return a.asin ().mulr (r2d); }
ComplexD fnatan  () { ComplexD a = getval (); return a.atan ().mulr (r2d); }
ComplexD fncos   () { ComplexD a = getval (); return a.mulr (d2r).cos (); }
ComplexD fndeg   () { r2d = 180.0 / M_PI; d2r = M_PI / 180.0; return getval (); }
ComplexD fndump  () { for (Var *var = variables; var != NULL; var = var->next) var->print (); throw ""; }
ComplexD fne     () { return ComplexD::make (M_E, 0); }
ComplexD fnhypot () { ComplexD a = getval (); ComplexD b = getval (); return a.sq ().add (b.sq ()).sqrt (); }
ComplexD fnim    () { ComplexD a = getval (); return ComplexD::make (a.imag, 0); }
ComplexD fnln    () { ComplexD a = getval (); return a.log (); }
ComplexD fnpi    () { return ComplexD::make (M_PI, 0); }
ComplexD fnquit  () { if (logfile != NULL) fclose (logfile); exit (0); }
ComplexD fnrad   () { r2d = 1.0; d2r = 1.0; return getval (); }
ComplexD fnre    () { ComplexD a = getval (); return ComplexD::make (a.real, 0); }
ComplexD fnsin   () { ComplexD a = getval (); return a.mulr (d2r).sin (); }
ComplexD fnsq    () { ComplexD a = getval (); return a.sq   (); }
ComplexD fnsqrt  () { ComplexD a = getval (); return a.sqrt (); }
ComplexD fntan   () { ComplexD a = getval (); return a.mulr (d2r).tan (); }

Fn const fntabl[] = {
    { "abs",   fnabs   },
    { "acos",  fnacos  },
    { "arg",   fnarg   },
    { "asin",  fnasin  },
    { "atan",  fnatan  },
    { "cos",   fncos   },
    { "deg",   fndeg   },
    { "dump",  fndump  },
    { "e",     fne     },
    { "hypot", fnhypot },
    { "im",    fnim    },
    { "ln",    fnln    },
    { "pi",    fnpi    },
    { "quit",  fnquit  },
    { "rad",   fnrad   },
    { "re",    fnre    },
    { "sin",   fnsin   },
    { "sq",    fnsq    },
    { "sqrt",  fnsqrt  },
    { "tan",   fntan   },
    { "",      NULL    }
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
    char *varname = lineptr;
    uint varnamelen = skipvarname ();
    if (varnamelen > 0) {
        for (Var *var = variables; var != NULL; var = var->next) {
            uint len = strlen (var->name);
            if ((len == varnamelen) && (memcmp (var->name, varname, len) == 0)) {
                val = var->valu;
                goto ret;
            }
        }
        for (Fn const *fn = fntabl; fn->name[0] != 0; fn ++) {
            uint len = strlen (fn->name);
            if ((len == varnamelen) && (memcmp (fn->name, varname, len) == 0)) {
                val = fn->func ();
                goto ret;
            }
        }
        throwerror ("unknown function/variable <%.*s>", varnamelen, varname);
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
//   returns next non-blank character (or nul if end-of-string or comment)
//   lineptr = advanced to next non-blank character (or nul if end-of-string)
char skipspaces ()
{
    char ch;
    while ((ch = *lineptr) != 0)  {
        if (ch == '#') return 0;
        if (ch > ' ') break;
        lineptr ++;
    }
    return ch;
}

// varname must start with letter
// and may contain additional letters and numbers
// don't allow varname starting with "j" followed by digits cuz it looks like imaginary number
uint skipvarname ()
{
    char *varname = lineptr;
    char ch = *lineptr;
    if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'))) {
        bool imag = ch == 'j';
        while (1) {
            ch = *(++ lineptr);
            if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')) || (ch == '_')) {
                imag = 0;
                continue;
            }
            if ((ch < '0') || (ch > '9')) break;
        }
        if (imag) lineptr = varname;
    }
    return lineptr - varname;
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

// print variable
void Var::print ()
{
    lprintf (stdout, " %s = ", this->name);
    printval (this->valu);
}

// print complex number
void printval (ComplexD val)
{
    if (val.imag == 0.0) {
        lprintf (stdout, "%.16lg\n", val.real);
    } else if (val.real == 0.0) {
        lprintf (stdout, "j %.16lg\n", val.imag);
    } else {
        lprintf (stdout, "%.16lg j %.16lg\n", val.real, val.imag);
    }
}

// print line to stream and logfile
void lprintf (FILE *stream, char const *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vfprintf (stream, fmt, ap);
    va_end (ap);

    if (logfile != NULL) {
        va_start (ap, fmt);
        vfprintf (logfile, fmt, ap);
        va_end (ap);
    }
}
