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
/**
 * Program runs on any computer to test a physical alu board.
 * Requires four IOW56Paddles connected to the A,C,I,D connectors.
 * sudo insmod km/enabtsc.ko
 * sudo ./alutester -cpuhz 200 -loop {lo,hi} -pauseonerror

    what we send to ALU board:

            hi          lo
        abus[15:08] abus[07:00]
        bbus[15:08] bbus[07:00]
        rawcmid     rawcin
        rawshrin    rawshrmid

                both
            irbus[03:00]
            alu_add
            alu_irfc
            alu_bonly

    what we get from ALU board:

            hi          lo
        dbus[15:08] dbus[07:00]
        rawcout     rawcmid
        rawvout
        rawshrout   rawshrmid
 */

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "alu8.h"
#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"

static bool pauseonerror;
static char const *lohi;
static GpioLib *gpio;

static void writeacon (uint16_t abus, uint16_t bbus);
static void writeccon (uint32_t pins);
static void writeicon (uint16_t irbus);
static void writedcon (uint32_t pins);
static uint32_t readdcon ();
static void dopause ();

int main (int argc, char **argv)
{
    bool loopit = false;
    uint32_t cpuhz = DEFCPUHZ;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-cpuhz") == 0) {
            if (++ i >= argc) {
                fprintf (stderr, "-cpuhz missing freq\n");
                return 1;
            }
            cpuhz = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-loop") == 0) {
            loopit = true;
            continue;
        }
        if (strcasecmp (argv[i], "-pauseonerror") == 0) {
            pauseonerror = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        if (lohi != NULL) {
            fprintf (stderr, "unknown argument %s\n", argv[i]);
            return 1;
        }
        lohi = argv[i];
    }
    if ((lohi == NULL) || ((strcasecmp (lohi, "lo") != 0) && (strcasecmp (lohi, "hi") != 0))) {
        fprintf (stderr, "missing or bad lo/hi argument\n");
        return 1;
    }

    // access seqboard circuitry via paddles
    gpio = new PhysLib (cpuhz);
    gpio->open ();

    srand (0);

    int count  = 0;
    int errors = 0;
    int passno = 0;

    do {

        for (int iter = 0; iter < 1000; iter ++) {

            // make up some random operands
            int randno = rand ();
            uint16_t ir = randno;
            int op = (randno >> 16) & 7;
            uint32_t ctlpins;
            switch (op) {
                case 0:  ctlpins = C_ALU_ADD;   break;
                case 1:  ctlpins = C_ALU_BONLY; break;
                default: ctlpins = C_ALU_IRFC;  break;
            }
            uint16_t abus  = rand ();
            uint16_t bbus  = rand ();
            uint32_t dpins = rand ();

            // send random numbers to board
            writeacon (abus, bbus);     // two 16-bit numbers
            writeccon (ctlpins);        // one of ADD,BONLY,IRFC
            writeicon (ir);             // only [03:00] matter
            writedcon (dpins);          // random carry inputs (lo: RAWCIN,RAWSHRMIDI; hi: RAWCMIDI,RAWSHRIN)

            // let it soak through circuits
            // board is all combinational so no clocking needed
            gpio->halfcycle ();

            // get the 8-bit operands the board should be operating on
            ALU8 alu;
            if (lohi[0] & 4) {
                alu.ain   = abus;
                alu.bin   = bbus;
                alu.cin   = (dpins & D_RAWCIN)     != 0;
                alu.shrin = (dpins & D_RAWSHRMIDI) != 0;
            } else {
                alu.ain   = abus >> 8;
                alu.bin   = bbus >> 8;
                alu.cin   = (dpins & D_RAWCMIDI)   != 0;
                alu.shrin = (dpins & D_RAWSHRIN)   != 0;
            }
            alu.ctlpins = ctlpins;
            alu.ir      = ir;

            // do the 8-bit arithmetic that the board should be doing
            alu.compute ();

            // read what the board is outputting
            uint32_t dconis = readdcon ();
            uint16_t dbusis = GpioLib::dbussort (dconis);

            // compare the two values
            count ++;
            uint8_t doutis;
            bool coutis, voutis, shroutis, ok;
            if (lohi[0] & 4) {
                doutis   = dbusis;
                coutis   = (dconis / D_RAWCMIDO) & 1;
                voutis   = alu.vout;
                shroutis = (dconis / D_RAWSHROUT) & 1;

                ok = (doutis == alu.dout) && (coutis == alu.cout) && (shroutis == alu.shrout);
                if (! ok) errors ++;

                char astr[5];
                astr[0] = 0;
                if (alu.aena) sprintf (astr, "--%02X", alu.ain);

                printf ("%5d.%7d  %d %4s %c %c --%02X %d  =  %d --%02X %d  sb  %d --%02X %d  %s\n",
                        errors, count, alu.shrin, astr, alu.opchar, (alu.bnot ? '~' : ' '),
                        alu.bin, alu.cin,  coutis, doutis, shroutis,  alu.cout, alu.dout, alu.shrout,
                        ok ? "OK" : "BAD");
            } else {
                doutis   = dbusis >> 8;
                coutis   = (dconis / D_RAWCOUT) & 1;
                voutis   = (dconis / D_RAWVOUT) & 1;
                shroutis = (dconis / D_RAWSHRMIDO) & 1;

                ok = (doutis == alu.dout) && (coutis == alu.cout) && (voutis == alu.vout) && (shroutis == alu.shrout);
                if (! ok) errors ++;

                char astr[5];
                astr[0] = 0;
                if (alu.aena) sprintf (astr, "%02X--", alu.ain);

                printf ("%5d.%7d  %d %4s %c %c %02X-- %d  =  %d %d %02X-- %d  sb  %d %d %02X-- %d  %s\n",
                        errors, count, alu.shrin, astr, alu.opchar, (alu.bnot ? '~' : ' '),
                        alu.bin, alu.cin,  coutis, voutis, doutis, shroutis,  alu.cout, alu.vout, alu.dout, alu.shrout,
                        ok ? "OK" : "BAD");
            }
            if (! ok && pauseonerror) {
                dopause ();
            }
        }

        // all done
        time_t nowbin = time (NULL);
        printf ("ERRORS %5d / PASS %d  %s\n", errors, ++ passno, ctime (&nowbin));

        // maybe keep doing it
    } while (loopit);

    return 0;
}

static void writeacon (uint16_t abus, uint16_t bbus)
{
    uint32_t pins = GpioLib::abusshuf (abus) | GpioLib::bbusshuf (bbus);
    if (! gpio->writecon (CON_A, 0xFFFFFFFF, pins)) {
        abort ();
    }
}

static void writeccon (uint32_t pins)
{
    uint32_t mask = C_ALU_ADD | C_ALU_BONLY | C_ALU_IRFC;
    if (! gpio->writecon (CON_C, mask, pins)) {
        abort ();
    }
}

static void writeicon (uint16_t irbus)
{
    uint32_t pins = GpioLib::ibusshuf (irbus);
    if (! gpio->writecon (CON_I, 0xFFFFFFFF, pins)) {
        abort ();
    }
}

static void writedcon (uint32_t pins)
{
    uint32_t mask;
    if (lohi[0] & 4) {
        mask = D_RAWCIN | D_RAWSHRMIDI;
    } else {
        mask = D_RAWCMIDI | D_RAWSHRIN;
    }
    if (! gpio->writecon (CON_D, mask, pins)) {
        abort ();
    }
}

static uint32_t readdcon ()
{
    uint32_t pins;
    if (! gpio->readcon (CON_D, &pins)) {
        abort ();
    }
    return pins;
}

static void dopause ()
{
    while (true) {
        char pausebuf[8];
        if (write (0, "pause> ", 7) < 7) {
            fprintf (stderr, "pause> ");
        }
        if (read (0, pausebuf, sizeof pausebuf) <= 0) exit (0);
        switch (pausebuf[0]) {
            case '\n': return;
            case 'c': {
                uint32_t ccon;
                if (gpio->readcon (CON_C, &ccon)) {
                    printf ("ccon=%08X  %s\n", ccon, GpioLib::decocon (CON_C, ccon).c_str ());
                } else {
                    printf ("read error\n");
                }
                break;
            }
            case 'i': {
                uint32_t icon;
                if (gpio->readcon (CON_I, &icon)) {
                    printf ("icon=%08X  ir=%04X  _ir=%04X\n", icon, GpioLib::ibussort (icon), GpioLib::_ibussort (icon));
                } else {
                    printf ("read error\n");
                }
                break;
            }
        }
    }
}
