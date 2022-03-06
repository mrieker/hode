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

// test speed of RAS,SEQ board combination
// optionally have LED board and C paddle
//  sudo insmod km/enabtsc.ko
//  . ./iow56sns.si
//  sudo -E ./raseqspeed [-cpuhz 200]

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
static uint32_t ncycles;

static uint16_t randuint16 ();
static void sendopcode (uint16_t opcode);
static void verifystate (char const *name, uint32_t gset, uint32_t gclr, uint32_t cset, uint32_t cign);
static uint32_t stepstate ();

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

    gpio = new PhysLib (cpuhz);
    gpio->open ();

    gpio->writegpio (false, G_RESET);
    gpio->halfcycle ();
    gpio->halfcycle ();
    gpio->halfcycle ();
    gpio->writegpio (false, 0);
    verifystate (ST_RESET0);
    stepstate ();
    verifystate (ST_RESET1);

    uint32_t lasttime = time (NULL);
    uint32_t lastcycs = 0;

    while (true) {
        uint32_t thistime = time (NULL);
        if (lasttime < thistime) {
            printf ("%10u cycles  %6u Hz  %10.3f uS\n",
                        ncycles, (ncycles - lastcycs) / (thistime - lasttime),
                        (thistime - lasttime) * 1000000.0 / (ncycles - lastcycs));
            lasttime = thistime;
            lastcycs = ncycles;
        }

        stepstate ();
        verifystate (ST_FETCH1);
        stepstate ();
        verifystate (ST_FETCH2);

        switch (randuint16 () % 9) {
            case 0: {  // BCC
                uint16_t cond = randuint16 () % 15 + 1;
                uint16_t offs = randuint16 () & 0x3FE;
                sendopcode (0x0000 | ((cond & 14) << 9) | offs | (cond & 1));
                verifystate (ST_BCC1);
                break;
            }
            case 1: {  // ARITH
                uint16_t aop = randuint16 () % 14;
                uint16_t adb = randuint16 () & 0x1FF;
                if (aop >=  3) aop ++;
                if (aop >= 11) aop ++;
                sendopcode (0x2000 | (adb << 4) | aop);
                verifystate (ST_ARITH1);
                break;
            }
            case 2: {  // STORE
                sendopcode (0x4000 | (randuint16 () & 0x3FFF));
                verifystate (ST_STORE1);
                stepstate ();
                verifystate (ST_STORE2);
                break;
            }
            case 3: {  // LDA
                sendopcode (0x8000 | (randuint16 () & 0x1FFF));
                verifystate (ST_LDA1);
                break;
            }
            case 4: {  // LOAD
                uint16_t r = randuint16 ();
                bool imm = (r & 7) == 0;
                uint16_t loadop = r / 8 % 3 + 5;
                uint16_t rardofs = imm ? (((randuint16 () & 7) << REGD) | (7 << REGA)) : (randuint16 () & 0x1FFF);
                sendopcode ((loadop << 13) | rardofs);
                verifystate (ST_LOAD1);
                stepstate ();
                verifystate (ST_LOAD2);
                if (imm) {
                    stepstate ();
                    verifystate (ST_LOAD3);
                }
                break;
            }
            case 5: {   // HALT
                sendopcode (0x0000 | ((randuint16 () & 7) << REGB));
                verifystate (ST_HALT1);
                break;
            }
            case 6: {   // IRET
                sendopcode (0x0002);
                verifystate (ST_IRET1);
                stepstate ();
                verifystate (ST_IRET2);
                stepstate ();
                verifystate (ST_IRET3);
                stepstate ();
                verifystate (ST_IRET4);
                break;
            }
            case 7: {   // WRPS
                sendopcode (0x0004 | ((randuint16 () & 7) << REGB));
                verifystate (ST_WRPS1);
                break;
            }
            case 8: {   // RDPS
                sendopcode (0x0006 | ((randuint16 () & 7) << REGD));
                verifystate (ST_RDPS1);
                break;
            }
            default: abort ();
        }
    }
}

static uint16_t randuint16 ()
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
    uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
    seed = (seed << 1) | (xnor & 1);

    return (uint16_t) seed;
}

// halfway through FETCH2 with G_CLK still asserted
static void sendopcode (uint16_t opcode)
{
    gpio->writegpio (true, opcode * G_DATA0);
    gpio->halfcycle ();

    uint32_t iactual;
    if (gpio->readcon (CON_I, &iactual)) {
        uint32_t iexpect = GpioLib::ibusshuf (opcode);
        if (iexpect != iactual) {
            fprintf (stderr, "expect icon %08X  actual %08X\n", iexpect, iactual);
            abort ();
        }
    }

    ncycles ++;
    gpio->writegpio (true, opcode * G_DATA0 | G_CLK);
    gpio->halfcycle ();
}

static void verifystate (char const *name, uint32_t gset, uint32_t gclr, uint32_t cset, uint32_t cign)
{
    uint32_t gactual = gpio->readgpio ();
    if ((gactual & (gset | gclr)) != gset) {
        fprintf (stderr, "verifystate: should be in state %s\n", name);
        if ((gset | gclr) & G_READ)  fprintf (stderr, "G_READ  is %s expect %s\n", (gactual & G_READ)  ? "set" : "clr", (gset & G_READ)  ? "set" : "clr");
        if ((gset | gclr) & G_WRITE) fprintf (stderr, "G_WRITE is %s expect %s\n", (gactual & G_WRITE) ? "set" : "clr", (gset & G_WRITE) ? "set" : "clr");
        if ((gset | gclr) & G_WORD)  fprintf (stderr, "G_WORD  is %s expect %s\n", (gactual & G_WORD)  ? "set" : "clr", (gset & G_WORD)  ? "set" : "clr");
        if ((gset | gclr) & G_HALT)  fprintf (stderr, "G_HALT  is %s expect %s\n", (gactual & G_HALT)  ? "set" : "clr", (gset & G_HALT)  ? "set" : "clr");
        abort ();
    }

/*
    uint32_t cactual;
    if (gpio->readcon (CON_C, &cactual)) {
        cactual ^= C_FLIP;
        cactual &= C_SEQ;
        if (((cactual & cset) != cset) || ((cactual & ~ cign & ~ cset) != 0)) {
            fprintf (stderr, "verifystate: should be in state %s\n", name);
            fprintf (stderr, "ccon expect set %s\n", GpioLib::decocon (CON_C, (  cset    & ~ cign) ^ C_FLIP).c_str ());
            fprintf (stderr, "     actual set %s\n", GpioLib::decocon (CON_C, (  cactual & ~ cign) ^ C_FLIP).c_str ());
            fprintf (stderr, "ccon expect clr %s\n", GpioLib::decocon (CON_C, (~ cset    & ~ cign) ^ C_FLIP).c_str ());
            fprintf (stderr, "     actual clr %s\n", GpioLib::decocon (CON_C, (~ cactual & ~ cign) ^ C_FLIP).c_str ());
            abort ();
        }
    }
*/
}

// halfway through state with clock still high
static uint32_t stepstate ()
{
    gpio->writegpio (false, 0);
    gpio->halfcycle ();
    uint32_t sample = gpio->readgpio ();
    ncycles ++;
    gpio->writegpio (false, G_CLK);
    gpio->halfcycle ();
    return sample;
}
