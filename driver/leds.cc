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

#include "gpiolib.h"
#include "leds.h"

#define WIDTH 80
#define HEIGHT 32

#define LIT ('@' | A_BOLD)
#define OUT '-'

struct CLine {
    uint32_t mask;
    char const *name;
};

static CLine const clines[] = {
    { C_PSW_WE,        " PSW_WE"         },
    { C__PSW_REB,      " _PSW_REB"       },
    { C__PSW_IECLR,    " _PSW_IECLR"     },
    { C_PSW_ALUWE,     " PSW_ALUWE"      },
    { C_MEM_WRITE,     " MEM_WRITE"      },
    { C_MEM_WORD,      " MEM_WORD"       },
    { C_MEM_REB,       " MEM_REB"        },
    { C_MEM_READ,      " MEM_READ"       },
    { C__IR_WE,        " _IR_WE"         },
    { C_HALT,          " HALT"           },
    { C_GPRS_WED,      " GPRS_WED"       },
    { C_GPRS_WE7,      " GPRS_WE7"       },
    { C_GPRS_RED2B,    " GPRS_RED2B"     },
    { C_GPRS_REB,      " GPRS_REB"       },
    { C_GPRS_REA7,     " GPRS_REA7"      },
    { C_GPRS_REA,      " GPRS_REA"       },
    { C_BMUX_IRLSOF,   " BMUX_IRLSOF"    },
    { C_BMUX_IRBROF,   " BMUX_IRBROF"    },
    { C_BMUX_CON__4,   " BMUX_CON__4"    },
    { C_BMUX_CON_2,    " BMUX_CON_2"     },
    { C_BMUX_CON_0,    " BMUX_CON_0"     },
    { C_ALU_IRFC,      " ALU_IRFC"       },
    { C_ALU_BONLY,     " ALU_BONLY"      },
    { C_ALU_ADD,       " ALU_ADD"        },
    { C_PSW_IE,        " PSW_IE"         },
    { C_PSW_BT,        " PSW_BT"         },
    { C_RESET,         " RESET"          },
    { C_IRQ,           " IRQ"            },
    { C_CLK2,          " CLK2"           } };

static void abline (char name, int row, int col, uint16_t bits);

LEDS::LEDS (char const *ttyname)
{
    initscr ();
}

LEDS::~LEDS ()
{
    endwin ();
}

void LEDS::update (uint32_t acon, uint32_t ccon, uint32_t icon, uint32_t dcon)
{
    uint16_t abus = GpioLib::abussort (acon);
    uint16_t bbus = GpioLib::bbussort (acon);
    uint16_t ibus = GpioLib::ibussort (icon);
    uint16_t dbus = GpioLib::dbussort (dcon);

    // top: abus, bbus
    abline ('A',  1, 11, abus);
    abline ('B',  1, 35, bbus);

    // right side: controls
    for (int i = 0; i < 29; i ++) {
        move (i + 3, 60);
        addch ((((clines[i].mask & ccon) != 0) ^ (clines[i].name[1] == '_')) ? LIT : OUT);
        addstr (clines[i].name);
    }

    // left side: ibus (lsb on top; msb on bottom)
    for (int i = 0; i < 16; i ++) {
        move (7 + i + (i - 1) / 3, 5);
        addch (((ibus >> i) & 1) ? LIT : OUT);
        char bitno[6];
        sprintf (bitno, " I%02d", i);
        addstr (bitno);
    }

    // bottom: dbus and carries
    abline ('D', 33, 11, dbus);
    addch (' ');
    addch (' ');
    addch ((dcon & D_RAWCOUT)    ? LIT : OUT);
    addch (' ');
    addch ((dcon & D_RAWVOUT)    ? LIT : OUT);
    addch (' ');
    addch ((dcon & D_RAWCMIDO)   ? LIT : OUT);
    addch (' ');
    addch ((dcon & D_RAWCIN)     ? LIT : OUT);
    addch (' ');
    addch (' ');
    addch ((dcon & D_RAWSHRIN)   ? LIT : OUT);
    addch (' ');
    addch ((dcon & D_RAWSHRMIDO) ? LIT : OUT);
    addch (' ');
    addch ((dcon & D_RAWSHROUT)  ? LIT : OUT);
    addch ('\r');

    refresh ();
}

static void abline (char name, int row, int col, uint16_t bits)
{
    move (row, col);
    addch (name);
    addch (' ');

    for (int i = 16; -- i >= 0;) {
        if ((i & 3) == 3) addch (' ');
        addch (((bits >> i) & 1) ? LIT : OUT);
    }
}
