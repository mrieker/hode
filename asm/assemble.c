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

// ./assemble asmfile objfile > lisfile

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pflags.h"

#define ARG_REGD 0x01
#define ARG_LSOF 0x02
#define ARG_REGA 0x04
#define ARG_REGB 0x08
#define ARG_BROF 0x10
#define ARG_LDIM 0x20
#define ARG_RGAB 0x40

#define IR_REGA 10
#define IR_REGD  7
#define IR_REGB  4

#define TT_ENDLINE 0
#define TT_INTEGER 1
#define TT_SYMBOL 2
#define TT_DELIM 3
#define TT_STRING 4
#define TT_REGISTER 5

typedef struct Opcode Opcode;
typedef struct PSect PSect;
typedef struct Stack Stack;
typedef struct Symbol Symbol;
typedef struct Token Token;

struct Opcode {
    uint16_t opc;           // basic opcode
    char mne[5];            // mnemonic string (null terminated)
    uint8_t args;           // argument bitmask
};

struct PSect {
    PSect *next;            // next in psects list
    uint16_t pflg;          // flags
    uint16_t p2al;          // log2 power of alignment
    uint16_t offs;          // offset for next object code generation
    uint16_t size;          // total size of the section
    char name[1];           // null terminated psect name
};

struct Stack {
    Stack *next;            // next stack entry
    PSect *psect;           // value relocation
    Symbol *undsym;         // undefined symbol
    Token *opctok;          // operator token
    uint32_t value;         // value
    char preced;            // precedence
};

struct Symbol {
    Symbol *next;           // next in symbols list
    PSect *psect;           // value relocation
    uint16_t value;         // value
    char defind;            // set if defined
    char global;            // set if global
    char name[1];           // null terminated symbol name
};

struct Token {
    Token *next;            // next token in line
    char const *srcname;    // source file name
    int srcline;            // one-based line in source file
    int srcchar;            // zero-based character in source line
    uint32_t intval;        // integer value
    char toktype;           // TT_ token type
    char strval[1];         // string value (null terminated)
};

static Opcode const opcodelist[] = {
    { 0x200E, "adc",  ARG_REGD | ARG_REGA | ARG_REGB },
    { 0x200C, "add",  ARG_REGD | ARG_REGA | ARG_REGB },
    { 0x2009, "and",  ARG_REGD | ARG_REGA | ARG_REGB },
    { 0x2001, "asr",  ARG_REGD | ARG_REGA            },
    { 0x1001, "bcc",  ARG_BROF                       },
    { 0x1000, "bcs",  ARG_BROF                       },
    { 0x0400, "beq",  ARG_BROF                       },
    { 0x0801, "bge",  ARG_BROF                       },
    { 0x0C01, "bgt",  ARG_BROF                       },
    { 0x1401, "bhi",  ARG_BROF                       },
    { 0x1001, "bhis", ARG_BROF                       },
    { 0x2389, "bit",  ARG_REGA | ARG_REGB            },     // and r7,ra,rb
    { 0x0C00, "ble",  ARG_BROF                       },
    { 0x1000, "blo",  ARG_BROF                       },
    { 0x1400, "blos", ARG_BROF                       },
    { 0x0800, "blt",  ARG_BROF                       },
    { 0x1800, "bmi",  ARG_BROF                       },
    { 0x0401, "bne",  ARG_BROF                       },
    { 0x1801, "bpl",  ARG_BROF                       },
    { 0x0001, "br",   ARG_BROF                       },
    { 0x1C01, "bvc",  ARG_BROF                       },
    { 0x1C00, "bvs",  ARG_BROF                       },
    { 0x3C7A, "clr",  ARG_REGD                       },     // xor rd,r7,r7
    { 0x238D, "cmp",  ARG_REGA | ARG_REGB            },     // sub r7,ra,rb
    { 0x2007, "com",  ARG_REGD | ARG_REGB            },
    { 0x0000, "halt", ARG_REGB                       },
    { 0x2006, "inc",  ARG_REGD | ARG_REGB            },
    { 0x0002, "iret", 0                              },
    { 0x8000, "lda",  ARG_REGD | ARG_LSOF            },
    { 0xE000, "ldbs", ARG_REGD | ARG_LSOF | ARG_LDIM },
    { 0xA000, "ldbu", ARG_REGD | ARG_LSOF | ARG_LDIM },
    { 0xC000, "ldw",  ARG_REGD | ARG_LSOF | ARG_LDIM },
    { 0x2000, "lsr",  ARG_REGD | ARG_REGA            },
    { 0x2004, "mov",  ARG_REGD | ARG_REGB            },
    { 0x2005, "neg",  ARG_REGD | ARG_REGB            },
    { 0x2008, "or",   ARG_REGD | ARG_REGA | ARG_REGB },
    { 0x0006, "rdps", ARG_REGD                       },
    { 0x200E, "rol",  ARG_REGD | ARG_RGAB            },     // adc rd,rab,rab
    { 0x2002, "ror",  ARG_REGD | ARG_REGA            },
    { 0x200F, "sbb",  ARG_REGD | ARG_REGA | ARG_REGB },
    { 0x200C, "shl",  ARG_REGD | ARG_RGAB            },     // add rd,rab,rab
    { 0x6000, "stb",  ARG_REGD | ARG_LSOF            },
    { 0x4000, "stw",  ARG_REGD | ARG_LSOF            },
    { 0x200D, "sub",  ARG_REGD | ARG_REGA | ARG_REGB },
    { 0x2384, "tst",  ARG_REGB                       },     // mov r7,rb
    { 0x0004, "wrps", ARG_REGB                       },
    { 0x200A, "xor",  ARG_REGD | ARG_REGA | ARG_REGB } };

static char const precedence[] = "*/% +-  &   ^   |   ";

static char error;
static char lisaddr[5], lisdata[9];
static FILE *hexfile;
static int emitidx, passno;
static int ifblock, iflevel;
static PSect *psects;
static Symbol *dotsymbol, *symbols;
static uint8_t emitbuf[64];

static void doasmpass (char const *asmname);
static Token *tokenize (char const *srcname, int srcline, char const *asmline);
static void processline (Token *token);
static char *processasciidirective (Token *token);
static void definesymbol (Token *token, char const *name, PSect *psect, uint16_t value);
static Token *decodexpr (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r);
static Token *decodeval (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r);
static Stack *dostackop (Stack *stackb);
static void emitbyte (uint8_t b);
static void emitlong (uint32_t w);
static void emitword (uint16_t w);
static void emitwordrel (uint16_t w, PSect *psect, Symbol *undsym);
static void emitflush ();
static void setdotsymval (uint16_t val);
static char *fillhex (int len, char *buf, uint32_t val);
static void errortok (Token *tok, char const *fmt, ...);
static void errorloc (char const *srcname, int srcline, int srcchar, char const *fmt, ...);

int main (int argc, char **argv)
{
    char const *asmname, *hexname;
    PSect *psect, *textpsect;
    Symbol *symbol;

    if (argc != 3) {
        fprintf (stderr, "usage: assemble <asmfile> <hexfile>\n");
        return 1;
    }

    asmname = argv[1];

    textpsect = malloc (8 + sizeof *textpsect);
    textpsect->next = NULL;
    textpsect->pflg = 0;
    textpsect->p2al = 1;
    textpsect->offs = 0;
    textpsect->size = 0;
    strcpy (textpsect->name, ".50.text");
    psects = textpsect;

    dotsymbol = malloc (1 + sizeof *dotsymbol);
    dotsymbol->next   = NULL;
    dotsymbol->psect  = textpsect;
    dotsymbol->value  = 0;          // invariant: dotsymbol->value == dotsymbol->psect->offs
    dotsymbol->defind = 1;
    dotsymbol->global = 0;
    strcpy (dotsymbol->name, ".");
    symbols = dotsymbol;

    passno = 1;
    doasmpass (asmname);

    dotsymbol->psect = textpsect;
    setdotsymval (0);
    passno  = 2;

    hexname = argv[2];
    hexfile = fopen (hexname, "w");
    if (hexfile == NULL) {
        fprintf (stderr, "error creating %s: %s\n", hexname, strerror (errno));
        return 1;
    }
    for (psect = psects; psect != NULL; psect = psect->next) {
        fprintf (hexfile, ">%s=%04X;pflg=%u;p2al=%u\n", psect->name, psect->size, psect->pflg, psect->p2al);
    }
    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (symbol->global && symbol->defind) {
            fprintf (hexfile, "?%s=%04X", symbol->name, symbol->value);
            if (symbol->psect != NULL) fprintf (hexfile, "+>%s", symbol->psect->name);
            fprintf (hexfile, "\n");
        }
    }

    for (psect = psects; psect != NULL; psect = psect->next) {
        psect->offs = 0;
    }

    doasmpass (asmname);
    fclose (hexfile);

    if (error) unlink (hexname);

    return error;
}

static void doasmpass (char const *asmname)
{
    char asmline[256];
    FILE *asmfile;
    int lineno;
    Token *token, *tokens;

    asmfile = fopen (asmname, "r");
    if (asmfile == NULL) {
        fprintf (stderr, "error opening %s: %s\n", asmname, strerror (errno));
        return;
    }

    lineno = 0;
    while (fgets (asmline, sizeof asmline, asmfile) != NULL) {
        ++ lineno;
        memset (lisaddr, ' ', 4);
        memset (lisdata, ' ', 8);
        tokens = tokenize (asmname, lineno, asmline);
        processline (tokens);
        while ((token = tokens) != NULL) {
            tokens = token->next;
            free (token);
        }
        if (passno == 2) {
            printf (" %s  %s  %6d:%s", lisdata, lisaddr, lineno, asmline);
        }
    }

    fclose (asmfile);
    emitflush ();
}

static Token *tokenize (char const *srcname, int srcline, char const *asmline)
{
    char const *q;
    char c, d, *p;
    int i, j;
    Token **ltoken, *token, *tokens;
    uint32_t v;

    q = asmline;
    ltoken = &tokens;
    while ((c = *q) != 0) {

        // skip whitespace
        if (c <= ' ') {
            ++ q;
            continue;
        }

        // stop if comment
        if (c == ';') break;

        // register
        if (c == '%') {
            d = c = *(++ q);
            if (c != 0) {
                d = *(++ q);
                if (d != 0) ++ q;
            }
            if ((c >= 'a') && (c <= 'z')) c += 'A' - 'a';
            if ((d >= 'a') && (d <= 'z')) d += 'A' - 'a';
            i = -1;
            if ((c == 'R') && (d >= '0') && (d <= '7')) i = d - '0';
            if ((c == 'S') && (d == 'P')) i = 6;
            if ((c == 'P') && (d == 'C')) i = 7;
            if (i < 0) {
                errorloc (srcname, srcline, q - asmline, "bad register %%%c%c", c, d);
                i = 0;
            }
            token = malloc (3 + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline - 3;
            token->toktype = TT_REGISTER;
            token->intval = i;
            token->strval[0] = '%';
            token->strval[1] = c;
            token->strval[2] = d;
            token->strval[3] = 0;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // numeric constant
        if ((c >= '0') && (c <= '9')) {
            v = strtoul (q, &p, 0);
            i = (char const *) p - q;
            token = malloc (i + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = TT_INTEGER;
            token->intval = v;
            memcpy (token->strval, q, i);
            token->strval[i] = 0;
            q = p;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // symbol
        if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '_') || (c == '$') || (c == '.')) {
            for (i = 0;; i ++) {
                c = q[i];
                if ((c >= 'a') && (c <= 'z')) continue;
                if ((c >= 'A') && (c <= 'Z')) continue;
                if ((c >= '0') && (c <= '9')) continue;
                if (c == '_') continue;
                if (c == '$') continue;
                if (c == '.') continue;
                if ((c == ':') && (q[i+1] == ':')) {
                    i ++;
                    continue;
                }
                break;
            }
            token = malloc (i + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = TT_SYMBOL;
            token->intval = i;
            memcpy (token->strval, q, i);
            token->strval[i] = 0;
            q += i;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // delimiter
        if (strchr ("+-&|^~*/%=():#,", c) != NULL) {
            token = malloc (1 + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = TT_DELIM;
            token->intval = c & 0xFF;
            token->strval[0] = c;
            token->strval[1] = 0;
            q ++;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // quoted string
        //  ": null terminated string
        //  ': integer constant
        if ((c == '"') || (c == '\'')) {

            // count string length (might go over a little)
            d = c;
            for (i = 0;;) {
                c = q[++i];
                if (c == 0) break;
                if (c == d) break;
                if ((c == '\\') && (q[++i] == 0)) break;
            }

            // allocate token to take whole string
            token = malloc (i + 1 + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = (d == '"') ? TT_STRING : TT_INTEGER;

            // fill in string characters
            for (i = 0;;) {
                c = *(++ q);
                if (c == 0) break;
                if (c == d) {
                    ++ q;
                    break;
                }
                if (c == '\\') {
                    c = *(++ q);
                    switch (c) {
                        case 'b': c = '\b'; break;
                        case 'n': c = '\n'; break;
                        case 'r': c = '\r'; break;
                        case 't': c = '\t'; break;
                        case 'z': c =    0; break;
                        case '0' ... '7': {
                            c -= '0';
                            j = 1;
                            do {
                                d = *(++ q);
                                if ((d < '0') || (d > '7')) {
                                    -- q;
                                    break;
                                }
                                c = (c << 3) + d - '0';
                            } while (++ j < 3);
                            break;
                        }
                    }
                }
                token->strval[i++] = c;
            }
            token->strval[i] = 0;

            // intval for TT_INTEGER is first two characters, null extended
            // intval for TT_STRING is string length
            if (token->toktype == TT_STRING) {
                token->intval = i;
            } else {
                if (i > 2) errorloc (srcname, srcline, q - asmline, "string too long for integer constant");
                token->strval[i+1] = 0;
                token->intval = ((token->strval[1] & 0xFF) << 8) | (token->strval[0] & 0xFF);
            }

            // link on end of tokens
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // who knows
        errorloc (srcname, srcline, q - asmline, "unknown char %c", c);
        break;
    }

    token = malloc (sizeof *token);
    token->srcname = srcname;
    token->srcline = srcline;
    token->srcchar = q - asmline;
    token->next = NULL;
    token->toktype = TT_ENDLINE;
    token->intval = 0;
    token->strval[0] = 0;
    *ltoken = token;

    return tokens;
}

static void processline (Token *token)
{
    char const *mne;
    char needscomma;
    char *p;
    int i, j, k, m;
    Opcode const *opc;
    PSect *exprpsect, **lpsect, *psect;
    Symbol *exprundsym;
    Token *tok;
    uint16_t word;
    uint32_t exprvalue;

    // process all <symbol>: at beginning of line
    while ((token->toktype == TT_SYMBOL) && (token->next->toktype == TT_DELIM) && (token->next->intval == ':')) {
        if (ifblock > 0) return;
        definesymbol (token, token->strval, dotsymbol->psect, dotsymbol->value);
        fillhex (4, lisaddr + 4, dotsymbol->value);
        token = token->next->next;
    }

    // skip otherwise blank lines
    if (token->toktype == TT_ENDLINE) return;

    // check for <symbol> = <expression>
    if ((token->toktype == TT_SYMBOL) && (token->next->toktype == TT_DELIM) && (token->next->intval == '=')) {
        if (ifblock > 0) return;
        tok = decodexpr (token->next->next, &exprpsect, &exprvalue, &exprundsym);
        if (exprundsym != NULL) errortok (tok, "cannot define one symbol off another undefined symbol %s", exprundsym->name);
        definesymbol (token, token->strval, exprpsect, (uint16_t) exprvalue);
        fillhex (4, lisdata + 8, exprvalue);
        if (tok->toktype != TT_ENDLINE) errortok (tok, "extra at end of expression %s", tok->strval);
        return;
    }

    // should have an opcode or directive
    if (token->toktype != TT_SYMBOL) {
        errortok (token, "expecting directive/opcode at %s", token->strval);
        return;
    }
    mne = token->strval;

    // if blocked by an .if, just look for .ifs, .elses, and .endifs
    if (ifblock > 0) {
        if (strcmp (mne, ".else") == 0) {
            if (iflevel == ifblock) ifblock = 0;
        }
        if (strcmp (mne, ".endif") == 0) {
            if (-- iflevel < ifblock) ifblock = 0;
        }
        if (strcmp (mne, ".if") == 0) {
            ++ iflevel;
        }
        return;
    }

    // check out directives
    if (mne[0] == '.') {

        if (strcmp (mne, ".align") == 0) {
            emitflush ();
            token = decodexpr (token->next, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting end of line");
            }
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "alignment must be absolute");
            if ((exprvalue == 0) || ((exprvalue & - exprvalue) != exprvalue)) {
                errortok (token, "alignment 0x%X must be a power of 2", exprvalue);
                exprvalue = 1;
            }
            uint16_t p2 = 0;
            while (!(exprvalue & 1)) {
                p2 ++;
                exprvalue >>= 1;
            }
            if ((dotsymbol->psect != NULL) && (dotsymbol->psect->p2al < p2)) {
                dotsymbol->psect->p2al = p2;
            }
            exprvalue = (1 << p2) - 1;
            emitflush ();
            setdotsymval ((uint16_t) ((dotsymbol->value + exprvalue) & ~ exprvalue));
            return;
        }

        if (strcmp (mne, ".ascii") == 0) {
            processasciidirective (token);
            return;
        }

        if (strcmp (mne, ".asciz") == 0) {
            p = processasciidirective (token);
            emitbyte (0);
            if (p >= lisdata + 2) {
                *(-- p) = '0';
                *(-- p) = '0';
            }
            return;
        }

        if (strcmp (mne, ".blkb") == 0) {
            emitflush ();
            fillhex (4, lisaddr + 4, dotsymbol->value);
            token = token->next;
            token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting end of line");
            }
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "block sizes must be absolute");
            fillhex (4, lisdata + 8, exprvalue);
            emitflush ();
            setdotsymval ((uint16_t) (dotsymbol->value + exprvalue));
            return;
        }

        if (strcmp (mne, ".byte") == 0) {
            fillhex (4, lisaddr + 4, dotsymbol->value);
            p = lisdata + 8;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "byte values must be absolute");
                if ((exprvalue > 0x000000FF) && (exprvalue < 0xFFFFFF80)) errortok (token, "byte value 0x%X out of range", exprvalue);
                emitbyte ((uint8_t) exprvalue);
                if (p >= lisdata + 2) p = fillhex (2, p, exprvalue);
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between byte values");
                    return;
                }
            }
        }

        if (strcmp (mne, ".else") == 0) {
            if (iflevel <= 0) {
                errortok (token, ".else without corresponding .if");
            }
            ifblock = iflevel;
            return;
        }

        if (strcmp (mne, ".endif") == 0) {
            if (iflevel <= 0) {
                errortok (token, ".endif without corresponding .if");
            } else {
                -- iflevel;
            }
            return;
        }

        if (strcmp (mne, ".extern") == 0) {
            return;
        }

        if (strcmp (mne, ".global") == 0) {
            Symbol *symbol;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                if (token->toktype != TT_SYMBOL) {
                    errortok (token, "expecting global symbol name");
                }
                for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
                    if (strcmp (symbol->name, token->strval) == 0) goto gotgblsym;
                }
                symbol = malloc (strlen (token->strval) + sizeof *symbol);
                symbol->next   = symbols;
                symbol->psect  = NULL;
                symbol->value  = 0;
                symbol->defind = 0;
                strcpy (symbol->name, token->strval);
                symbols = symbol;
            gotgblsym:;
                symbol->global = 1;
                token = token->next;
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between global symbols");
                    return;
                }
            }
            return;
        }

        if (strcmp (mne, ".if") == 0) {
            token = token->next;
            token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, ".if values must be absolute");
            ++ iflevel;
            if (exprvalue == 0) ifblock = iflevel;
            return;
        }

        if (strcmp (mne, ".include") == 0) {
            token = token->next;
            if ((token->toktype != TT_STRING) || (token->next->toktype != TT_ENDLINE)) {
                errortok (token, "expecting filename string then end of line for .include");
                return;
            }
            doasmpass (token->strval);
            return;
        }

        if (strcmp (mne, ".long") == 0) {
            fillhex (4, lisaddr + 4, dotsymbol->value);
            p = lisdata + 8;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "long values must be absolute");
                emitlong (exprvalue);
                if (p >= lisdata + 8) p = fillhex (8, p, exprvalue);
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between long values");
                    return;
                }
            }
        }

        if (strcmp (mne, ".p2align") == 0) {
            emitflush ();
            token = decodexpr (token->next, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting end of line");
            }
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "alignment must be absolute");
            if (exprvalue > 16) {
                errortok (token, "alignment %u must be le 16", exprvalue);
                exprvalue = 0;
            }
            if ((dotsymbol->psect != NULL) && (dotsymbol->psect->p2al < exprvalue)) {
                dotsymbol->psect->p2al = (uint16_t) exprvalue;
            }
            exprvalue = (1 << exprvalue) - 1;
            emitflush ();
            setdotsymval ((uint16_t) ((dotsymbol->value + exprvalue) & ~ exprvalue));
            return;
        }

        if (strcmp (mne, ".psect") == 0) {
            token = token->next;
            if (token->toktype != TT_SYMBOL) {
                errortok (token, ".psect must be followed by psect name");
                return;
            }
            for (lpsect = &psects; (psect = *lpsect) != NULL; lpsect = &psect->next) {
                if (strcmp (psect->name, token->strval) == 0) goto gotpsect;
            }
            psect = malloc ((uint16_t) token->intval + sizeof *psect);
            psect->next = NULL;
            psect->pflg = 0;
            psect->p2al = 0;
            psect->offs = 0;
            psect->size = 0;
            strcpy (psect->name, token->strval);
            *lpsect = psect;
            while (((token = token->next)->toktype == TT_DELIM) && (token->intval == ',')) {
                token = token->next;
                if ((token->toktype != TT_SYMBOL) || (strcasecmp (token->strval, "ovr") != 0)) break;
                psect->pflg |= PF_OVR;
            }
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "invalid .psect option");
            }
        gotpsect:;
            emitflush ();
            dotsymbol->psect = psect;
            dotsymbol->value = psect->offs;
            return;
        }

        if (strcmp (mne, ".word") == 0) {
            fillhex (4, lisaddr + 4, dotsymbol->value);
            p = lisdata + 8;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                emitwordrel ((uint16_t) exprvalue, exprpsect, exprundsym);
                if (p >= lisdata + 4) p = fillhex (4, p, exprvalue);
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between word values (have toktype %d, intval %d)", (int) token->toktype, (int) token->intval);
                    return;
                }
            }
        }

        errortok (token, "unknown directive %s", mne);
        return;
    }

    // check out opcodes
    i = 0;
    j = sizeof opcodelist / sizeof opcodelist[0];
    do {
        k = (i + j) / 2;
        opc = &opcodelist[k];
        m = strcmp (mne, opc->mne);
        if (m < 0) { j = k - 1; continue; }
        if (m > 0) { i = k + 1; continue; }
        goto gotit;
    } while (j >= i);
    errortok (token, "unknown opcode %s", mne);
    return;
gotit:;
    fillhex (4, lisaddr + 4, dotsymbol->value);
    word = opc->opc;
    needscomma = 0;
    if (opc->args & ARG_REGD) {
        token = token->next;
        if (token->toktype != TT_REGISTER) {
            errortok (token, "expecting register");
            return;
        }
        word |= token->intval << IR_REGD;
        needscomma = 1;
    }
    if (opc->args & ARG_LSOF) {
        token = token->next;
        if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
            errortok (token, "expecting comma");
            return;
        }
        token = token->next;
        if ((token->toktype == TT_DELIM) && (token->intval == '#')) {
            if (! (opc->args & ARG_LDIM)) {
                errortok (token, "immediate mode not allowed");
            }
            token = decodexpr (token->next, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting eol at %s", token->strval);
            }
            word |= 7 << IR_REGA;
            emitword (word);
            emitwordrel ((uint16_t) exprvalue, exprpsect, exprundsym);
            fillhex (4, lisdata + 8, word);
            fillhex (4, lisdata + 4, exprvalue);
            return;
        }
        token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
        if (exprundsym != NULL) {
            errortok (token, "undefined symbol '%s'", exprundsym->name);
        }
        if (token->toktype == TT_ENDLINE) {
            if (exprpsect != dotsymbol->psect) {
                errortok (token, "expression must be %%pc relative");
            }
            exprvalue -= dotsymbol->value + 2;
            word |= 7 << IR_REGA;   // rega = %pc
        } else if (
                (token->toktype == TT_DELIM) && (token->intval == '(') &&
                (token->next->toktype == TT_REGISTER) &&
                (token->next->next->toktype == TT_DELIM) && (token->next->next->intval == ')') &&
                (token->next->next->next->toktype == TT_ENDLINE)) {
            if (exprpsect != NULL) {
                errortok (token, "expression must be absolute");
            }
            word |= token->next->intval << IR_REGA;
        } else {
            errortok (token, "expecting eol or (%%reg)");
        }
        exprvalue &= 0xFFFF;
        if ((exprvalue > 0x003F) && (exprvalue < 0xFFC0)) {
            errortok (token, "load/store offset value 0x%04X out of range", exprvalue);
        }
        word |= exprvalue & 0x007F;
        goto done;
    }
    if (opc->args & ARG_REGA) {
        token = token->next;
        if (needscomma) {
            if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                errortok (token, "expecting ,%%reg");
                goto done;
            }
            token = token->next;
        }
        if (token->toktype != TT_REGISTER) {
            errortok (token, "expecting %%reg");
            goto done;
        }
        word |= token->intval << IR_REGA;
        needscomma = 1;
    }
    if (opc->args & (ARG_REGB | ARG_RGAB)) {
        token = token->next;
        if (needscomma) {
            if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                errortok (token, "expecting ,%%reg");
                goto done;
            }
            token = token->next;
        }
        if (token->toktype != TT_REGISTER) {
            errortok (token, "expecting %%reg");
            goto done;
        }
        word |= token->intval << IR_REGB;
        if (opc->args & ARG_RGAB) {
            word |= token->intval << IR_REGA;
        }
        needscomma = 1;
    }
    if (opc->args & ARG_BROF) {
        token = token->next;
        token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
        if (exprundsym != NULL) {
            errortok (token, "undefined symbol '%s'", exprundsym->name);
        }
        if (token->toktype != TT_ENDLINE) {
            errortok (token, "expecting end of line");
        }
        if ((exprpsect != dotsymbol->psect) || (exprundsym != NULL)) {
            errortok (token, "expression must be %%pc relative");
        }
        exprvalue -= dotsymbol->value + 2;
        if ((exprvalue > 511) && (exprvalue < (uint32_t)-512)) {
            errortok (token, "branch offset %d out of range", exprvalue);
        }
        if (exprvalue & 1) {
            errortok (token, "branch offset %04X odd", exprvalue);
        }
        word |= exprvalue & 0x03FE;
        goto done;
    }
    token = token->next;
    if (token->toktype != TT_ENDLINE) {
        errortok (token, "expecting eol at %s", token->strval);
    }
done:;
    emitword (word);
    fillhex (4, lisdata + 8, word);
}

static char *processasciidirective (Token *token)
{
    char *p;
    uint16_t i;

    fillhex (4, lisaddr + 4, dotsymbol->value);
    p = lisdata + 8;
    while (1) {
        token = token->next;
        switch (token->toktype) {
            case TT_STRING: {
                for (i = 0; i < token->intval; i ++) {
                    emitbyte (token->strval[i]);
                    if (p >= lisdata + 2) p = fillhex (2, p, token->strval[i]);
                }
                break;
            }
            case TT_INTEGER: {
                if (token->intval > 0xFF) errortok (token, "non-byte value %u", token->intval);
                emitbyte ((uint8_t) token->intval);
                if (p >= lisdata + 2) p = fillhex (2, p, token->strval[i]);
                break;
            }
            case TT_ENDLINE: {
                return p;
            }
            default: {
                errortok (token, "non-integer non-string %s", token->strval);
                return p;
            }
        }
    }
}

/**
 * Define symbol, replace previous definition if exists
 */
static void definesymbol (Token *token, char const *name, PSect *psect, uint16_t value)
{
    Symbol *symbol;

    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (strcmp (symbol->name, name) == 0) {
            if (symbol == dotsymbol) {
                emitflush ();
                symbol->psect = psect;
                setdotsymval (value);
            } else if (symbol->defind && ((symbol->psect != psect) || (symbol->value != value))) {
                errortok (token, "multiple definitions of symbol '%s'", name);
                fprintf (stderr, "  old value %04X psect %s\n", symbol->value, (symbol->psect == NULL) ? "*abs*" : (char const *) symbol->psect->name);
                fprintf (stderr, "  new value %04X psect %s\n",         value, (        psect == NULL) ? "*abs*" : (char const *)         psect->name);
            }
            symbol->psect  = psect;
            symbol->value  = value;
            symbol->defind = 1;
            return;
        }
    }

    symbol = malloc (strlen (name) + sizeof *symbol);
    symbol->next   = symbols;
    symbol->psect  = psect;
    symbol->value  = value;
    symbol->defind = 1;
    symbol->global = 0;
    strcpy (symbol->name, name);
    symbols = symbol;
}

/**
 * Decode expression
 *  Input:
 *   token = starting token of expression
 *  Output:
 *   returns token past last one in expression
 *   *psect_r  = relocation or NULL if absolute
 *   *value_r  = offset in psect or absolute value
 *   *undsym_r = undefined symbol or NULL if none
 */
static Token *decodexpr (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r)
{
    char const *p;
    char preced;
    Stack *stack, *stackent;

    // get first value in expression
    stack = malloc (sizeof *stack);
    stack->next   = NULL;
    stack->preced = 0;
    stack->opctok = NULL;
    token = decodeval (token, &stack->psect, &stack->value, &stack->undsym);

    while ((token->toktype == TT_DELIM) && (token->intval <= 0xFF)) {
        p = strchr (precedence, (char) token->intval);
        if (p == NULL) break;
        preced = (char) (p - precedence) / 4;
        stackent = malloc (sizeof *stackent);
        stackent->next   = stack;
        stackent->preced = preced + 1;
        stackent->opctok = token;
        token = decodeval (token->next, &stackent->psect, &stackent->value, &stackent->undsym);
        stack = stackent;
        while (stack->preced <= stack->next->preced) {
            stack = dostackop (stack);
        }
    }

    while (stack->next != NULL) {
        stack = dostackop (stack);
    }

    *psect_r  = stack->psect;
    *value_r  = stack->value;
    *undsym_r = stack->undsym;
    free (stack);

    return token;
}

/**
 * Decode value from tokens
 *  Input:
 *   token = value token
 *  Output:
 *   returns token after value
 *   *psect_r = relocation or NULL if absolute
 *   *value_r = offset in psect or absolute value
 */
static Token *decodeval (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r)
{
    Symbol *symbol;

    *psect_r  = NULL;
    *value_r  = 0;
    *undsym_r = NULL;

    switch (token->toktype) {
        case TT_INTEGER: {
            *value_r = token->intval;
            break;
        }
        case TT_SYMBOL: {
            for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
                if (strcmp (token->strval, symbol->name) == 0) goto gotsym;
            }
            symbol = malloc (strlen (token->strval) + sizeof *symbol);
            symbol->next   = symbols;
            symbol->psect  = NULL;
            symbol->value  = 0;
            symbol->defind = 0;
            symbol->global = 0;
            strcpy (symbol->name, token->strval);
            symbols = symbol;
            *undsym_r = symbol;
            break;
        gotsym:;
            if (symbol->defind) {
                *psect_r  = symbol->psect;
                *value_r  = symbol->value;
            } else {
                *undsym_r = symbol;
            }
            break;
        }
        case TT_DELIM: {
            switch (token->intval) {
                case '(': {
                    token = decodexpr (token->next, psect_r, value_r, undsym_r);
                    if ((token->toktype != TT_DELIM) || (token->intval != ')')) {
                        errortok (token, "expecting ) at end of ( expression");
                    } else {
                        token = token->next;
                    }
                    return token;
                }
                case '-': {
                    token = decodeval (token->next, psect_r, value_r, undsym_r);
                    if (*psect_r != NULL) {
                        errortok (token, "cannot negate a relocatable symbol");
                    }
                    if (*undsym_r != NULL) {
                        errortok (token, "cannot negate an undefined symbol");
                    }
                    *value_r = - *value_r;
                    return token;
                }
                case '~': {
                    token = decodeval (token->next, psect_r, value_r, undsym_r);
                    if (*psect_r != NULL) {
                        errortok (token, "cannot complement a relocatable symbol");
                    }
                    if (*undsym_r != NULL) {
                        errortok (token, "cannot complement an undefined symbol");
                    }
                    *value_r = ~ *value_r;
                    return token;
                }
            }
            // fallthrough
        }
        default: {
            errortok (token, "expecting integer, symbol or ( expr )");
            return token;
        }
    }

    return token->next;
}

/**
 * Do arithmetic on top two stack elements
 */
static Stack *dostackop (Stack *stackb)
{
    char opchr;
    Stack *stacka;
    Token *optok;

    stacka = stackb->next;
    optok  = stackb->opctok;
    opchr  = (char) optok->intval;

    switch (opchr) {
        case '|':
        case '^':
        case '&':
        case '*':
        case '/':
        case '%': {
            if ((stacka->psect != NULL) || (stackb->psect != NULL) || (stacka->undsym != NULL) || (stackb->undsym != NULL)) {
                errortok (optok, "operator %c cannot have any relocatables", opchr);
            }
            switch (opchr) {
                case '|': stacka->value |= stackb->value; break;
                case '^': stacka->value ^= stackb->value; break;
                case '&': stacka->value &= stackb->value; break;
                case '*': stacka->value *= stackb->value; break;
                case '/': {
                    if (stackb->value == 0) {
                        errortok (optok, "divide by zero");
                    } else {
                        stacka->value /= stackb->value;
                    }
                    break;
                }
                case '%': {
                    if (stackb->value == 0) {
                        errortok (optok, "divide by zero");
                    } else {
                        stacka->value %= stackb->value;
                    }
                    break;
                }
            }
            break;
        }
        case '+': {
            if (stackb->psect != NULL) {
                if (stacka->psect != NULL) {
                    errortok (optok, "operator + cannot have two relocatables");
                } else {
                    stacka->psect = stackb->psect;
                }
            }
            if (stackb->undsym != NULL) {
                if (stacka->undsym != NULL) {
                    errortok (optok, "operator + cannot have two undefineds");
                } else {
                    stacka->undsym = stackb->undsym;
                }
            }
            stacka->value += stackb->value;
            break;
        }
        case '-': {
            if (stackb->psect != NULL) {
                if (stackb->psect != stacka->psect) {
                    errortok (optok, "operator - cannot have two different relocatables");
                }
                stacka->psect = NULL;
            }
            if (stackb->undsym != NULL) {
                if (stackb->undsym != stacka->undsym) {
                    errortok (optok, "operator - cannot have two different undefineds");
                }
                stacka->undsym = NULL;
            }
            stacka->value -= stackb->value;
            break;
        }
        default: abort ();
    }
    free (stackb);
    return stacka;
}

/**
 * Output object code
 */
static void emitbyte (uint8_t b)
{
    if (emitidx == sizeof emitbuf) {
        emitflush ();
    }
    emitbuf[emitidx++] = b;
    setdotsymval (dotsymbol->value + 1);
}

static void emitlong (uint32_t w)
{
    if (emitidx + 4 > sizeof emitbuf) {
        emitflush ();
    }
    emitbuf[emitidx++] = (uint8_t) w;
    emitbuf[emitidx++] = (uint8_t) (w >>  8);
    emitbuf[emitidx++] = (uint8_t) (w >> 16);
    emitbuf[emitidx++] = (uint8_t) (w >> 24);
    setdotsymval (dotsymbol->value + 4);
}

static void emitword (uint16_t w)
{
    if (emitidx + 2 > sizeof emitbuf) {
        emitflush ();
    }
    emitbuf[emitidx++] = (uint8_t) w;
    emitbuf[emitidx++] = (uint8_t) (w >> 8);
    setdotsymval (dotsymbol->value + 2);
}

static void emitwordrel (uint16_t w, PSect *psect, Symbol *undsym)
{
    if ((psect == NULL) && (undsym == NULL)) {
        emitword (w);
    } else {
        emitflush ();
        if (passno == 2) {
            fprintf (hexfile, "%04X", dotsymbol->value);
            if (dotsymbol->psect != NULL) {
                fprintf (hexfile, "+>%s", dotsymbol->psect->name);
            }
            fprintf (hexfile, "=(%04X", w);
            if (psect != NULL) {
                fprintf (hexfile, "+>%s", psect->name);
            }
            if (undsym != NULL) {
                fprintf (hexfile, "+?%s", undsym->name);
            }
            fprintf (hexfile, ")\n");
        }
        setdotsymval (dotsymbol->value + 2);
    }
}

static void emitflush ()
{
    int i;
    uint16_t addr;

    if ((passno == 2) && (emitidx > 0)) {
        addr = dotsymbol->value - emitidx;
        if (dotsymbol->psect == NULL) {
            fprintf (hexfile, "%04X=", addr);
        } else {
            fprintf (hexfile, "%04X+>%s=", addr, dotsymbol->psect->name);
        }
        for (i = 0; i < emitidx; i ++) {
            fprintf (hexfile, "%02X", emitbuf[i] & 0xFF);
        }
        fprintf (hexfile, "\n");
    }
    emitidx = 0;
}

/**
 * Set value of the "." symbol,
 * ie, address where the next object code goes
 */
static void setdotsymval (uint16_t val)
{
    PSect *psect;
    psect = dotsymbol->psect;
    dotsymbol->value = psect->offs = val;
    if (psect->size < val) psect->size = val;
}

/**
 * Fill buffer with hexadecimal characters
 *  Input:
 *   len = number of hex chars to output
 *   buf = just past end of string
 *   val = value to convert
 *  Output:
 *   returns pointer to first char
 */
static char *fillhex (int len, char *buf, uint32_t val)
{
    char j;
    while (-- len >= 0) {
        j = (char) val & 15;
        *(-- buf) = (j < 10) ? (j + '0') : (j - 10 + 'A');
        val >>= 4;
    }
    return buf;
}

/**
 * Print error message
 */
static void errortok (Token *tok, char const *fmt, ...)
{
    va_list ap;

    if (passno == 2) {
        fprintf (stderr, "%s:%d.%d:", tok->srcname, tok->srcline, tok->srcchar);
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
        fprintf (stderr, "\n");
        error = 1;
    }
}

static void errorloc (char const *srcname, int srcline, int srcchar, char const *fmt, ...)
{
    va_list ap;

    if (passno == 2) {
        fprintf (stderr, "%s:%d.%d:", srcname, srcline, srcchar);
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
        fprintf (stderr, "\n");
        error = 1;
    }
}
