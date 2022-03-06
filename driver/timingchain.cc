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
 * Program runs on raspberry pi to test the timing through the given propagation chain
 * Requires an IOW56Paddle connected to the C connector.
 * The D connector paddle is optional (used for verification).
 * Requires the RAS, REG45, ALULO, ALUHI boards.
 * SEQ board must be omitted.  Other boards optional (unused).
 * sudo insmod km/enabtsc.ko
 * . ./iow56sns.si
 * sudo -E ./timingchain [-cpuhz 200]
 */

/*
                                            t+estim +actual
        GCON<23> = GPIO[11] = g_data[07]    t+0

00024 RasPi MQ/raspi/rbc[7]                             RAS     Q83     11.7, 16.3      mq[07]
00023 DLat _Q/ir0704/ir/rbc[3]                          RAS     Q257    16.0,  2.3      _irbus[07]

        ICON<20> = _irbus[07]               t+300   +400

00022 Gate.NOT ir7_a/r4r5[0:0]1[0]                      REG45   Q37     10.6,  2.2      ir7_a = ~_ir7
00021 Gate.AOI _reb/rodd/r4r5[0:0]4[0]                  REG45   Q51     10.2,  5.4      _reb  = ~(ir6 & ir5 & ir4 & REB | ir9 & ir8 & ir7 & RED2B)
00020 Gate.NOT reb_b/rodd/r4r5[0:0]1[0]                 REG45   Q50     10.2, 12.9      reb_b = ~_reb
00019 Gate.NAND bbus/rodd/r4r5[0:0]2[0]                 REG45   Q49     16.7, 16.1      bbus[00] = oc ~(_q[00] & reb_b)

        ACON<02> = bbus[00]                 t+900   +600

00018 Gate.NAND nandedb/alubit0/alulo[0:0]2[0]          ALULO   Q95      3.2,  3.4      nandedb[00] = ~(bnot & bbus[00])
00017 Gate.NAND nottedb/alubit0/alulo[0:0]4[0]          ALULO   Q96      4.6,  3.4      nottedb[00] = ~( ~(bnot & nandedb[00]) & ~(nandedb[00] & bbus[00]))
00016 Gate.NAND nottedb/alubit0/alulo[0:0]6[0]          ALULO   Q93      4.6,  4.0
00015 Gate.NAND anandnottedb/alubit0/alulo[0:0]2[0]     ALULO   Q91      6.0,  3.1      anandnottedb[00] = ~(ena[00] & nottedb[00])
00014 Gate.NOT aandb/alubit0/alulo[0:0]1[0]             ALULO   Q90      8.8,  4.3      aandb[00] = ~ anandnottedb[00]
00013 Gate.AO cin3/alulo[0:0]4[0]                       ALULO   Q70     10.2,  7.2      cin3[lo] = 
00012 Gate.AO cin6/alulo[0:0]4[0]                       ALULO   Q40     11.6, 11.8      cin6[lo] = 
00011 Gate.AO COUT/alulo[0:0]3[0]                       ALULO   Q180    11.6, 14.5      cout[lo] = 

        DCON<22> = cout[lo] (rawcmid)       t+2100  +1700

00010 Gate.AO cin3/aluhi[0:0]4[0]                       ALUHI   Q70     10.2,  7.2      cin3[hi] = 
00009 Gate.AO cin6/aluhi[0:0]4[0]                       ALUHI   Q40     11.6, 11.8      cin6[hi] = 
00008 Gate.AO COUT/aluhi[0:0]3[0]                       ALUHI   Q180    11.6, 14.5      cout[hi] = 

        DCON<25> = cout[hi] (rawcout)       t+2550  +1900

00007 Gate.NAND w/ccoxor/psw/rbc[0:0]2[0]               RAS     Q50      8.2,  7.8
00006 Gate.NAND v/ccoxor/psw/rbc[0:0]2[0]               RAS     Q51      8.2,  8.4
00005 Gate.NAND X/ccoxor/psw/rbc[0:0]2[0]               RAS     Q48      8.2,  6.6      cookedcout = bnot ^ rawcout
00004 Gate.NAND aluc/psw/rbc[0:0]8[0]                   RAS     Q47      4.3,  9.3
00003 Gate.NAND aluc/psw/rbc[0:0]10[0]                  RAS     Q36      4.3,  5.7      aluc = ~( ... & ~( irbus3 &  irbus2 & cookedcout))
00002 Gate.AOI _cd/pswreg/psw/rbc[0:0]5[0]              RAS     Q35      3.0,  5.7      _cd = ~(ALUWE & ALUC | WE & D[0] | cchold & C)
                                            t+3450  +2100
00001 OpndLhs D/cff/pswreg/psw/rbc[0]                                                   D:_cd
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gpiolib.h"
#include "miscdefs.h"

static GpioLib *gpio;

static void loadregimm (uint16_t reg, uint16_t imm);
static void writeccon (uint32_t pins);
static void verifydcon (uint32_t expect);

int main (int argc, char **argv)
{
    bool verify = false;
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
        if (strcasecmp (argv[i], "-verify") == 0) {
            verify = true;
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

    // set up R4 and R5
    loadregimm (4, 0xFFFF);
    loadregimm (5, 0x0000);

    // set up to continuously forward GPIO data pins out the IRBUS
    // and to do arithmetic function in IR on Ra:Rd
    writeccon (C__IR_WE | C_ALU_IRFC | C_GPRS_REA | C_GPRS_RED2B | C_PSW_ALUWE);
    usleep (100000);

    uint32_t r5r5diff = GpioLib::dbusshuf (0) | D_RAWCOUT;
    uint32_t r5r4diff = GpioLib::dbusshuf (1);

    uint32_t lasttime  = time (NULL);
    uint32_t lastcount = 0;
    uint32_t thiscount = 0;

    // alternate IR[07] bit
    while (true) {

        // do subtract R5-R5 and let it soak
        //  does 0000+FFFF+1 = 1 0000
        gpio->writegpio (true, (0x200D | (5 << REGA) | (5 << REGD)) * G_DATA0 | G_CLK);
        gpio->halfcycle ();
        gpio->writegpio (true, (0x200D | (5 << REGA) | (5 << REGD)) * G_DATA0);
        gpio->halfcycle ();
        if (verify) verifydcon (r5r5diff);

        // do subtract R5-R4 and let it soak
        //  does 0000+0000+1 = 0 0001
        gpio->writegpio (true, (0x200D | (5 << REGA) | (4 << REGD)) * G_DATA0 | G_CLK);
        gpio->halfcycle ();
        gpio->writegpio (true, (0x200D | (5 << REGA) | (4 << REGD)) * G_DATA0);
        gpio->halfcycle ();
        if (verify) verifydcon (r5r4diff);

        thiscount ++;
        uint32_t thistime = time (NULL);
        if (lasttime < thistime) {
            printf ("%10u cycles %7u Hz\n", thiscount, (thiscount - lastcount) / (thistime - lasttime));
            lastcount = thiscount;
            lasttime  = thistime;
        }
    }

    return 0;
}

static void loadregimm (uint16_t reg, uint16_t imm)
{
    // put LDW R,0(PC) instruction in instruction register
    gpio->writegpio (true, (0xC000 | (7 << REGA) | (reg << REGD)) * G_DATA0);
    writeccon (C__IR_WE);
    usleep (100000);
    writeccon (0);
    usleep (100000);

    gpio->writegpio (true, imm * G_DATA0 | G_CLK);
    writeccon (C_MEM_REB | C_ALU_BONLY | C_GPRS_WED);
    usleep (100000);

    gpio->writegpio (true, imm * G_DATA0);
    usleep (100000);

    gpio->writegpio (true, imm * G_DATA0 | G_CLK);
    writeccon (C_MEM_REB | C_ALU_BONLY);
    usleep (100000);

    gpio->writegpio (true, imm * G_DATA0);
    usleep (100000);
}

#define C_MASK (C_RAS | C_SEQ)

static void writeccon (uint32_t pins)
{
    if (! gpio->writecon (CON_C, C_MASK, pins ^ C_FLIP)) abort ();
}

static void verifydcon (uint32_t expect)
{
    uint32_t actual;
    if (gpio->readcon (CON_D, &actual)) {
        actual &= D_DBUS | D_RAWCOUT;
        if (actual != expect) {
            fprintf (stderr, "dcon actual=%08X=%s\n     expect=%08X=%s\n",
                        actual, GpioLib::decocon (CON_D, actual).c_str (),
                        expect, GpioLib::decocon (CON_D, expect).c_str ());
            abort ();
        }
    }
}
