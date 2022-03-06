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
 * Program runs on any computer to test a physical led board.
 * Requires four IOW56Paddles connected to the A,C,I,D connectors.
 * sudo insmod km/enabtsc.ko
 * sudo ./ledtester -ledhz 10
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpiolib.h"
#include "leds.h"
#include "miscdefs.h"

// top-to-bottom
static uint32_t const cleds[29] = {
    C_PSW_WE,
    C__PSW_REB,
    C__PSW_IECLR,
    C_PSW_ALUWE,
    C_MEM_WRITE,
    C_MEM_WORD,
    C_MEM_REB,
    C_MEM_READ,
    C__IR_WE,
    C_HALT,
    C_GPRS_WED,
    C_GPRS_WE7,
    C_GPRS_RED2B,
    C_GPRS_REB,
    C_GPRS_REA7,
    C_GPRS_REA,
    C_BMUX_IRLSOF,
    C_BMUX_IRBROF,
    C_BMUX_CON__4,
    C_BMUX_CON_2,
    C_BMUX_CON_0,
    C_ALU_IRFC,
    C_ALU_BONLY,
    C_ALU_ADD,
    C_PSW_IE,
    C_PSW_BT,
    C_RESET,
    C_IRQ,
    C_CLK2
};

// left-to-right
static uint32_t const dleds[7] = {
    D_RAWCOUT,      // bit 19, pin 25
    D_RAWVOUT,      // bit 20, pin 26
    D_RAWCMIDO,     // bit 17, pin 24
    D_RAWCIN,       // bit 16, pin 21
    D_RAWSHRIN,     // bit 22, pin 29
    D_RAWSHRMIDO,   // bit 25, pin 33
    D_RAWSHROUT     // bit 24, pin 32
};

static GpioLib *gpio;

int main (int argc, char **argv)
{
    uint32_t ledhz = 10;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-ledhz") == 0) {
            if (++ i >= argc) {
                fprintf (stderr, "-ledhz missing freq\n");
                return 1;
            }
            ledhz = atoi (argv[i]);
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access ledboard circuitry via paddles
    gpio = new PhysLib (ledhz);
    gpio->open ();

    // display ascii-art leds
    LEDS *leds = new LEDS ("/dev/tty");

    uint32_t axor = 0;
    uint32_t cxor = C_FLIP;
    uint32_t ixor = 0;
    uint32_t dxor = 0;

    while (true) {

        leds->update (axor, cxor, ixor, dxor);
        gpio->writecon (CON_A, 0xFFFFFFFF, axor);
        gpio->writecon (CON_C, 0xFFFFFFFF, cxor);
        gpio->writecon (CON_I, 0xFFFFFFFF, ixor);
        gpio->writecon (CON_D, 0xFFFFFFFF, dxor);

        for (int abit = 16; -- abit >= 0;) {
            uint32_t acon = GpioLib::abusshuf (1 << abit);
            leds->update (acon ^ axor, cxor, ixor, dxor);
            gpio->writecon (CON_A, 0xFFFFFFFF, acon ^ axor);
            gpio->halfcycle ();
            gpio->halfcycle ();
        }

        for (int bbit = 16; -- bbit >= 0;) {
            uint32_t acon = GpioLib::bbusshuf (1 << bbit);
            leds->update (acon ^ axor, cxor, ixor, dxor);
            gpio->writecon (CON_A, 0xFFFFFFFF, acon ^ axor);
            gpio->halfcycle ();
            gpio->halfcycle ();
        }

        gpio->writecon (CON_A, 0xFFFFFFFF, axor);

        for (int cbit = 0; cbit < 29; cbit ++) {
            uint32_t ccon = cleds[cbit];
            leds->update (axor, ccon ^ cxor, ixor, dxor);
            gpio->writecon (CON_C, 0xFFFFFFFF, ccon ^ cxor);
            gpio->halfcycle ();
            gpio->halfcycle ();
        }

        gpio->writecon (CON_C, 0xFFFFFFFF, cxor);

        for (int dbit = 7; -- dbit >= 0;) {
            uint32_t dcon = dleds[dbit];
            leds->update (axor, cxor, ixor, dcon ^ dxor);
            gpio->writecon (CON_D, 0xFFFFFFFF, dcon ^ dxor);
            gpio->halfcycle ();
            gpio->halfcycle ();
        }

        for (int dbit = 0; dbit < 16; dbit ++) {
            uint32_t dcon = GpioLib::dbusshuf (1 << dbit);
            leds->update (axor, cxor, ixor, dcon ^ dxor);
            gpio->writecon (CON_D, 0xFFFFFFFF, dcon ^ dxor);
            gpio->halfcycle ();
            gpio->halfcycle ();
        }

        gpio->writecon (CON_D, 0xFFFFFFFF, dxor);

        for (int ibit = 16; -- ibit >= 0;) {
            uint32_t icon = GpioLib::ibusshuf (1 << ibit);
            leds->update (axor, cxor, icon ^ ixor, dxor);
            gpio->writecon (CON_I, 0xFFFFFFFF, icon ^ ixor);
            gpio->halfcycle ();
            gpio->halfcycle ();
        }

        gpio->writecon (CON_I, 0xFFFFFFFF, ixor);

        axor ^= 0xFFFFFFFF;
        cxor ^= 0xFFFFFFFF;
        ixor ^= 0xFFFFFFFF;
        dxor ^= 0xFFFFFFFF;
    }

    return 0;
}
