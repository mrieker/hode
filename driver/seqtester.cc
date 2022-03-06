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
 * Program runs on any computer to test the physical sequencer board.
 * Requires two IOW56Paddles connected to the C and I connectors.
 * sudo insmod km/enabtsc.ko
 * sudo ./seqtester -cpuhz 200 -loop
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

#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"

#define ST_RESET0  "reset0",  0, C_ALU_BONLY | C_MEM_WORD
#define ST_RESET1  "reset1",  C_BMUX_CON_0 | C_ALU_BONLY | C_GPRS_WE7 | C_PSW_WE | C__PSW_IECLR, C_MEM_WORD

#define ST_FETCH1  "fetch1",  C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, 0
#define ST_FETCH2  "fetch2",  C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7 | C__IR_WE, C_MEM_WORD

#define ST_BCC1F   "bcc1f",   C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD, C_MEM_WORD
#define ST_BCC1T   "bcc1t",   C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD | C_GPRS_WE7, C_MEM_WORD

#define ST_ARITH1  "arith1",  C_GPRS_REA | C_GPRS_REB | C_ALU_IRFC | C_GPRS_WED | C_PSW_ALUWE, C_MEM_WORD

#define ST_STORE1W "store1w", C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE | C_MEM_WORD, 0
#define ST_STORE1B "store1b", C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE, 0
#define ST_STORE2  "store2",  C_GPRS_RED2B | C_ALU_BONLY, C_MEM_WORD

#define ST_LDA1    "lda1",    C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_GPRS_WED, C_MEM_WORD

#define ST_LOAD1W  "load1w",  C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, 0
#define ST_LOAD1B  "load1b",  C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ, 0
#define ST_LOAD2W  "load2w",  C_MEM_REB | C_ALU_BONLY | C_GPRS_WED | C_MEM_WORD, 0
#define ST_LOAD2B  "load2b",  C_MEM_REB | C_ALU_BONLY | C_GPRS_WED, 0
#define ST_LOAD3   "load3",   C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7, C_MEM_WORD

#define ST_IREQ1   "ireq1",   C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, 0
#define ST_IREQ2   "ireq2",   C__PSW_REB | C_ALU_BONLY, C_MEM_WORD
#define ST_IREQ3   "ireq3",   C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, 0
#define ST_IREQ4   "ireq4",   C__PSW_IECLR | C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD, C_MEM_WORD
#define ST_IREQ5   "ireq5",   C_BMUX_CON_2 | C_ALU_BONLY | C_GPRS_WE7, C_MEM_WORD

#define ST_HALT1   "halt1",   C_HALT | C_GPRS_REB | C_ALU_BONLY, C_MEM_WORD

#define ST_IRET1   "iret1",   C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, 0
#define ST_IRET2   "iret2",   C_MEM_REB | C_MEM_WORD | C_ALU_BONLY | C_GPRS_WE7, 0
#define ST_IRET3   "iret3",   C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, 0
#define ST_IRET4   "iret4",   C_MEM_REB | C_MEM_WORD | C_ALU_BONLY | C_PSW_WE, 0

#define ST_WRPS1   "wrps1",   C_GPRS_REB | C_ALU_BONLY | C_PSW_WE, C_MEM_WORD

#define ST_RDPS1   "rdps1",   C__PSW_REB | C_ALU_BONLY | C_GPRS_WED, C_MEM_WORD

static GpioLib *gpio;
static int errors;
static uint32_t c_extras;

static void fetchopcode (uint16_t opc);
static void stepstate ();
static void verifystate (char const *statename, uint32_t shouldbe, uint32_t dontcare);
static void writeccon (uint32_t pins);

int main (int argc, char **argv)
{
    bool loopit = false;
    int instno = 0;
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
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access seqboard circuitry via paddles
    gpio = new PhysLib (cpuhz);
    gpio->open ();

    srand (0);

    do {

        // reset CPU circuit for a couple cycles
        writeccon (C_RESET);
        gpio->halfcycle ();
        gpio->halfcycle ();
        gpio->halfcycle ();

        // drop reset and leave clock signal low
        writeccon (0);
        verifystate (ST_RESET0);

        stepstate ();
        verifystate (ST_RESET1);

        for (int iter = 0; iter < 1000; iter ++) {

            int randno = rand ();
            bool irq   = (randno >> 0) & 1;
            bool pswbt = (randno >> 1) & 1;
            bool pswie = (randno >> 2) & 1;
            randno >>= 3;

            uint16_t opcode;
            switch (randno % 6) {
                case 0: opcode = 0x0000 + ((randno / 6 % 0x200) << 1); break;  // HALT/IRET/WRPS/RDPS
                case 1: {                                                      // BCC
                    randno /= 6;
                    uint16_t brop = randno % 15 + 1;
                    uint16_t brof = (randno / 15) & 0x1FF;
                    opcode  = ((brop & 14) << 9) + (brof << 1) + (brop & 1);
                    break;
                }
                case 2: opcode = 0x2000 + (randno / 6 % 0x2000); break;         // ARITH
                case 3: opcode = 0x4000 + (randno / 6 % 0x4000); break;         // STW/STB
                case 4: opcode = 0x8000 + (randno / 6 % 0x2000); break;         // LDA
                case 5: {                                                       // LDBU/LDW/LDBS
                    randno /= 6;
                    randno %= 0x6000;
                    opcode  = 0xA000 + randno;
                    if (randno % 4 == 0) opcode = (opcode & 0xFF80) | (7 << REGA); // LD imm
                    break;
                }
                default: abort ();
            }

            printf ("%5d.%7d  irq=%d bt=%d ie=%d op=%04X  %s\n", errors, ++ instno, irq, pswbt, pswie, opcode, disassemble (opcode).c_str ());

            // we are halfway through last cycle of previous instruction
            // set up the interrupt request/enable bits a half cycle before raising clock
            c_extras = (irq ? C_IRQ : 0) | (pswie ? C_PSW_IE : 0) | (pswbt ? C_PSW_BT : 0);
            writeccon (0);

            // if interrupt being requested and enabled, the board should step through the interrupt request states
            if (irq & pswie) {
                stepstate ();
                verifystate (ST_IREQ1);
                stepstate ();
                verifystate (ST_IREQ2);
                stepstate ();
                verifystate (ST_IREQ3);
                stepstate ();
                verifystate (ST_IREQ4);
                stepstate ();
                verifystate (ST_IREQ5);

                // ireq disables interrupts, so turn off interrupt enable
                c_extras &= ~ C_PSW_IE;
                writeccon (0);
            }

            // step through fetch1 & fetch2, then step halfway through first instruction cycle
            fetchopcode (opcode);

            // verify first instruction cycle then step through and verify any additional instruction cycles
            switch ((opcode >> 13) & 7) {
                case 0: {
                    if (opcode & 0x1C01) {
                        if (pswbt) {
                            verifystate (ST_BCC1T);
                        } else {
                            verifystate (ST_BCC1F);
                        }
                    } else {
                        switch ((opcode >> 1) & 3) {
                            case 0: verifystate (ST_HALT1); break;
                            case 1: {
                                verifystate (ST_IRET1);
                                stepstate ();
                                verifystate (ST_IRET2);
                                stepstate ();
                                verifystate (ST_IRET3);
                                stepstate ();
                                verifystate (ST_IRET4);
                                break;
                            }
                            case 2: verifystate (ST_WRPS1); break;
                            case 3: verifystate (ST_RDPS1); break;
                        }
                    }
                    break;
                }
                case 1: {
                    verifystate (ST_ARITH1);
                    break;
                }
                case 2: {
                    verifystate (ST_STORE1W);
                    stepstate ();
                    verifystate (ST_STORE2);
                    break;
                }
                case 3: {
                    verifystate (ST_STORE1B);
                    stepstate ();
                    verifystate (ST_STORE2);
                    break;
                }
                case 4: {
                    verifystate (ST_LDA1);
                    break;
                }
                case 5:
                case 7: {
                    verifystate (ST_LOAD1B);
                    stepstate ();
                    verifystate (ST_LOAD2B);
                    if (((opcode & 0x007F) == 0) && (((opcode >> REGA) & 7) == 7)) {
                        stepstate ();
                        verifystate (ST_LOAD3);
                    }
                    break;
                }
                case 6: {
                    verifystate (ST_LOAD1W);
                    stepstate ();
                    verifystate (ST_LOAD2W);
                    if (((opcode & 0x007F) == 0) && (((opcode >> REGA) & 7) == 7)) {
                        stepstate ();
                        verifystate (ST_LOAD3);
                    }
                    break;
                }
                default: abort ();
            }
        }

        // all done
        time_t nowbin = time (NULL);
        printf ("SUCCESS / PASS %d  %s\n", ++ passno, ctime (&nowbin));

        // maybe keep doing it
    } while (loopit);

    return 0;
}

// step through the fetch states
// called while in middle of previous instruction's last state with clock just dropped
// returns in middle of new instruction's first state with clock just dropped
static void fetchopcode (uint16_t opc)
{
    uint32_t ir32 = GpioLib::ibusshuf (opc);

    stepstate ();
    verifystate (ST_FETCH1);

    stepstate ();
    verifystate (ST_FETCH2);
    if (! gpio->writecon (CON_I, 0xFFFFFFFF, ir32)) {
        abort ();
    }

    stepstate ();
}

// finish off current state and step into next state
// called when halfway through current cycle with clock just dropped
// returns when halfway through next cycle with clock just dropped
static void stepstate ()
{
    gpio->halfcycle ();
    writeccon (C_CLK2);
    gpio->halfcycle ();
    writeccon (0);
}

// make sure the control output pins match this state
// called while in middle of cycle with clock just dropped
//  input:
//   expect = list of control input pins that are expected to be asserted
//            note: active low pins such as C__IR_WE are given if they are supposed to be active this cycle
//   dontcare = list of control input pins that we don't care about the state
static void verifystate (char const *statename, uint32_t expect, uint32_t dontcare)
{
    if (0) {
        char tmp[8];
        printf ("\nverifystate: %s> ", statename);
        if (fgets (tmp, sizeof tmp, stdin) == NULL) {
            printf ("\n");
            exit (0);
        }
        printf ("\n");
    }
    if (0) {
        printf ("verifystate: %s\n", statename);
    }

    bool reread = false;

    uint32_t conraw;
    if (! gpio->readcon (CON_C, &conraw)) {
        abort ();
    }

compute:
    uint32_t actual = conraw ^ (C__PSW_IECLR | C__IR_WE | C__PSW_REB);
    actual &= C_MEM_REB | C_PSW_ALUWE | C_ALU_IRFC | C_GPRS_REB | C_BMUX_IRBROF | C_GPRS_WED | C_MEM_READ | C_PSW_WE | C_HALT | C_GPRS_REA | C_GPRS_RED2B | C_ALU_BONLY | C_BMUX_IRLSOF | C__PSW_REB | C_BMUX_CON__4 | C_MEM_WORD | C_ALU_ADD | C_GPRS_WE7 | C_GPRS_REA7 | C_BMUX_CON_2 | C__IR_WE | C_BMUX_CON_0 | C_MEM_WRITE | C__PSW_IECLR;
    actual &= ~ dontcare;

    if (actual != expect) {
        if (! reread) {
            errors ++;
            expect ^= conraw ^ actual;
            printf ("verifystate: ERROR %-7s\n    actual %08X %s\n    expect %08X %s\n",
                    statename, conraw, GpioLib::decocon (CON_C, conraw).c_str (),
                               expect, GpioLib::decocon (CON_C, expect).c_str ());
            expect ^= conraw ^ actual;
            gpio->halfcycle ();
            gpio->halfcycle ();
            if (gpio->readcon (CON_C, &conraw)) {
                printf ("    reread %08X %s\n", conraw, GpioLib::decocon (CON_C, conraw).c_str ());
                reread = true;
                goto compute;
            }
        }
        abort ();
    }
}

// write the given control output pins + any output pins in c_extras
static void writeccon (uint32_t pins)
{
    if (! gpio->writecon (CON_C, C_IRQ | C_PSW_IE | C_PSW_BT | C_RESET | C_CLK2, c_extras | pins)) {
        abort ();
    }
}
