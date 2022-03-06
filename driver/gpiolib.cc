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

// common struct between PhysLib and PipeLib

#include <stdlib.h>

#include "gpiolib.h"

static char const *const gpins[] = {
    "clk",
    "reset",
    "irq",
    "_qena",
    "dena",
    "mword",
    "halt",
    "mread",
    "mwrite" };

static char const *const cpins[] = {
    "irq",
    "psw_ie",
    "psw_bt",
    "reset",
    "clk2",
    "bit-05",
    "mem_reb",
    "psw_aluwe",
    "alu_irfc",
    "gprs_reb",
    "bmux_irbrof",
    "gprs_wed",
    "mem_read",
    "psw_we",
    "halt",
    "gprs_rea",
    "bit-16",
    "gprs_red2b",
    "alu_bonly",
    "bmux_irlsof",
    "_psw_re",
    "bmux_con__4",
    "mem_word",
    "alu_add",
    "gprs_we7",
    "gprs_rea7",
    "bmux_con_2",
    "_ir_we",
    "bit-28",
    "bmux_con_0",
    "mem_write",
    "_psw_ieclr" };

static char const *const dpins[] = {
    "rawcin",
    "rawcmidi",
    "rawcmido",
    "rawcout",
    "rawvout",
    "rawshrmidi",
    "rawshrin",
    "bit-23",
    "rawshrout",
    "rawshrmido",
    "bit-26",
    "bit-27",
    "bit-28",
    "bit-29",
    "bit-30",
    "bit-31" };

// decode 32-bit value read from the gpio or a 2x20 connector
//  input:
//   c = CON_G,A,C,I,D
//   bits = value read from the connector
//  returns:
//   bits decoded as a string
std::string GpioLib::decocon (IOW56Con c, uint32_t bits)
{
    char const *const *table = NULL;
    std::string str;

    switch (c) {
        case CON_G: {
            char buf[12];
            sprintf (buf, "mbus=%04X ", (bits & G_DATA) / G_DATA0);
            str.append (buf);
            bits = (((bits >> 20) << 2) | ((bits >> 2) & 3)) & 0x1FF;
            table = gpins;
            break;
        }

        case CON_A: {
            uint16_t abus = abussort (bits);
            uint16_t bbus = bbussort (bits);
            char buf[24];
            sprintf (buf, "abus=%04X bbus=%04X ", abus, bbus);
            str.append (buf);
            bits = 0;
            break;
        }

        case CON_C: {
            table = cpins;
            break;
        }

        case CON_I: {
            uint16_t  ir =
                      (bits        & 1)        |
                    (((bits >>  2) & 1) <<  1) |
                    (((bits >>  4) & 1) <<  2) |
                    (((bits >>  6) & 1) <<  3) |
                    (((bits >>  8) & 1) <<  4) |
                    (((bits >> 10) & 1) <<  5) |
                    (((bits >> 12) & 1) <<  6) |
                    (((bits >> 14) & 1) <<  7) |
                    (((bits >> 16) & 1) <<  8) |
                    (((bits >> 18) & 1) <<  9) |
                    (((bits >> 20) & 1) << 10) |
                    (((bits >> 22) & 1) << 11) |
                    (((bits >> 24) & 1) << 12) |
                    (((bits >> 26) & 1) << 13) |
                    (((bits >> 28) & 1) << 14) |
                    (((bits >> 30) & 1) << 15);
            uint16_t _ir =
                     ((bits >>  1) & 1)        |
                    (((bits >>  3) & 1) <<  1) |
                    (((bits >>  5) & 1) <<  2) |
                    (((bits >>  7) & 1) <<  3) |
                    (((bits >>  9) & 1) <<  4) |
                    (((bits >> 11) & 1) <<  5) |
                    (((bits >> 13) & 1) <<  6) |
                    (((bits >> 15) & 1) <<  7) |
                    (((bits >> 17) & 1) <<  8) |
                    (((bits >> 19) & 1) <<  9) |
                    (((bits >> 21) & 1) << 10) |
                    (((bits >> 23) & 1) << 11) |
                    (((bits >> 25) & 1) << 12) |
                    (((bits >> 27) & 1) << 13) |
                    (((bits >> 29) & 1) << 14) |
                    (((bits >> 31) & 1) << 15);
            char buf[24];
            sprintf (buf, "ir=%04X _ir=%04X ", ir, _ir);
            str.append (buf);
            bits = 0;
            break;
        }

        case CON_D: {
            uint16_t dbus = dbussort (bits);
            char buf[12];
            sprintf (buf, "dbus=%04X ", dbus);
            str.append (buf);
            bits >>= 16;
            table = dpins;
            break;
        }

        default: abort ();
    }

    while (bits != 0) {
        if ((bits & 1) != (**table == '_')) {
            str.append (*table);
            str.append (1, ' ');
        }
        table ++;
        bits >>= 1;
    }

    int len = str.length ();
    if (len > 0) str.erase (-- len, 1);

    return str;
}

uint32_t GpioLib::abusshuf (uint16_t abus)
{
    return
              (((uint32_t) abus)        & 1)        |
            (((((uint32_t) abus) >>  1) & 1) <<  4) |
            (((((uint32_t) abus) >>  2) & 1) <<  8) |
            (((((uint32_t) abus) >>  3) & 1) << 12) |
            (((((uint32_t) abus) >>  4) & 1) << 16) |
            (((((uint32_t) abus) >>  5) & 1) << 20) |
            (((((uint32_t) abus) >>  6) & 1) << 24) |
            (((((uint32_t) abus) >>  7) & 1) << 28) |
            (((((uint32_t) abus) >>  8) & 1) <<  2) |
            (((((uint32_t) abus) >>  9) & 1) <<  6) |
            (((((uint32_t) abus) >> 10) & 1) << 10) |
            (((((uint32_t) abus) >> 11) & 1) << 14) |
            (((((uint32_t) abus) >> 12) & 1) << 18) |
            (((((uint32_t) abus) >> 13) & 1) << 22) |
            (((((uint32_t) abus) >> 14) & 1) << 26) |
            (((((uint32_t) abus) >> 15) & 1) << 30);
}

// extract abus number from A-connector
uint16_t GpioLib::abussort (uint32_t acon)
{
    return
              (acon        & 1)        |
            (((acon >>  4) & 1) <<  1) |
            (((acon >>  8) & 1) <<  2) |
            (((acon >> 12) & 1) <<  3) |
            (((acon >> 16) & 1) <<  4) |
            (((acon >> 20) & 1) <<  5) |
            (((acon >> 24) & 1) <<  6) |
            (((acon >> 28) & 1) <<  7) |
            (((acon >>  2) & 1) <<  8) |
            (((acon >>  6) & 1) <<  9) |
            (((acon >> 10) & 1) << 10) |
            (((acon >> 14) & 1) << 11) |
            (((acon >> 18) & 1) << 12) |
            (((acon >> 22) & 1) << 13) |
            (((acon >> 26) & 1) << 14) |
            (((acon >> 30) & 1) << 15);
}

uint32_t GpioLib::bbusshuf (uint16_t bbus)
{
    return
             ((((uint32_t) bbus)        & 1) <<  1) |
            (((((uint32_t) bbus) >>  1) & 1) <<  5) |
            (((((uint32_t) bbus) >>  2) & 1) <<  9) |
            (((((uint32_t) bbus) >>  3) & 1) << 13) |
            (((((uint32_t) bbus) >>  4) & 1) << 17) |
            (((((uint32_t) bbus) >>  5) & 1) << 21) |
            (((((uint32_t) bbus) >>  6) & 1) << 25) |
            (((((uint32_t) bbus) >>  7) & 1) << 29) |
            (((((uint32_t) bbus) >>  8) & 1) <<  3) |
            (((((uint32_t) bbus) >>  9) & 1) <<  7) |
            (((((uint32_t) bbus) >> 10) & 1) << 11) |
            (((((uint32_t) bbus) >> 11) & 1) << 15) |
            (((((uint32_t) bbus) >> 12) & 1) << 19) |
            (((((uint32_t) bbus) >> 13) & 1) << 23) |
            (((((uint32_t) bbus) >> 14) & 1) << 27) |
            (((((uint32_t) bbus) >> 15) & 1) << 31);
}

// extract bbus number from A-connector
uint16_t GpioLib::bbussort (uint32_t acon)
{
    return
             ((acon >>  1) & 1)        |
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

// shuffle dbus number to pins for D-connector
uint32_t GpioLib::dbusshuf (uint16_t dbus)
{
    return
            (((dbus >> 15) & 1) <<  0) |
            (((dbus >>  7) & 1) <<  1) |
            (((dbus >> 14) & 1) <<  2) |
            (((dbus >>  6) & 1) <<  3) |
            (((dbus >> 13) & 1) <<  4) |
            (((dbus >>  5) & 1) <<  5) |
            (((dbus >> 12) & 1) <<  6) |
            (((dbus >>  4) & 1) <<  7) |
            (((dbus >> 11) & 1) <<  8) |
            (((dbus >>  3) & 1) <<  9) |
            (((dbus >> 10) & 1) << 10) |
            (((dbus >>  2) & 1) << 11) |
            (((dbus >>  9) & 1) << 12) |
            (((dbus >>  1) & 1) << 13) |
            (((dbus >>  8) & 1) << 14) |
            (((dbus >>  0) & 1) << 15);
}

// extract dbus number from D-connector
uint16_t GpioLib::dbussort (uint32_t dcon)
{
    return
            (((dcon >>  0) & 1) << 15) |
            (((dcon >>  1) & 1) <<  7) |
            (((dcon >>  2) & 1) << 14) |
            (((dcon >>  3) & 1) <<  6) |
            (((dcon >>  4) & 1) << 13) |
            (((dcon >>  5) & 1) <<  5) |
            (((dcon >>  6) & 1) << 12) |
            (((dcon >>  7) & 1) <<  4) |
            (((dcon >>  8) & 1) << 11) |
            (((dcon >>  9) & 1) <<  3) |
            (((dcon >> 10) & 1) << 10) |
            (((dcon >> 11) & 1) <<  2) |
            (((dcon >> 12) & 1) <<  9) |
            (((dcon >> 13) & 1) <<  1) |
            (((dcon >> 14) & 1) <<  8) |
            (((dcon >> 15) & 1) <<  0);
}

uint32_t GpioLib::ibusshuf (uint16_t ibus)
{
    uint32_t pins = 0;
    for (int i = 0; i < 16; i ++) {
        int bit = i * 2 + 1 - (ibus & 1);
        pins |= 1 << bit;
        ibus >>= 1;
    }
    return pins;
}

// extract irbus number from I-connector
uint16_t GpioLib::ibussort (uint32_t icon)
{
    return
              (icon        & 1)        |
            (((icon >>  2) & 1) <<  1) |
            (((icon >>  4) & 1) <<  2) |
            (((icon >>  6) & 1) <<  3) |
            (((icon >>  8) & 1) <<  4) |
            (((icon >> 10) & 1) <<  5) |
            (((icon >> 12) & 1) <<  6) |
            (((icon >> 14) & 1) <<  7) |
            (((icon >> 16) & 1) <<  8) |
            (((icon >> 18) & 1) <<  9) |
            (((icon >> 20) & 1) << 10) |
            (((icon >> 22) & 1) << 11) |
            (((icon >> 24) & 1) << 12) |
            (((icon >> 26) & 1) << 13) |
            (((icon >> 28) & 1) << 14) |
            (((icon >> 30) & 1) << 15);
}

// extracted complemented irbus number from I-connector
uint16_t GpioLib::_ibussort (uint32_t icon)
{
    return
             ((icon >>  1) & 1)        |
            (((icon >>  3) & 1) <<  1) |
            (((icon >>  5) & 1) <<  2) |
            (((icon >>  7) & 1) <<  3) |
            (((icon >>  9) & 1) <<  4) |
            (((icon >> 11) & 1) <<  5) |
            (((icon >> 13) & 1) <<  6) |
            (((icon >> 15) & 1) <<  7) |
            (((icon >> 17) & 1) <<  8) |
            (((icon >> 19) & 1) <<  9) |
            (((icon >> 21) & 1) << 10) |
            (((icon >> 23) & 1) << 11) |
            (((icon >> 25) & 1) << 12) |
            (((icon >> 27) & 1) << 13) |
            (((icon >> 29) & 1) << 14) |
            (((icon >> 31) & 1) << 15);
}
