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
 * Program runs on any computer to test a physical register board.
 * Requires four IOW56Paddles connected to the A,C,I,D connectors.
 * Can test up to all four register boards at once.
 * sudo insmod km/enabtsc.ko
 * sudo ./regtester [-cpuhz 200] [-loop] [01] [23] [45] [67]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "miscdefs.h"

static bool regexists[8];
static bool regknowns[8];
static GpioLib *gpio;
static int errors;
static uint16_t regvalues[8];
static uint32_t ranum, rbnum, rdnum;

static void verifycontents ();
static void verifyabvals (uint32_t anum, bool aknown, uint16_t avalue, uint32_t bnum, bool bknown, uint16_t bvalue);
static void writeccon (uint32_t pins);
static void writedbus (uint16_t dbus);
static void dopause ();

int main (int argc, char **argv)
{
    bool loopit = false;
    int passno = 0;
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
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        int rpair = atoi (argv[i]);
        if ((rpair < 80) && (rpair % 22 == 1)) {
            rpair /= 11;
            regexists[rpair++] = true;
            regexists[rpair++] = true;
            continue;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access regboard circuitry via paddles
    gpio = new PhysLib (cpuhz);
    gpio->open ();

    writeccon (0);
    gpio->halfcycle ();
    writeccon (C_CLK2);
    gpio->halfcycle ();
    writeccon (0);
    gpio->halfcycle ();

    srand (0);

    do {

        for (int iter = 0; iter < 1000; iter ++) {

            // select random A,B,D registers
            uint16_t ir = rand ();

            ranum = (ir >> REGA) & 7;
            rbnum = (ir >> REGB) & 7;
            rdnum = (ir >> REGD) & 7;
            printf ("%5u.%5u.%5u  RA = %u  RB = %u  RD = %u\n", errors, passno, iter, ranum, rbnum, rdnum);

            uint32_t ir32 = GpioLib::ibusshuf (ir);
            if (! gpio->writecon (CON_I, 0xFFFFFFFF, ir32)) {
                abort ();
            }

            // write random value to D register
            uint16_t randd = rand ();           // get some random value to write to Rd
            writeccon (C_CLK2 | C_GPRS_WED);    // start a cycle saying we want to write Rd
            gpio->halfcycle ();                 // let it soak in
            writeccon (C_GPRS_WED);             // drop CLK so the WED locks in wevn/wodd dff
            writedbus (randd);                  // put value to be written on dbus
            gpio->halfcycle ();                 // let it soak into Rd's dff inputs
            writeccon (C_CLK2);                 // clock it into Rd's dffs
            gpio->halfcycle ();                 // give it time to soak thru to Rd's dff outputs
            writedbus (rand ());                // some other rand value in case it keeps writing

            printf ("                   wrote R%u = %04X\n", rdnum, randd);
            regknowns[rdnum] = true;
            regvalues[rdnum] = randd;

            verifycontents ();

            // write random value to register 7
            uint16_t rand7 = rand ();           // get some random value to write to R7
            writeccon (C_CLK2 | C_GPRS_WE7);    // start a cycle saying we want to write R7
            gpio->halfcycle ();                 // let it soak in
            writeccon (C_GPRS_WE7);             // drop CLK so the WE7 locks in wevn/wodd dff
            writedbus (rand7);                  // put value to be written on dbus
            gpio->halfcycle ();                 // let it soak into R7's dff inputs
            writeccon (C_CLK2);                 // clock it into R7's dffs
            gpio->halfcycle ();                 // give it time to soak thru to R7's dff outputs
            writedbus (rand ());                // some other rand value in case it keeps writing

            printf ("                   wrote PC = %04X\n", rand7);
            regknowns[7] = true;
            regvalues[7] = rand7;

            verifycontents ();
        }

        // all done
        time_t nowbin = time (NULL);
        printf ("PASS %d  %s\n", ++ passno, ctime (&nowbin));

        // maybe keep doing it
    } while (loopit);

    return 0;
}

static void verifycontents ()
{
    // nothing being read, should have all ones on both busses
    verifyabvals (9, true, 0xFFFF, 9, true, 0xFFFF);

    // gate Ra onto abus, check that we get the known value if any
    // if board is not present, should have all ones
    // in any case the bbus should be all ones
    writeccon (C_GPRS_REA);
    gpio->halfcycle ();
    verifyabvals (
        regexists[ranum] ? ranum : 9, 
        regexists[ranum] ? regknowns[ranum] : true,
        regexists[ranum] ? regvalues[ranum] : 0xFFFF,
        9, true, 0xFFFF);

    // gate R7 onto abus, check that we have the known value if any
    // if board is not present, should have all ones
    // in any case the bbus should be all ones
    writeccon (C_GPRS_REA7);
    gpio->halfcycle ();
    verifyabvals (
        regexists[7] ? 7 : 9,
        regexists[7] ? regknowns[7] : true,
        regexists[7] ? regvalues[7] : 0xFFFF,
        9, true, 0xFFFF);

    // gate Rb onto bbus, check that we have the known value if any
    // if board is not present, should have all ones
    // in any case the abus should be all ones
    writeccon (C_GPRS_REB);
    gpio->halfcycle ();
    verifyabvals (
        9, true, 0xFFFF,
        regexists[rbnum] ? rbnum : 9,
        regexists[rbnum] ? regknowns[rbnum] : true,
        regexists[rbnum] ? regvalues[rbnum] : 0xFFFF);

    // gate Rd onto bbus, check that we have the known value if any
    // if board is not present, should have all ones
    // in any case the abus should be all ones
    writeccon (C_GPRS_RED2B);
    gpio->halfcycle ();
    verifyabvals (
        9, true, 0xFFFF,
        regexists[rdnum] ? rdnum : 9,
        regexists[rdnum] ? regknowns[rdnum] : true,
        regexists[rdnum] ? regvalues[rdnum] : 0xFFFF);
}

// verify abus and bbus values
static void verifyabvals (uint32_t anum, bool aknown, uint16_t avalue, uint32_t bnum, bool bknown, uint16_t bvalue)
{
    if (aknown | bknown) {
        uint32_t acon;
        if (! gpio->readcon (CON_A, &acon)) {
            abort ();
        }
        if (aknown) {
            uint16_t ais = GpioLib::abussort (acon);
            if (ais != avalue) {
                printf ("bad R%u avalue is %04X, sb %04X, diff %04X\n", anum, ais, avalue, ais ^ avalue);
                errors ++;
                dopause ();
            } else {
                //printf ("good R%u avalue %04X\n", anum, ais);
            }
        }
        if (bknown) {
            uint16_t bis = GpioLib::bbussort (acon);
            if (bis != bvalue) {
                printf ("bad R%u bvalue is %04X, sb %04X, diff %04X\n", bnum, bis, bvalue, bis ^ bvalue);
                errors ++;
                dopause ();
            } else {
                //printf ("good R%u avalue %04X\n", bnum, bis);
            }
        }
    }
}

// write control output pins that regboards care about
// all other ccon pins get random values
static void writeccon (uint32_t pins)
{
    uint32_t mask = C_CLK2 | C_GPRS_WED | C_GPRS_WE7 | C_GPRS_REA | C_GPRS_REB | C_GPRS_RED2B | C_GPRS_REA7;
    ASSERT ((pins & ~ mask) == 0);
    pins |= rand () & ~ mask;
    if (! gpio->writecon (CON_C, 0xFFFFFFFF, pins)) {
        abort ();
    }
}

// write dbus value that goes to all register dff inputs
// all other dcon pins get random values
static void writedbus (uint16_t dbus)
{
    ASSERT (D_DBUS == 0xFFFF);
    uint32_t pins = GpioLib::dbusshuf (dbus) | (rand () << 16);
    if (! gpio->writecon (CON_D, 0xFFFFFFFF, pins)) {
        abort ();
    }
}

static void dopause ()
{
    uint32_t gcon, acon, ccon, icon, dcon;

    gcon = gpio->readgpio ();
    printf ("gcon=%08X  %s\n", gcon, GpioLib::decocon (CON_G, gcon).c_str ());

    if (gpio->readcon (CON_A, &acon)) {
        printf ("acon=%08X  %s\n", acon, GpioLib::decocon (CON_A, acon).c_str ());
    } else {
        printf ("acon read error\n");
    }

    if (gpio->readcon (CON_C, &ccon)) {
        printf ("ccon=%08X  %s\n", ccon, GpioLib::decocon (CON_C, ccon).c_str ());
    } else {
        printf ("ccon read error\n");
    }

    if (gpio->readcon (CON_I, &icon)) {
        uint16_t ir = GpioLib::ibussort (icon);
        printf ("icon=%08X  ir=%04X  _ir=%04X\n", icon, ir, GpioLib::_ibussort (icon));
    } else {
        printf ("icon read error\n");
    }

    if (gpio->readcon (CON_D, &dcon)) {
        printf ("dcon=%08X  %s\n", dcon, GpioLib::decocon (CON_D, dcon).c_str ());
    } else {
        printf ("dcon read error\n");
    }

    while (true) {
        char pausebuf[8];
        if (write (0, "pause> ", 7) < 7) {
            fprintf (stderr, "pause> ");
        }
        if (read (0, pausebuf, sizeof pausebuf) <= 0) exit (0);
        switch (pausebuf[0]) {
            case '\n': return;
            case 'c': {
                break;
            }
            case 'i': {
                break;
            }
        }
    }
}
