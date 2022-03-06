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
 * Program runs on raspberry pi to test the physical ALU speed.
 * Requires four IOW56Paddles connected to the A,C,I,D connectors.
 * Assumes SEQ, RAS, both ALU boards present, no REG boards present.
 * Loops alternating D_RAWCIN while leaving IR=ADC, ABUS=0000, BBUS=FFFF.
 * Measure speed with oscope attached to D_RAWCIN and D_RAWCOUT.
 * sudo insmod km/enabtsc.ko
 * sudo ./aluspeedtet [-cpuhz 200]
 */

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"

#define ST_RESET0  "reset0",  0, C_ALU_BONLY | C_MEM_WORD | C_PSW_IE
#define ST_RESET1  "reset1",  C_BMUX_CON_0 | C_ALU_BONLY | C_GPRS_WE7 | C_PSW_WE | C__PSW_IECLR, C_MEM_WORD

#define ST_FETCH1  "fetch1",  C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_FETCH2  "fetch2",  C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7 | C__IR_WE, C_MEM_WORD | C_PSW_IE

#define ST_BCC1F   "bcc1f",   C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD, C_MEM_WORD | C_PSW_IE
#define ST_BCC1T   "bcc1t",   C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD | C_GPRS_WE7, C_MEM_WORD | C_PSW_IE
#define ST_BCC1X   "bcc1x",   C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD, C_MEM_WORD | C_GPRS_WE7 | C_PSW_IE

#define ST_ARITH1  "arith1",  C_GPRS_REA | C_GPRS_REB | C_ALU_IRFC | C_GPRS_WED | C_PSW_ALUWE, C_MEM_WORD | C_PSW_IE

#define ST_STORE1W "store1w", C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE | C_MEM_WORD, C_PSW_IE
#define ST_STORE1B "store1b", C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE, C_PSW_IE
#define ST_STORE2  "store2",  C_GPRS_RED2B | C_ALU_BONLY, C_MEM_WORD | C_PSW_IE

#define ST_LDA1    "lda1",    C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_GPRS_WED, C_MEM_WORD | C_PSW_IE

#define ST_LOAD1W  "load1w",  C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_LOAD1B  "load1b",  C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ, C_PSW_IE
#define ST_LOAD2W  "load2w",  C_MEM_REB | C_ALU_BONLY | C_GPRS_WED | C_MEM_WORD, C_PSW_IE
#define ST_LOAD2B  "load2b",  C_MEM_REB | C_ALU_BONLY | C_GPRS_WED, C_PSW_IE
#define ST_LOAD3   "load3",   C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7, C_MEM_WORD | C_PSW_IE

#define ST_IREQ1   "ireq1",   C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, C_PSW_IE
#define ST_IREQ2   "ireq2",   C__PSW_REB | C_ALU_BONLY, C_MEM_WORD | C_PSW_IE
#define ST_IREQ3   "ireq3",   C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, C_PSW_IE
#define ST_IREQ4   "ireq4",   C__PSW_IECLR | C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD, C_MEM_WORD
#define ST_IREQ5   "ireq5",   C_BMUX_CON_2 | C_ALU_BONLY | C_GPRS_WE7, C_MEM_WORD

#define ST_HALT1   "halt1",   C_HALT | C_GPRS_REB | C_ALU_BONLY, C_MEM_WORD | C_PSW_IE

#define ST_IRET1   "iret1",   C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_IRET2   "iret2",   C_MEM_REB | C_MEM_WORD | C_ALU_BONLY | C_GPRS_WE7, C_PSW_IE
#define ST_IRET3   "iret3",   C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, C_PSW_IE
#define ST_IRET4   "iret4",   C_MEM_REB | C_MEM_WORD | C_ALU_BONLY | C_PSW_WE, C_PSW_IE

#define ST_WRPS1   "wrps1",   C_GPRS_REB | C_ALU_BONLY | C_PSW_WE, C_MEM_WORD | C_PSW_IE

#define ST_RDPS1   "rdps1",   C__PSW_REB | C_ALU_BONLY | C_GPRS_WED, C_MEM_WORD | C_PSW_IE

static bool memreadpend;        // MQ data being sent from GPIO pins to IR or BBUS
                                // - eg, set in middle of LOAD2, remains set until middle of FETCH1
static GpioLib *gpio;           // access to GPIO and A,C,I,D connector pins
static uint16_t memreaddata;    // MQ data being sent from GPIO pins to IR or BBUS

static void memreadcycle (uint16_t mqbus);
static void stepstate ();
static void verifystate (char const *statename, uint32_t cexpect, uint32_t cignore);
static bool writeabbusses (bool aena, uint16_t abus, bool bena, uint16_t bbus);
static void goterror ();
static void dopause ();

int main (int argc, char **argv)
{
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
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access circuitry via gpio and paddles
    gpio = new PhysLib (cpuhz);
    gpio->open ();

    // reset CPU circuit for a couple cycles
    gpio->writegpio (false, G_RESET);
    gpio->halfcycle ();
    gpio->halfcycle ();
    gpio->halfcycle ();

    // drop reset and leave clock signal low
    gpio->writegpio (false, 0);
    verifystate (ST_RESET0);

    stepstate ();
    verifystate (ST_RESET1);

    stepstate ();

    verifystate (ST_FETCH1);                        // it should be asking to read a word from memory
    stepstate ();
    verifystate (ST_FETCH2);                        // should be time to send opcode and it gets latched in IR
    memreadcycle (0x0004);                          // WRPS R0
    stepstate ();                                   // step into first cycle of instruction

    verifystate (ST_WRPS1);
    writeabbusses (true, 0, true, 1);               // set C bit, keep all others clear

    stepstate ();

    verifystate (ST_FETCH1);                        // it should be asking to read a word from memory
    stepstate ();
    verifystate (ST_FETCH2);                        // should be time to send opcode and it gets latched in IR
    memreadcycle (0x200E);                          // ADC R0,R0,R0
    stepstate ();                                   // step into first cycle of instruction

    verifystate (ST_ARITH1);
    writeabbusses (true, 0x0000, true, 0xFFFF);

    while (true) {
        gpio->halfcycle ();
        gpio->writecon (CON_D, D_RAWCIN, 0);
        gpio->halfcycle ();
        gpio->writecon (CON_D, D_RAWCIN, D_RAWCIN);
    }

    return 0;
}

// send value on MQ bus out to IR or B bus pins
// it is currently halfway through cycle after cycle with MEM_READ asserted
// the data is persisted to halfway through next cycle
//  input:
//   mqbus = value to send on G_DATA pins of the GPIO connector
//           propagates to IR latch enabled by C__IR_WE
//           propagates to BBUS enabled by C_MEM_REB
//  output:
//   data sent over GPIO pins
//   remains sent until halfway through next cycle
static void memreadcycle (uint16_t mqbus)
{
    gpio->writegpio (true, mqbus * G_DATA0);
    memreadpend = true;
    memreaddata = mqbus;
}

// finish off current state and step into next state
// called when halfway through current cycle with clock just dropped
// returns when halfway through next cycle with clock just dropped
//  input:
//   girq = set if interrupt being requested, keep requesting it throughout transition
//   vfy{a,b,d}buseocd{m,b} = verify alu inputs/output at end of current cycle (one-shot)
//   memread{pend,data} = persist sending memory read data during transition (one-shot)
static void stepstate ()
{
    // finish off previous state
    gpio->halfcycle ();

    // raise clock starting new state
    // maybe keep sending out data or opcode read from memory
    gpio->writegpio (memreadpend, G_CLK | (memreaddata * G_DATA0));
    memreadpend = false;

    // let new state soak in then drop clock and memory read data
    gpio->halfcycle ();
    gpio->writegpio (false, 0);
}

// make sure the control output pins match this state
// called while in middle of cycle with clock just dropped
//  input:
//   expect = list of control input pins that are expected to be asserted
//            note: active low pins such as C__IR_WE are given if they are supposed to be active this cycle
//   ignore = list of control input pins that we don't care about the state
static void verifystate (char const *statename, uint32_t cexpect, uint32_t cignore)
{
    cignore |= ~ (C_MEM_REB | C_PSW_ALUWE | C_ALU_IRFC | C_GPRS_REB | C_PSW_IE |
                    C_BMUX_IRBROF | C_GPRS_WED | C_MEM_READ | C_PSW_WE |
                    C_HALT | C_GPRS_REA | C_GPRS_RED2B | C_ALU_BONLY |
                    C_BMUX_IRLSOF | C__PSW_REB | C_BMUX_CON__4 | C_MEM_WORD |
                    C_ALU_ADD | C_GPRS_WE7 | C_GPRS_REA7 | C_BMUX_CON_2 |
                    C__IR_WE | C_BMUX_CON_0 | C_MEM_WRITE | C__PSW_IECLR);

    uint32_t cflip = C__PSW_IECLR | C__IR_WE | C__PSW_REB;
    cexpect ^= cflip;
    cexpect &= ~ cignore;

    int retries = 0;
    uint32_t cactual;
retry:
    if (gpio->readcon (CON_C, &cactual)) {
        cactual &= ~ cignore;
        uint32_t cdiff = cactual ^ cexpect;
        if (cdiff) {
            printf ("verifystate: ERROR %-7s\n   cactual %08X %s\n   cexpect %08X %s\n     cdiff %08X %s\n",
                    statename, cactual, GpioLib::decocon (CON_C, cactual).c_str (),
                               cexpect, GpioLib::decocon (CON_C, cexpect).c_str (),
                                 cdiff, GpioLib::decocon (CON_C, cdiff ^ cflip).c_str ());
            if (++ retries < 3) goto retry;
            goterror ();
        } else if (retries > 0) {
            printf ("verifystate: ...good %d\n", retries);
        }
    }

    cexpect ^= cflip;

    uint32_t gexpect = 0;
    if (cexpect & C_HALT)      gexpect |= G_HALT;
    if (cexpect & C_MEM_READ)  gexpect |= G_READ;
    if (cexpect & C_MEM_WORD)  gexpect |= G_WORD;
    if (cexpect & C_MEM_WRITE) gexpect |= G_WRITE;
    uint32_t gignore = 0;
    if (cignore & C_HALT)      gignore |= G_HALT;
    if (cignore & C_MEM_READ)  gignore |= G_READ;
    if (cignore & C_MEM_WORD)  gignore |= G_WORD;
    if (cignore & C_MEM_WRITE) gignore |= G_WRITE;
    uint32_t gwhole  = gpio->readgpio ();
    uint32_t gactual = gwhole & (G_WORD | G_HALT | G_READ | G_WRITE) & ~ gignore;
    if (gactual != gexpect) {
        printf ("verifystate: ERROR %-7s\n    gpio actual %08X %s\n    gpio expect %08X %s\n",
                statename, gactual, GpioLib::decocon (CON_G, gactual).c_str (),
                           gexpect, GpioLib::decocon (CON_G, gexpect).c_str ());
        goterror ();
    }
}

// provide A and B inputs to ALU
//  inputs:
//   ?ena false: leave paddle open-drained so we can read actual value generated by {ras,reg}board at end of cycle
//         true: write value given in ?bus
static bool writeabbusses (bool aena, uint16_t abus, bool bena, uint16_t bbus)
{
    uint32_t mask = (aena ? GpioLib::abusshuf (0xFFFF) : 0) | (bena ? GpioLib::bbusshuf (0xFFFF) : 0);
    uint32_t pins = GpioLib::abusshuf (abus) | GpioLib::bbusshuf (bbus);
    return gpio->writecon (CON_A, mask, pins);
}

// increment error count and maybe pause
static void goterror ()
{
    dopause ();
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
        printf ("icon=%08X  ir=%04X  _ir=%04X    %s\n", icon, ir, GpioLib::_ibussort (icon), disassemble (ir).c_str ());
    } else {
        printf ("icon read error\n");
    }

    if (gpio->readcon (CON_D, &dcon)) {
        printf ("dcon=%08X  %s\n", acon, GpioLib::decocon (CON_D, dcon).c_str ());
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
        }
    }
}
