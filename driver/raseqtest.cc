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
 * Program runs on raspberry pi to test the physical ras/seq board combination.
 * Requires four IOW56Paddles connected to the A,C,I,D connectors.
 * sudo insmod km/enabtsc.ko
 * sudo ./raseqtest [-alu] [-cpuhz 200] [-loop] [-loopat <count>] [-nopads] [-pauseat <count>] [-reg{01,23,45,67}] [-statepause]
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

#include "alu8.h"
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

#define ST_ARITH1N "arith1n",  C_GPRS_REA | C_GPRS_REB | C_ALU_IRFC | C_PSW_ALUWE, C_MEM_WORD | C_PSW_IE
#define ST_ARITH1W "arith1w",  C_GPRS_REA | C_GPRS_REB | C_ALU_IRFC | C_GPRS_WED | C_PSW_ALUWE, C_MEM_WORD | C_PSW_IE

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

static bool hasalu;             // both ALU boards are plugged in
static bool hasregs[8];         // which REG boards are plugged in
static bool memreadpend;        // MQ data being sent from GPIO pins to IR or BBUS
                                // - eg, set in middle of LOAD2, remains set until middle of FETCH1
static bool pads;               // io-warrior-56 paddles connected to A,C,I,D connectors
static bool pauseonerror;       // pause on any error
static bool regknowns[8];       // which gpr contents are known
static bool statepause;         // pause at end of state (eg to take voltmeter readings)
static bool vfyabuseocm;        // verify ALU input (A bus) at end of current cycle
static bool vfybbuseocm;        // verify ALU input (B vus) at end of current cycle
static bool vfyireoc;           // verify IR contents at end of cycle
static GpioLib *gpio;           // access to GPIO and A,C,I,D connector pins
static int errors;              // number of errors detected
static int pauseat;             // pause at this instruction number
static uint16_t memreaddata;    // MQ data being sent from GPIO pins to IR or BBUS
static uint16_t opcode;         // random instruction we are doing
static uint16_t pswbits;
static uint16_t pswmask;
static uint16_t regvalues[8];   // known gpr contents
static uint32_t girq;           // interrupt request being sent on GPIO pins to processor
static uint32_t vfyabuseocb;    // verify ALU input (A bus) at end of current cycle
static uint32_t vfybbuseocb;    // verify ALU input (B bus) at end of current cycle
static uint32_t vfydbuseocdb;   // verify ALU output (D connector) at end of current cycle (dcon bits)
static uint32_t vfydbuseocdm;   // verify ALU output (D connector) at end of current cycle (dcon mask)

static char const *pswstring (uint16_t pswbits, uint16_t pswmask, char *pswbuff);
static void computenzvc (uint16_t aop, uint32_t dcon, uint16_t *pswbits_r, uint16_t *pswmask_r);
static bool branchtrue (uint16_t bop, uint16_t pswbits);
static void memreadcycle (uint16_t mqbus);
static void verifyabuseoc (uint16_t abus);
static void verifybbuseoc (uint16_t abus);
static void verifydbuseoc (uint32_t dbits, uint32_t dmask);
static void stepstate ();
static void verifystate (char const *statename, uint32_t cexpect, uint32_t cignore);
static bool writeabbusses (bool aena, uint16_t abus, bool bena, uint16_t bbus);
static bool writedcon (uint32_t pins);
static void goterror ();
static void dopause ();



int main (int argc, char **argv)
{
    bool loopit = false;
    int loopat = 0;
    int passno = 0;
    uint32_t cpuhz = DEFCPUHZ;
    pads = true;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-alu") == 0) {
            hasalu = true;
            continue;
        }
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
        if (strcasecmp (argv[i], "-loopat") == 0) {
            if (++ i >= argc) {
                fprintf (stderr, "-loopat missing count\n");
                return 1;
            }
            loopat = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-nopads") == 0) {
            pads = false;
            continue;
        }
        if (strcasecmp (argv[i], "-pauseat") == 0) {
            if (++ i >= argc) {
                fprintf (stderr, "-pauseat missing count\n");
                return 1;
            }
            pauseat = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-pauseonerror") == 0) {
            pauseonerror = true;
            continue;
        }
        if (strcasecmp (argv[i], "-reg01") == 0) {
            hasregs[0] = hasregs[1] = true;
            continue;
        }
        if (strcasecmp (argv[i], "-reg23") == 0) {
            hasregs[2] = hasregs[3] = true;
            continue;
        }
        if (strcasecmp (argv[i], "-reg45") == 0) {
            hasregs[4] = hasregs[5] = true;
            continue;
        }
        if (strcasecmp (argv[i], "-reg67") == 0) {
            hasregs[6] = hasregs[7] = true;
            continue;
        }
        if (strcasecmp (argv[i], "-statepause") == 0) {
            statepause = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access rasboard and seqboard circuitry via gpio and paddles
    gpio = new PhysLib (cpuhz);
    gpio->open ();

loopatloop:
    srand (0);
    int instno = 0;

    do {
        vfydbuseocdm = 0;
        vfyabuseocm  = false;
        vfybbuseocm  = false;
        vfyireoc     = false;

        // reset CPU circuit for a couple cycles
        gpio->writegpio (false, G_RESET);
        gpio->halfcycle ();
        gpio->halfcycle ();
        gpio->halfcycle ();

        // PS gets cleared to zero
        pswbits = 0x7FF0;
        pswmask = 0xFFFF;

        // PC also gets cleared to zero
        regknowns[7] = true;
        regvalues[7] = 0x0000;

        // drop reset and leave clock signal low
        gpio->writegpio (false, 0);
        verifystate (ST_RESET0);

        stepstate ();
        verifystate (ST_RESET1);

        bool pswiv = (pswmask & 0x8000) != 0;
        bool pswie = (pswbits & 0x8000) != 0;

        for (int iter = 0; iter < 1000; iter ++) {

            // randomly generate interrupt request
            // but only if we know the state of the interrupt enable bit
            int randno = rand ();
            bool irq   = pswiv && ((randno & 1) != 0);
            randno >>= 1;

            // we are halfway through last cycle of previous instruction
            // set up the interrupt request bit a half cycle before raising clock
            // if we are in middle of eg LOAD2, keep sending out data over GPIO pins so it will get clocked into register
            girq = irq ? G_IRQ : 0;
            gpio->writegpio (memreadpend, girq | (memreaddata * G_DATA0));

            // finish off last instruction's last cycle then step to IREQ1 or FETCH1
            stepstate ();

            // if interrupt being requested and enabled, the board should step through the interrupt request states
            // note we request interrupt only when we know the state of pswie
            if (irq && pswie) {
                char pswbuff[8];
                printf ("%5d.%7d  irq=%d pswiv=%d pswie=%d psw=%s interrupt\n", errors, ++ instno,
                        irq, pswiv, pswie, pswstring (pswbits, pswmask, pswbuff));

                verifystate (ST_IREQ1);                     // IREQ1: send address FFFE to raspi
                stepstate ();
                verifystate (ST_IREQ2);                     // IREQ2: send psw to raspi to write to FFFE
                stepstate ();
                verifystate (ST_IREQ3);                     // IREQ3: send address FFFC to raspi
                stepstate ();
                verifystate (ST_IREQ4);                     // IREQ4: send pc to raspi to write to FFFC, clear interrupt enable
                stepstate ();
                verifystate (ST_IREQ5);                     // IREQ5: write 0002 to PC
                stepstate ();
                pswie = false;
            }

            // generate random opcode
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

            vfyireoc = false;                               // IR doesn't match opcode any more

            char pswbuff[8];
            printf ("%5d.%7d  irq=%d pswiv=%d pswie=%d psw=%s op=%04X  %s\n", errors, ++ instno,
                    irq, pswiv, pswie, pswstring (pswbits, pswmask, pswbuff), opcode, disassemble (opcode).c_str ());

            // step through fetch1 & fetch2, then step halfway through first instruction cycle
            verifystate (ST_FETCH1);                        // it should be asking to read a word from memory
            stepstate ();
            verifystate (ST_FETCH2);                        // should be time to send opcode and it gets latched in IR
            stepstate ();                                   // step into first cycle of instruction

            // last instruction may have updated pswie (IRET, WRPS) in its lastcycle
            // so get updated bits now
            pswiv = (pswmask & 0x8000) != 0;
            pswie = (pswbits & 0x8000) != 0;

            // verify first instruction cycle then step through and verify any additional instruction cycles
            switch ((opcode >> 13) & 7) {
                case 0: {
                    if (opcode & 0x1C01) {
                        if ((pswmask & 0x000F) != 0x000F) {
                            verifystate (ST_BCC1X);
                        } else if (branchtrue (opcode, pswbits)) {
                            verifystate (ST_BCC1T);
                        } else {
                            verifystate (ST_BCC1F);
                        }
                    } else {
                        switch ((opcode >> 1) & 3) {
                            case 0: {
                                verifystate (ST_HALT1);
                                break;
                            }

                            case 1: {
                                verifystate (ST_IRET1);             // IRET1: send address FFFC to raspi
                                stepstate ();
                                verifystate (ST_IRET2);             // IRET2: restore PC from memory location FFFC
                                stepstate ();
                                verifystate (ST_IRET3);             // IRET3: send address FFFE to raspi
                                stepstate ();
                                verifystate (ST_IRET4);             // IRET4: restore PS from memory location FFFE
                                break;
                            }

                            case 2: {
                                verifystate (ST_WRPS1);
                                break;
                            }

                            case 3: {
                                verifystate (ST_RDPS1);
                                break;
                            }
                        }
                    }
                    break;
                }

                case 1: {
                    if (((opcode >> REGD) & 7) == 7) {
                        verifystate (ST_ARITH1N);
                    } else {
                        verifystate (ST_ARITH1W);
                    }
                    break;
                }

                case 2:
                case 3: {
                    if (opcode & 0x2000) {
                        verifystate (ST_STORE1B);
                    } else {
                        verifystate (ST_STORE1W);
                    }
                    stepstate ();
                    verifystate (ST_STORE2);
                    break;
                }

                case 4: {
                    verifystate (ST_LDA1);
                    break;
                }

                case 5:     // LDBU
                case 6:     // LDW
                case 7: {   // LDBS
                    if (opcode & 0x2000) {
                        verifystate (ST_LOAD1B);
                    } else {
                        verifystate (ST_LOAD1W);
                    }
                    stepstate ();
                    if (opcode & 0x2000) {
                        verifystate (ST_LOAD2B);
                    } else {
                        verifystate (ST_LOAD2W);
                    }

                    // increment PC over the immediate operand
                    if (((opcode & 0x007F) == 0) && (((opcode >> REGA) & 7) == 7) && (((opcode >> REGD) & 7) != 7)) {
                        stepstate ();
                        verifystate (ST_LOAD3);
                    }
                    break;
                }

                default: abort ();
            }

            if ((instno >= pauseat) && (pauseat > 0)) {
                dopause ();
            }

            if ((instno >= loopat) && (loopat > 0)) goto loopatloop;
        }

        // all done
        time_t nowbin = time (NULL);
        printf ("ERRORS %d / PASS %d  %s\n", errors, ++ passno, ctime (&nowbin));

        // maybe keep doing it
    } while (loopit);

    return 0;
}

// get string for current psw contents
static char const *pswstring (uint16_t pswbits, uint16_t pswmask, char *pswbuff)
{
    pswbuff[0] = (pswmask & 0x8000) ? ((pswbits & 0x8000) ? 'I' : '-') : '.';
    pswbuff[1] = '/';
    pswbuff[2] = (pswmask & 0x0008) ? ((pswbits & 0x0008) ? 'N' : '-') : '.';
    pswbuff[3] = (pswmask & 0x0004) ? ((pswbits & 0x0004) ? 'Z' : '-') : '.';
    pswbuff[4] = (pswmask & 0x0002) ? ((pswbits & 0x0002) ? 'V' : '-') : '.';
    pswbuff[5] = (pswmask & 0x0001) ? ((pswbits & 0x0001) ? 'C' : '-') : '.';
    pswbuff[6] = 0;
    return pswbuff;
}

// compute what nzvc should be after doing some arithmetic
//  input:
//   aop  = what arith function was done
//   dcon = result of the arithmetic
//  output:
//   *pswbits_r = updated
//   *pswmask_r = updated
static void computenzvc (uint16_t aop, uint32_t dcon, uint16_t *pswbits_r, uint16_t *pswmask_r)
{
    uint16_t dbus = GpioLib::dbussort (dcon);

    bool alun = (dbus & 0x8000) != 0;
    bool aluz = (dbus & 0xFFFF) == 0;

    bool rawcout   = (dcon & D_RAWCOUT) != 0;
    bool rawvout   = (dcon & D_RAWVOUT) != 0;
    bool rawshrout = (dcon & D_RAWSHROUT) != 0;

    // compute what the bits should be
    bool bnot       = ((aop & 5) == 5);  // module psw: bnot = irfc & irbus2 & irbus0  (NEG,COM,SUB,SBB)
    bool cookedcout = bnot ^ rawcout;    // module psw: as is
    bool cookedvout = rawvout ^ rawcout;

    bool aluv = ((aop & 4) == 4) & cookedvout;

    *pswbits_r &= 0xFFF1;
    if (alun) *pswbits_r |= 8;
    if (aluz) *pswbits_r |= 4;
    if (aluv) *pswbits_r |= 2;
    *pswmask_r |= 0x000E;

    if ((aop & 12) ==  0) { // LSR,ASR,ROR
        *pswbits_r &= 0xFFFE;
        if (rawshrout) *pswbits_r |= 1;
        *pswmask_r |= 0x0001;
    }
    if ((aop & 12) == 12) { // ADD,SUB,ADC,SBB
        *pswbits_r &= 0xFFFE;
        if (cookedcout) *pswbits_r |= 1;
        *pswmask_r |= 0x0001;
    }
}

// determine if branch condition is true given current condition codes
//  input:
//   bop = branch opcode
//   pswbits = condition code bits (assumed n,z,v,c are all valid)
//  output:
//   returns true iff branch should be taken
static bool branchtrue (uint16_t bop, uint16_t pswbits)
{
    bool cond = false;
    bool n = (pswbits & 8) != 0;
    bool z = (pswbits & 4) != 0;
    bool v = (pswbits & 2) != 0;
    bool c = (pswbits & 1) != 0;
    switch ((bop >> 10) & 7) {
        case 1: cond = z;           break;
        case 2: cond = n ^ v;       break;
        case 3: cond = (n ^ v) | z; break;
        case 4: cond = c;           break;
        case 5: cond = c | z;       break;
        case 6: cond = n;           break;
        case 7: cond = v;           break;
    }
    return cond ^ (bop & 1);
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
    gpio->writegpio (true, girq | (mqbus * G_DATA0));
    memreadpend = true;
    memreaddata = mqbus;
}

// verify alu inputs at end of cycle
static void verifyabuseoc (uint16_t abus)
{
    vfyabuseocb = abus;
    vfyabuseocm = true;
}
static void verifybbuseoc (uint16_t bbus)
{
    vfybbuseocb = bbus;
    vfybbuseocm = true;
}

// verify alu output at end of cycle (assuming -alu option)
//  input:
//   dbits = value to verify
//   dmask = which bits of dbits are valid
static void verifydbuseoc (uint32_t dbits, uint32_t dmask)
{
    vfydbuseocdb = dbits;
    vfydbuseocdm = dmask;
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

    // maybe verify ALU inpputs generated in the previous state
    if (vfyabuseocm | vfybbuseocm) {
        uint32_t acon;
        if (gpio->readcon (CON_A, &acon)) {
            if (vfyabuseocm) {
                uint16_t abus = GpioLib::abussort (acon);
                if (abus != vfyabuseocb) {
                    printf ("bad alu A input, is=%04X sb=%04X\n", abus, vfyabuseocb);
                    goterror ();
                }
            }
            if (vfybbuseocm) {
                uint16_t bbus = GpioLib::bbussort (acon);
                if (bbus != vfybbuseocb) {
                    printf ("bad alu B input, is=%04X sb=%04X\n", bbus, vfybbuseocb);
                    goterror ();
                }
            }
        }
        vfyabuseocm = vfybbuseocm = false;
    }

    // maybe verify ALU output generated in the previous state
    if (vfydbuseocdm) {

        // ALU output appears on D connector
        uint32_t dcon;
        if (gpio->readcon (CON_D, &dcon)) {
            ASSERT (D_DBUS == 0xFFFF);
            dcon = (dcon & 0xFFFF0000) | GpioLib::dbussort (dcon);
            if (((dcon ^ vfydbuseocdb) & vfydbuseocdm) != 0) {
                printf ("bad alu output is=%08X sb=%08X mask=%08X\n", dcon, vfydbuseocdb, vfydbuseocdm);
                goterror ();
            }
        }

        // if G_DENA is set, the ALU output should be coming in on the GPIO pins
        // this happens when processor is sending a memory address to the raspi
        // ...and when the processor is sending STW/STB data to the raspi
        if (vfydbuseocdm & 0xFFFF) {
            uint32_t gcon = gpio->readgpio ();
            if (gcon & G_DENA) {
                gcon /= G_DATA0;
                if (((gcon ^ vfydbuseocdb) & vfydbuseocdm & 0xFFFF) != 0) {
                    printf ("bad md gpio input data actual=%04X expect=%04X mask=%04X\n", gcon & 0xFFFF, vfydbuseocdb & 0xFFFF, vfydbuseocdm & 0xFFFF);
                    goterror ();
                }
            }
        }

        // this ALU output goes away next state
        vfydbuseocdm = 0;
    }

    // maybe verify IR contents loaded in previous state
    // keep verifying it throughout instruction cycles, ie, don't clear vfyireoc
    if (vfyireoc) {
        uint32_t icon;
        if (gpio->readcon (CON_I, &icon)) {
            uint16_t  ir = GpioLib::ibussort  (icon);
            uint16_t _ir = GpioLib::_ibussort (icon);
            if ((ir != opcode) || ((ir ^ _ir) != 0xFFFF)) {
                printf ("bad IR contents opcode=%04X ir=%04X _ir=%04X icon=%08X\n", opcode, ir, _ir, icon);
                goterror ();
            }
        }
    }

    // raise clock starting new state
    // maybe keep requesting interrupt
    // maybe keep sending out data or opcode read from memory
    gpio->writegpio (memreadpend, G_CLK | girq | (memreaddata * G_DATA0));
    memreadpend = false;

    // let new state soak in then drop clock and memory read data
    // keep requesting interrupt
    gpio->halfcycle ();
    gpio->writegpio (false, girq);
}

// make sure the control output pins match this state
// called while in middle of cycle with clock just dropped
//  input:
//   expect = list of control input pins that are expected to be asserted
//            note: active low pins such as C__IR_WE are given if they are supposed to be active this cycle
//   ignore = list of control input pins that we don't care about the state
static void verifystate (char const *statename, uint32_t cexpect, uint32_t cignore)
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
        printf ("verifystate: %-7s  cexpect=%08X\n", statename, cexpect);
    }

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
    if (pads && gpio->readcon (CON_C, &cactual)) {
        if (statepause) {
            printf ("statepause: should now be in state %s\n", statename);
            printf ("statepause:   C-connector is %08X %s\n", cactual, GpioLib::decocon (CON_C, cactual).c_str ());
            dopause ();
        }
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

    // maybe clear interrupt enable
    if (cexpect & C__PSW_IECLR) {
        pswbits &= 0x7FFF;
        pswmask |= 0x8000;
    }

    // check out alu inputs (abus, bbus), alu output (dbus)
    bool aknown = false;
    bool bknown = false;
    bool dknown = false;
    bool wraena = false;
    bool wrbena = false;
    uint16_t avalue = 0;
    uint16_t bvalue = 0;
    uint16_t dvalue = 0;
    uint16_t wrabus = 0xFFFF;
    uint16_t wrbbus = 0xFFFF;

    // a regboard is supplying an abus value
    if (cexpect & (C_GPRS_REA | C_GPRS_REA7)) {
        uint16_t ran = (cexpect & C_GPRS_REA) ? (opcode >> REGA) & 7 : 7;
        if (hasregs[ran]) {
            if (regknowns[ran]) {
                avalue = regvalues[ran];    // get value it should have
                verifyabuseoc (avalue);     // verify it at end of cycle
            } else {
                wrabus = 0x0000;            // value unknown, ground open-collectors
                wraena = true;
                avalue = 0x0000;            // the value is now zero
            }
        } else {
            avalue = rand ();               // no regboard present, make up a random value
            wrabus = avalue;                // send that value out over the paddle
            wraena = true;
        }
        aknown = true;                      // we now know what is supposedly on the connector
    }

    // see if something is supposed to be driving the bbus
    if (cexpect & C_MEM_REB) {
        bvalue = rand ();                   // send a random number out the gpio pins
        memreadcycle (bvalue);              // ...and keep sending until middle of next cycle
        if (! (cexpect & C_MEM_WORD)) {     // maybe byte zero/sign extend
            bvalue &= 0x00FF;
            if (opcode & 0x4000) bvalue = (bvalue ^ 0x0080) - 0x0080;
        }
        verifybbuseoc (bvalue);             // verify it at end of cycle
        bknown = true;                      // we now know what is supposedly on the connector
    }

    if (cexpect & (C_GPRS_REB | C_GPRS_RED2B)) {
        uint16_t rbn = (opcode >> ((cexpect & C_GPRS_REB) ? REGB : REGD)) & 7;
        if (hasregs[rbn]) {
            if (regknowns[rbn]) {
                bvalue = regvalues[rbn];    // get value it should have
                verifybbuseoc (bvalue);     // verify it at end of cycle
            } else {
                wrbbus = 0x0000;            // value unknown, ground open-collectors
                wrbena = true;
                bvalue = 0x0000;            // the value is now zero
            }
        } else {
            bvalue = rand ();               // no regboard present, make up a random value
            wrbbus = bvalue;                // send that value out over the paddle
            wrbena = true;
        }
        bknown = true;                      // we now know what is supposedly on the connector
    }

    if (cexpect & (C_BMUX_IRLSOF | C_BMUX_IRBROF | C_BMUX_CON_0 | C_BMUX_CON_2 | C_BMUX_CON__4)) {
        if (cexpect & C_BMUX_IRLSOF) bvalue  = ((opcode & 0x007F) ^ 0x0040) - 0x0040;
        if (cexpect & C_BMUX_IRBROF) bvalue  = ((opcode & 0x03FE) ^ 0x0200) - 0x0200;
        if (cexpect & C_BMUX_CON_0)  bvalue |=  0;
        if (cexpect & C_BMUX_CON_2)  bvalue |=  2;  // CON_2 and CON__4 means -2
        if (cexpect & C_BMUX_CON__4) bvalue |= -4;
        verifybbuseoc (bvalue);             // verify value at end of cycle
        bknown = true;                      // we now know what is supposedly on the connector
    }

    // output values to open-collector abus,bbus
    writeabbusses (wraena, wrabus, wrbena, wrbbus);

    // opcode being written to instruction register latch
    // asynchronous write through the latch should be good by end of cycle
    if (cexpect & C__IR_WE) {
        memreadcycle (opcode);
        vfyireoc = true;
    }

    // aluboards are passing the bvalue on to its output
    if (bknown && (cexpect & C_ALU_BONLY)) {
        dvalue = bvalue;
        if (hasalu) {
            verifydbuseoc (dvalue, 0xFFFF);
        } else {
            writedcon (GpioLib::dbusshuf (dvalue));
        }
        dknown = true;
    }

    // aluboards are summing avalue + bvalue on to its output
    if (aknown && bknown && (cexpect & C_ALU_ADD)) {
        dvalue = avalue + bvalue;
        if (hasalu) {
            verifydbuseoc (dvalue, 0xFFFF);
        } else {
            writedcon (GpioLib::dbusshuf (dvalue));
        }
        dknown = true;
    }

    // aluboards are computing function given in IR[3:0]
    if (aknown && bknown && (cexpect & C_ALU_IRFC)) {

        // psw.mod
        bool rawshrin = ((avalue >> 15) & 1 & opcode) | (pswbits & 1 & (opcode >> 1));
        bool bnot     = (opcode >> 2) & opcode & 1;
        bool rawcin   = bnot ^ ((opcode >> 2) & (opcode >> 1) & (pswbits | (~ opcode >> 3)) & 1);

        // compute what alu should be computing
        ALU8 aluhi, alulo;
        aluhi.ain     = avalue >> 8;
        aluhi.bin     = bvalue >> 8;
        aluhi.cin     = false;
        aluhi.shrin   = rawshrin;
        aluhi.ctlpins = C_ALU_IRFC;
        aluhi.ir      = opcode;
        alulo.ain     = avalue;
        alulo.bin     = bvalue;
        alulo.cin     = rawcin;
        alulo.shrin   = false;
        alulo.ctlpins = C_ALU_IRFC;
        alulo.ir      = opcode;
        aluhi.compute ();
        alulo.compute ();
        alulo.shrin   = aluhi.shrout;
        aluhi.cin     = alulo.cout;
        aluhi.compute ();
        alulo.compute ();

        uint16_t dbus  = (((uint16_t) aluhi.dout) << 8) | alulo.dout;
        uint32_t dbits = (aluhi.cout ? D_RAWCOUT : 0) | (aluhi.vout ? D_RAWVOUT : 0) | (alulo.shrout ? D_RAWSHROUT : 0);

        if (hasalu) {
            // set up to verify alu output at end of cycle
            verifydbuseoc (dbits | dbus, 0xFFFF | D_RAWCOUT | D_RAWVOUT | D_RAWSHROUT);
        } else {
            // output what values the aluboards should be outputting
            writedcon (GpioLib::dbusshuf (dbus) | dbits | ~ (D_DBUS | D_RAWCOUT | D_RAWVOUT | D_RAWSHROUT));
        }

        // save new psw based on assumed alu output
        ASSERT (cexpect & C_PSW_ALUWE);
        computenzvc (opcode, dbits | GpioLib::dbusshuf (dbus), &pswbits, &pswmask);

        // we now know what should be on the dbus
        dvalue = dbus;
        dknown = true;
    }

    // registers (D-flipflops) get written at end of cycle with alu output
    if (cexpect & (C_GPRS_WED | C_GPRS_WE7)) {
        uint16_t rdn = (cexpect & C_GPRS_WED) ? (opcode >> REGD) & 7 : 7;
        regvalues[rdn] = dvalue;
        regknowns[rdn] = dknown;
    }
    if (cignore & C_GPRS_WE7) {
        regknowns[7] = false;
    }

    // psw gets written at end of cycle with alu output
    if (cexpect & C_PSW_WE) {
        pswbits = dvalue | 0x7FF0;
        pswmask = dknown ? 0xFFFF : 0x7FF0;
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

// presumably we don't have ALU boards so provide what the ALU would be generating via the D paddle
static bool writedcon (uint32_t pins)
{
    // rasboard.mod D connections (see rasboard.mod):
    //  DBUS        : written by ALU
    //  RAWCIN      : written by RAS (see psw.mod)
    //  RAWCOUT     : written by ALU
    //  RAWVOUT     : written by ALU
    //  RAWSHRIN    : written by RAS (see psw.mod)
    //  RAWSHROUT   : written by ALU

    uint32_t mask = D_DBUS | D_RAWCOUT | D_RAWVOUT | D_RAWSHROUT;

    return pads && gpio->writecon (CON_D, mask, pins);
}

// increment error count and maybe pause
static void goterror ()
{
    ++ errors;
    if (pauseonerror) {
        dopause ();
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
            case 'c': {
                break;
            }
            case 'i': {
                break;
            }
        }
    }
}
