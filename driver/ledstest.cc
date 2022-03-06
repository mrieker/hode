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

#include <unistd.h>

#include "gpiolib.h"
#include "leds.h"

static uint32_t const clines[] = {
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
    C_CLK2 };

static uint32_t const dlines[] = {
    D_RAWCOUT,
    D_RAWVOUT,
    D_RAWCMIDO,
    D_RAWCIN,
    D_RAWSHRIN,
    D_RAWSHRMIDO,
    D_RAWSHROUT };

int main ()
{
    LEDS *leds = new LEDS ("/dev/tty");
    for (int i = 16; -- i >= 0;) {
        leds->update (GpioLib::abusshuf (1 << i), C_FLIP, 0, 0);
        usleep (200000);
    }
    for (int i = 16; -- i >= 0;) {
        leds->update (GpioLib::bbusshuf (1 << i), C_FLIP, 0, 0);
        usleep (200000);
    }
    for (int i = 0; i < 29; i ++) {
        leds->update (0, clines[i] ^ C_FLIP, 0, 0);
        usleep (200000);
    }
    for (int i = 0; i < 16; i ++) {
        leds->update (0, C_FLIP, GpioLib::ibusshuf (1 << i), 0);
        usleep (200000);
    }
    for (int i = 16; -- i >= -7;) {
        uint32_t dcon = (i < 0) ? dlines[~i] : GpioLib::dbusshuf (1 << i);
        leds->update (0, C_FLIP, 0, dcon);
        usleep (200000);
    }
    leds->update (0, C_FLIP, 0, 0);
    delete leds;
    return 0;
}
