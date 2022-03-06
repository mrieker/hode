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
 * Program runs on raspberry pi to test the physical raspi board.
 * Requires four IOW56Paddles connected to the A,C,I,D connectors.
 * sudo insmod km/enabtsc.ko
 * sudo ./rastester -cpuhz 200 -loop

 * inputs to rasboard:

    dbus[15:00]     # ALU output

    C_ALU_IRFC      # perform function indicated in irbus[]
    C_BMUX_CON_0    # gate 0 onto bbus
    C_BMUX_CON_2    # gate 2 onto bbus
    C_BMUX_CON__4   # gate -4 onto bbus
    C_BMUX_IRBROF   # gate branch offset from IR onto bbus
    C_BMUX_IRLSOF   # gate load/store offset from IR onto bbus
    C_HALT          # HALT instruction being executed
    C__IR_WE        # write instruction register at end of cycle
    C_MEM_READ      # memory read being requested
    C_MEM_WORD      # word-sized transfer (not byte)
    C_MEM_WRITE     # memory write being requested
    C_MEM_REB       # gate memory read data onto B bus
    C_PSW_ALUWE     # write psw condition codes at end of cycle
    C__PSW_IECLR    # clear interrupt enable flipflop immediately
    C__PSW_REB      # gate psw register contents out to bbus
    C_PSW_WE        # write whole psw register at end of cycle

    abus[15]        # sign bit of 'A' operand going into ALU
    D_RAWCOUT       # carry bit out of top of ALU
    D_RAWVOUT       # carry bit out of next to top of ALU
    D_RAWSHROUT     # carry bit out of bottom of ALU

 * outputs from rasboard:

    bbus[15:00]     # ALU B operand
    irbus[15:00]    # instruction register
    _irbus[15:00]

    C_CLK2          # clock signal
    C_IRQ           # interrupt request
    C_RESET         # reset processor

    C_PSW_IE        # psw interrupt enable bit as saved in flipflops
    C_PSW_BT        # psw condition codes match branch condition in instruction register

    D_RAWCIN        # carry bit going into bottom of ALU
    D_RAWSHRIN      # shift right bit going into top of ALU

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

#include <string>

#include "gpiolib.h"
#include "miscdefs.h"

static GpioLib *gpio;

static char const *nzvcstr (int nzvcbits, char *nzvcbuff);
static void verifybbus (uint16_t bbussb);
static void verifyccout (uint32_t pins, uint32_t mask);
static void verifyiroutput (uint16_t irsb);
static void writeasign (int asign);
static void writeccon (uint32_t pins);
static void writedcon (uint32_t pins);
static uint16_t readbbus ();

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
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access rasboard circuitry via gpio and paddles
    gpio = new PhysLib (cpuhz);
    gpio->open ();

    srand (0);

    do {

        // signals from gpio pins -> control pins
        printf ("VERIFY CLK2,IRQ,RESET\n");
        for (int i = 0; i < 8; i ++) {
            int clk2  = (i & 1) != 0;
            int irq   = (i & 2) != 0;
            int reset = (i & 4) != 0;
            gpio->writegpio (false, (clk2 ? G_CLK : 0) | (irq ? G_IRQ : 0) | (reset ? G_RESET : 0));
            gpio->halfcycle();
            verifyccout ((clk2 ? C_CLK2 : 0) | (irq ? C_IRQ : 0) | (reset ? C_RESET : 0), C_CLK2 | C_IRQ | C_RESET);
        }

        // signals from control pins -> gpio pins
        printf ("VERIFY HALT,MEM_READ,_WORD,_WRITE\n");
        for (int i = 0; i < 16; i ++) {
            int halt  = (i & 1) != 0;
            int read  = (i & 2) != 0;
            int word  = (i & 4) != 0;
            int write = (i & 8) != 0;
            writeccon ((halt ? C_HALT : 0) | (read ? C_MEM_READ : 0) | (word ? C_MEM_WORD : 0) | (write ? C_MEM_WRITE : 0));
            gpio->halfcycle();
            uint32_t gpiois = gpio->readgpio ();
            uint32_t gpiosb = (halt ? G_HALT : 0) | (read ? G_READ : 0) | (word ? G_WORD : 0) | (write ? G_WRITE : 0);
            uint32_t gpmask = G_HALT | G_READ | G_WORD | G_WRITE;
            bool good = (gpiois & gpmask) == gpiosb;
            if (! good) {
                printf ("gpiosb=%08X, gpiois=%08X %08X  %s\n", gpiosb, gpiois, gpiois & gpmask, good ? "good" : "BAD");
                printf ("       HALT sb=%d, is=%d\n", (gpiosb & G_HALT)  != 0, (gpiois & G_HALT)  != 0);
                printf ("   MEM_READ sb=%d, is=%d\n", (gpiosb & G_READ)  != 0, (gpiois & G_READ)  != 0);
                printf ("   MEM_WORD sb=%d, is=%d\n", (gpiosb & G_WORD)  != 0, (gpiois & G_WORD)  != 0);
                printf ("  MEM_WRITE sb=%d, is=%d\n", (gpiosb & G_WRITE) != 0, (gpiois & G_WRITE) != 0);
            }
        }

        // verify that we can store a bunch of random numbers in instruction register
        printf ("VERIFY INSTREG\n");
        for (int i = 0; i < 100; i ++) {
            uint16_t ir = rand ();
            gpio->writegpio (true, (ir * G_DATA0));
            writeccon (C__IR_WE);
            gpio->halfcycle();
            verifyiroutput (ir);
            writeccon (0);
            gpio->halfcycle();
            verifyiroutput (ir);
            gpio->writegpio (true, ((ir * G_DATA0) ^ G_DATA));
            gpio->halfcycle();
            verifyiroutput (ir);
        }

        // verify that memory address/data output from the alu makes it to the raspi gpio data pins
        printf ("VERIFY MWRITE\n");
        gpio->writegpio (false, 0);
        writeccon (0);
        for (int i = 0; i < 100; i ++) {
            uint16_t mdsend = rand ();
            writedcon (mdsend);
            gpio->halfcycle ();
            uint16_t mdrecv = gpio->readgpio () / G_DATA0;
            if (mdrecv != mdsend) {
                printf ("main: md sent %04X, received %04X\n", mdsend, mdrecv);
                abort ();
            }
        }

        // verify that memory data read from the raspi gpio pins makes it to the bbus pins on the CON_A connector
        printf ("VERIFY MREAD\n");
        for (int opc = 5; opc <= 7; opc ++) {  // 5=LDBU; 6=LDW; 7=LDBS

            // put LDx opcode in instruction register
            gpio->writegpio (true, (opc * 0x2000 * G_DATA0));
            writeccon (C__IR_WE);
            gpio->halfcycle();
            writeccon (0);
            gpio->halfcycle();

            // set up mux for word or signed/unsigned byte gate mq on gpio pins to bbus on the CON_A connector
            if (opc & 1) {
                writeccon (C_MEM_REB);  // ir<14>=0: unsigned; =1: signed
            } else {
                writeccon (C_MEM_REB | C_MEM_WORD);
            }

            // try 100 random values being read from memory
            // the value gets sent to the rasboard via gpio pins
            // the rasboard forwards them to the bbus
            for (int i = 0; i < 100; i ++) {
                uint16_t mq = rand ();
                gpio->writegpio (true, (G_DATA0 * mq));
                gpio->halfcycle();

                switch (opc) {
                    case 5: {  // LDBU
                        verifybbus (mq & 0xFF);
                        break;
                    }
                    case 6: {  // LDW
                        verifybbus (mq);
                        break;
                    }
                    case 7: {  // LDBS
                        verifybbus (((mq & 0xFF) ^ 0x80) - 0x80);
                        break;
                    }
                }
            }
        }

        // other things on the bbus: constants, branch/load/store offsets in the ir
        printf ("VERIFY BMUX\n");
        writeccon (C_BMUX_CON__4);
        gpio->halfcycle();
        verifybbus (-4);
        writeccon (C_BMUX_CON_2);
        gpio->halfcycle();
        verifybbus (2);
        writeccon (C_BMUX_CON__4 | C_BMUX_CON_2);
        gpio->halfcycle();
        verifybbus (-2);
        writeccon (C_BMUX_CON_0);
        gpio->halfcycle();
        verifybbus (0);
        writeccon (0);
        gpio->halfcycle();
        verifybbus (-1);

        for (int i = 0; i < 100; i ++) {
            uint16_t ir = rand ();
            gpio->writegpio (true, (ir * G_DATA0));
            writeccon (C__IR_WE);
            gpio->halfcycle();
            writeccon (C_BMUX_IRBROF);
            gpio->halfcycle();
            verifybbus (((ir & 0x3FE) ^ 0x200) - 0x200);
            writeccon (C_BMUX_IRLSOF);
            gpio->halfcycle();
            verifybbus (((ir & 0x07F) ^ 0x040) - 0x040);
        }

        printf ("VERIFY PSW WE & BRANCH TESTING\n");
        for (int n = 0; n < 2; n ++) {
            for (int z = 0; z < 2; z ++) {
                for (int v = 0; v < 2; v ++) {
                    for (int c = 0; c < 2; c ++) {
                        for (int ie = 0; ie < 2; ie ++) {

                            // write ie,nzvc to ps
                            writedcon ((ie << 15) | (n << 3) | (z << 2) | (v << 1) | c);
                            gpio->writegpio (false, 0);
                            writeccon (C_PSW_WE);
                            gpio->halfcycle();
                            gpio->writegpio (false, G_CLK);
                            gpio->halfcycle();

                            // verify we can read them back
                            gpio->writegpio (false, 0);
                            writeccon (C__PSW_REB);
                            gpio->halfcycle();
                            verifybbus ((ie << 15) | 0x7FF0 | (n << 3) | (z << 2) | (v << 1) | c);

                            // verify 'branch true' condition
                            writeccon (C__IR_WE);
                            int cz  = c  | z;
                            int nv  = n  ^ v;
                            int nvz = nv | z;
                            for (int ircc = 0; ircc < 8; ircc ++) {
                                for (int ir0 = 0; ir0 < 2; ir0 ++) {
                                    uint16_t ir = (ircc << 10) | ir0;
                                    gpio->writegpio (true, (ir * G_DATA0));
                                    gpio->halfcycle();
                                    int btsb;
                                    switch (ircc) {
                                        case 0: btsb = 0;   break;
                                        case 1: btsb = z;   break;
                                        case 2: btsb = nv;  break;
                                        case 3: btsb = nvz; break;
                                        case 4: btsb = c;   break;
                                        case 5: btsb = cz;  break;
                                        case 6: btsb = n;   break;
                                        case 7: btsb = v;   break;
                                        default: abort ();
                                    }
                                    btsb ^= ir0;
                                    verifyccout (btsb ? C_PSW_BT : 0, C_PSW_BT);
                                }
                            }
                        }
                    }
                }
            }
        }

        printf ("VERIFY PSW ALUWE\n");

        for (int aop = 0; aop < 16; aop ++) {
            if (aop ==  3) continue;
            if (aop == 11) continue;
            printf ("  aop=%d\n", aop);
            uint16_t iraop = (rand () << 4) | aop;
            writeccon (C__IR_WE);
            gpio->writegpio (true, (iraop * G_DATA0));
            gpio->halfcycle ();
            writeccon (C_ALU_IRFC);
            gpio->halfcycle ();
            verifyiroutput (iraop);

            for (int dbit = -1; dbit < 16; dbit ++) {
                for (int rawcout = 0; rawcout < 2; rawcout ++) {
                    for (int rawvout = 0; rawvout < 2; rawvout ++) {
                        for (int rawshrout = 0; rawshrout < 2; rawshrout ++) {
                            for (int asign = 0; asign < 2; asign ++) {
                                for (int lastpswc = 0; lastpswc < 2; lastpswc ++) {

                                    // clock lastpswc into PSW<C> register
                                    writedcon ((rand () & 0xFFFE) | lastpswc);
                                    writeccon (C_ALU_IRFC | C_PSW_WE);
                                    gpio->writegpio (false, 0);
                                    gpio->halfcycle();
                                    gpio->writegpio (false, G_CLK);

                                    // set dbus (ALU output) value with bit 'dbit' set
                                    // ...for negative and zero detection
                                    int alun, aluz;
                                    uint16_t dbus;
                                    if (dbit == 15) {
                                        dbus = 0x8000 | rand ();
                                        alun = 1;
                                        aluz = 0;
                                    } else if (dbit == -1) {
                                        dbus = 0;
                                        alun = 0;
                                        aluz = 1;
                                    } else {
                                        dbus = (rand () & 0x7FFF) | (1 << dbit);
                                        alun = 0;
                                        aluz = 0;
                                    }

                                    // compute what the bits should be
                                    int bnot       = ((aop & 5) == 5);  // module psw: bnot = irfc & irbus2 & irbus0  (NEG,COM,SUB,SBB)
                                    int cookedcout = bnot ^ rawcout;    // module psw: as is
                                    int cookedvout = rawvout ^ rawcout;

                                    int aluv = ((aop &  4) ==  4) & cookedvout;

                                    int aluc = (((aop & 12) ==  0) & rawshrout) |   // LSR,ASR,ROR
                                               (((aop & 12) ==  4) & lastpswc)  |   // MOV,NEG,INC,COM
                                               (((aop & 12) ==  8) & lastpswc)  |   // OR,AND,XOR
                                               (((aop & 12) == 12) & cookedcout);   // ADD,SUB,ADC,SBB

                                    // set raw C and raw V bits coming out of ALU
                                    // also set up shr bit coming out of bottom of ALU
                                    writeasign (asign);
                                    writedcon (dbus | (rawcout ? D_RAWCOUT : 0) | (rawvout ? D_RAWVOUT : 0) | (rawshrout ? D_RAWSHROUT : 0));
                                    writeccon (C_ALU_IRFC | C_PSW_ALUWE);
                                    gpio->writegpio (false, 0);

                                    // let the values soak into psw circuits
                                    gpio->halfcycle();

                                    // clock values into psw's flipflops
                                    gpio->writegpio (false, G_CLK);
                                    gpio->halfcycle();
                                    writeccon (C_ALU_IRFC | C__PSW_REB);
                                    gpio->writegpio (false, 0);
                                    gpio->halfcycle();

                                    // get what nzvc bits got clocked into the psw register
                                    int nzvcis = readbbus () & 15;

                                    int nzvcsb = (alun << 3) | (aluz << 2) | (aluv << 1) | aluc;
                                    if (nzvcis != nzvcsb) {
                                        char isstr[5], sbstr[5], diffs[5];
                                        printf ("nzvc is=%s sb=%s diff=%s (aop=%d rawvout=%d rawcout=%d lastpswc=%d rawshrout=%d asign=%d bnot=%d)\n",
                                                nzvcstr (nzvcis, isstr), nzvcstr (nzvcsb, sbstr), nzvcstr (nzvcis ^ nzvcsb, diffs),
                                                aop, rawvout, rawcout, lastpswc, rawshrout, asign, bnot);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // all done
        time_t nowbin = time (NULL);
        printf ("SUCCESS / PASS %d  %s\n", ++ passno, ctime (&nowbin));

        // maybe keep doing it
    } while (loopit);

    gpio->close ();

    return 0;
}

static char const *nzvcstr (int nzvcbits, char *nzvcbuff)
{
    nzvcbuff[0] = (nzvcbits & 8) ? 'N' : '-';
    nzvcbuff[1] = (nzvcbits & 4) ? 'Z' : '-';
    nzvcbuff[2] = (nzvcbits & 2) ? 'V' : '-';
    nzvcbuff[3] = (nzvcbits & 1) ? 'C' : '-';
    nzvcbuff[4] = 0;
    return nzvcbuff;
}

// verify that the physical board is generating what we think it should be for the BBUS
// - constants, memory read data, ir branch/load/store offsets, psw contents
static void verifybbus (uint16_t bbussb)
{
    uint16_t bbusis = readbbus ();
    if (bbusis != bbussb) {
        printf ("verifybbus: bbus is %04X, sb %04X\n", bbusis, bbussb);
        abort ();
    }
}

// verify that the physical board is generating what we think it should be for the CBUS
// - control signals coming from the raspi gpio pins:  clock, reset, interrupt request
//   also control signals generated by the board:  branch true, interrupts enabled
static void verifyccout (uint32_t pins, uint32_t mask)
{
    uint32_t ccon;
    if (! gpio->readcon (CON_C, &ccon)) {
        fprintf (stderr, "verifyccout: error reading C connector\n");
        abort ();
    }
    if (((pins ^ ccon) & mask) != 0) {
        printf ("verifyccout: cbus is %04X sb %04X\n", ccon & mask, pins & mask);
        abort ();
    }
}

// verify instruction register bus contents
static void verifyiroutput (uint16_t irsb)
{
    uint32_t icon;
    if (! gpio->readcon (CON_I, &icon)) {
        fprintf (stderr, "verifyiroutput: error reading I connector\n");
        abort ();
    }
    uint16_t irpos = 0;
    uint16_t irneg = 0;
    for (int i = 0; i < 16; i ++) {
        irpos |= ((icon >> (i * 2))     & 1) << i;
        irneg |= ((icon >> (i * 2 + 1)) & 1) << i;
    }
    bool good = (irpos == irsb) && ((irpos ^ irneg) == 0xFFFF);
    if (! good) {
        printf ("verifyiroutput: ir is %04X ~%04X, sb %04X ~%04X  %-4s  diff %04X ~%04X\n",
            irpos, irneg, irsb, irsb ^ 0xFFFF, good ? "good" : "BAD",
            irpos ^ irsb, irneg ^ irsb ^ 0xFFFF);
    }
}

// write the ALU A-operand sign bit
// normally generated by a register board
// used by the rasboard to generate raw carry-in for ASR instruction
static void writeasign (int asign)
{
    uint32_t mask = GpioLib::abusshuf (0x8000);
    uint32_t pins = GpioLib::abusshuf (asign << 15);
    if (! gpio->writecon (CON_A, mask, pins)) {
        fprintf (stderr, "writeccon: error writing A connector\n");
        abort ();
    }
}

// write the given control output pins
static void writeccon (uint32_t pins)
{
    // these pins are normally driven by the SEQ board
    uint32_t mask =
            C_MEM_REB     |
            C_PSW_ALUWE   |
            C_ALU_IRFC    |
            C_GPRS_REB    |
            C_BMUX_IRBROF |
            C_GPRS_WED    |
            C_MEM_READ    |
            C_PSW_WE      |
            C_HALT        |
            C_GPRS_REA    |
            C_GPRS_RED2B  |
            C_ALU_BONLY   |
            C_BMUX_IRLSOF |
            C__PSW_REB    |
            C_BMUX_CON__4 |
            C_MEM_WORD    |
            C_ALU_ADD     |
            C_GPRS_WE7    |
            C_GPRS_REA7   |
            C_BMUX_CON_2  |
            C__IR_WE      |
            C_BMUX_CON_0  |
            C_MEM_WRITE   |
            C__PSW_IECLR;

    // flip active-low signals
    uint32_t flip =
            C__PSW_REB |
            C__IR_WE   |
            C__PSW_IECLR;

    if (! gpio->writecon (CON_C, mask, pins ^ flip)) {
        fprintf (stderr, "writeccon: error writing C connector\n");
        abort ();
    }
}

// write the given data output pins
// - this is the output of the ALU boards that we are simulating
static void writedcon (uint32_t pins)
{
    // rasboard.mod D connections (see rasboard.mod):
    //  DBUS        : written by ALU
    //  RAWCIN      : written by RAS (see psw.mod)
    //  RAWCOUT     : written by ALU
    //  RAWVOUT     : written by ALU
    //  RAWSHRIN    : written by RAS (see psw.mod)
    //  RAWSHROUT   : written by ALU

    ASSERT (D_DBUS == 0xFFFF);

    pins = (pins & 0xFFFF0000) | GpioLib::dbusshuf (pins);

    uint32_t mask = D_DBUS | D_RAWCIN | D_RAWCOUT | D_RAWVOUT | D_RAWSHRIN | D_RAWSHROUT;

    if (! gpio->writecon (CON_D, mask, pins)) {
        fprintf (stderr, "writedcon: error writing D connector\n");
        abort ();
    }
}

// read value on the B bus generated by the board
// - C_BMUX_CON_...: constants
//   C_MEM_REB: memory read data
//   C_BMUX_IRBROF,IRLSOF: ir branch/load/store offsets
//   C__PSW_REB: psw contents
static uint16_t readbbus ()
{
    uint32_t acon;
    if (! gpio->readcon (CON_A, &acon)) {
        fprintf (stderr, "readbbus: error reading A connector\n");
        abort ();
    }
    return
            (((acon >>  1) & 1) <<  0) |
            (((acon >>  5) & 1) <<  1) |
            (((acon >>  9) & 1) <<  2) |
            (((acon >> 13) & 1) <<  3) |
            (((acon >> 17) & 1) <<  4) |
            (((acon >> 21) & 1) <<  5) |
            (((acon >> 25) & 1) <<  6) |
            (((acon >> 29) & 1) <<  7) |
            (((acon >>  3) & 1) <<  8) |
            (((acon >>  7) & 1) <<  9) |
            (((acon >> 11) & 1) << 10) |
            (((acon >> 15) & 1) << 11) |
            (((acon >> 19) & 1) << 12) |
            (((acon >> 23) & 1) << 13) |
            (((acon >> 27) & 1) << 14) |
            (((acon >> 31) & 1) << 15);
}
