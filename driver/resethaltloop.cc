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

// do loop of RESET0..HALT1 states
// requires whole processor
//  sudo insmod km/enabtsc.ko
//  ./resethaltloop.armv7l [-cpuhz 500000]
// typically run at -cpuhz 500000 with scope attached

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gpiolib.h"
#include "miscdefs.h"

#define ST_RESET0  "RESET0",  0, G_READ | G_WRITE | G_HALT, 0, C_ALU_BONLY | C_MEM_WORD | C_PSW_IE
#define ST_RESET1  "RESET1",  0, G_READ | G_WRITE | G_HALT, C_BMUX_CON_0 | C_ALU_BONLY | C_GPRS_WE7 | C_PSW_WE | C__PSW_IECLR, C_MEM_WORD

#define ST_FETCH1  "FETCH1",  G_WORD | G_READ, G_WRITE | G_HALT, C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_FETCH2  "FETCH2",  0, G_READ | G_WRITE | G_HALT, C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7 | C__IR_WE, C_MEM_WORD | C_PSW_IE

#define ST_BCC1    "BCC1",    0, G_READ | G_WRITE | G_HALT, C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD, C_MEM_WORD | C_GPRS_WE7 | C_PSW_IE

#define ST_ARITH1  "ARITH1",  0, G_READ | G_WRITE | G_HALT, C_GPRS_REA | C_GPRS_REB | C_ALU_IRFC | C_GPRS_WED | C_PSW_ALUWE, C_MEM_WORD | C_PSW_IE

#define ST_STORE1  "STORE1",  G_WRITE, G_READ | G_HALT, C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE, C_PSW_IE | C_MEM_WORD
#define ST_STORE2  "STORE2",  0, G_READ | G_WRITE | G_HALT, C_GPRS_RED2B | C_ALU_BONLY, C_MEM_WORD | C_PSW_IE

#define ST_LDA1    "LDA1",    0, G_READ | G_WRITE | G_HALT, C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_GPRS_WED, C_MEM_WORD | C_PSW_IE

#define ST_LOAD1   "LOAD1",   G_READ, G_WRITE | G_HALT, C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ, C_PSW_IE | C_MEM_WORD
#define ST_LOAD2   "LOAD2",   0, G_READ | G_WRITE | G_HALT, C_MEM_REB | C_ALU_BONLY | C_GPRS_WED, C_PSW_IE | C_MEM_WORD
#define ST_LOAD3   "LOAD3",   0, G_READ | G_WRITE | G_HALT, C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7, C_MEM_WORD | C_PSW_IE

#define ST_IREQ1   "IREQ1",   G_WORD | G_WRITE, G_READ | G_HALT, C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, C_PSW_IE
#define ST_IREQ2   "IREQ2",   0, G_READ | G_WRITE | G_HALT, C__PSW_REB | C_ALU_BONLY, C_MEM_WORD | C_PSW_IE
#define ST_IREQ3   "IREQ3",   G_WORD | G_WRITE, G_READ | G_HALT, C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, C_PSW_IE
#define ST_IREQ4   "IREQ4",   0, G_READ | G_WRITE | G_HALT, C__PSW_IECLR | C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD, C_MEM_WORD
#define ST_IREQ5   "IREQ5",   0, G_READ | G_WRITE | G_HALT, C_BMUX_CON_2 | C_ALU_BONLY | C_GPRS_WE7, C_MEM_WORD

#define ST_HALT1   "HALT1",   G_HALT, G_READ | G_WRITE, C_HALT | C_GPRS_REB | C_ALU_BONLY, C_MEM_WORD | C_PSW_IE

#define ST_IRET1   "IRET1",   G_WORD | G_READ, G_WRITE | G_HALT, C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_IRET2   "IRET2",   0, G_READ | G_WRITE | G_HALT, C_MEM_REB | C_MEM_WORD | C_ALU_BONLY | C_GPRS_WE7, C_PSW_IE
#define ST_IRET3   "IRET3",   G_WORD | G_READ, G_WRITE | G_HALT, C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_IRET4   "IRET4",   0, G_READ | G_WRITE | G_HALT, C_MEM_REB | C_MEM_WORD | C_ALU_BONLY | C_PSW_WE, C_PSW_IE

#define ST_WRPS1   "WRPS1",   0, G_READ | G_WRITE | G_HALT, C_GPRS_REB | C_ALU_BONLY | C_PSW_WE, C_MEM_WORD | C_PSW_IE

#define ST_RDPS1   "RDPS1",   0, G_READ | G_WRITE | G_HALT, C__PSW_REB | C_ALU_BONLY | C_GPRS_WED, C_MEM_WORD | C_PSW_IE

static GpioLib *gpio;
static uint32_t nerrors;
static uint64_t ncycles;

static bool verifystate (char const *name, uint32_t gset, uint32_t gclr, uint32_t cset, uint32_t cign);

int main (int argc, char **argv)
{
    uint32_t cpuhz = DEFCPUHZ;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-cpuhz") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "-cpuhz missing freq\n");
                return 1;
            }
            cpuhz = atoi (argv[i]);
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    gpio = new PhysLib (cpuhz, true);
    gpio->open ();

    uint32_t lasttime = time (NULL);
    uint64_t lastcycs = 0;
    uint16_t opcode   = 0x0070; // HALT %R7

    while (true) {

        uint32_t thistime = time (NULL);
        if (thistime / 60 > lasttime / 60) {
            printf ("%02u:%02u:%02u %4u %13llu cycles  %6llu Hz  %10.3f uS\n",
                        thistime / 3600 % 24, thistime / 60 % 60, thistime % 60,
                        nerrors, ncycles, (ncycles - lastcycs) / (thistime - lasttime),
                        (thistime - lasttime) * 1000000.0 / (ncycles - lastcycs));
            lasttime = thistime;
            lastcycs = ncycles;
        }

        gpio->writegpio (false, G_RESET);
        gpio->halfcycle ();
        gpio->halfcycle ();
        if (! verifystate (ST_RESET0)) continue;

        gpio->writegpio (false, 0);
        gpio->halfcycle ();
        gpio->writegpio (false, G_CLK);
        gpio->halfcycle ();
        if (! verifystate (ST_RESET1)) continue;

        gpio->writegpio (false, 0);
        gpio->halfcycle ();
        gpio->writegpio (false, G_CLK);
        gpio->halfcycle ();
        if (! verifystate (ST_FETCH1)) continue;

        gpio->writegpio (false, 0);
        gpio->halfcycle ();
        gpio->writegpio (false, G_CLK);
        gpio->halfcycle ();
        if (! verifystate (ST_FETCH2)) continue;

        gpio->writegpio (true, opcode * G_DATA0);
        gpio->halfcycle ();
        gpio->writegpio (true, opcode * G_DATA0 | G_CLK);
        gpio->halfcycle ();
        if (! verifystate (ST_HALT1)) continue;
    }

    return 0;
}

static bool verifystate (char const *name, uint32_t gset, uint32_t gclr, uint32_t cset, uint32_t cign)
{
    ncycles ++;
    uint32_t gactual = gpio->readgpio ();
    if ((gactual & (gset | gclr)) == gset) return true;
    gpio->writegpio (false, G_FLAG);
    gpio->halfcycle ();
    gpio->halfcycle ();
    printf ("%13llu should be in state %s\n", ncycles, name);
    if ((gset | gclr) & G_READ)  printf ("              G_READ  is %s expect %s\n", (gactual & G_READ)  ? "set" : "clr", (gset & G_READ)  ? "set" : "clr");
    if ((gset | gclr) & G_WRITE) printf ("              G_WRITE is %s expect %s\n", (gactual & G_WRITE) ? "set" : "clr", (gset & G_WRITE) ? "set" : "clr");
    if ((gset | gclr) & G_WORD)  printf ("              G_WORD  is %s expect %s\n", (gactual & G_WORD)  ? "set" : "clr", (gset & G_WORD)  ? "set" : "clr");
    if ((gset | gclr) & G_HALT)  printf ("              G_HALT  is %s expect %s\n", (gactual & G_HALT)  ? "set" : "clr", (gset & G_HALT)  ? "set" : "clr");
    nerrors ++;
    return false;
}
