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
 * Tests databus on pb2board
 * Wire the 4 flipflops to rotate dataout one bit to left round to datain
 * Wire clocks in parallel connected to CLK pin
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

#include "gpiolib.h"
#include "rdcyc.h"

#define ever (;;)

static PhysLib gpio (DEFCPUHZ);

static void delay (uint32_t cycles, char const *msg);

int main (int argc, char **argv)
{
    if (argc != 2) {
        fprintf (stderr, "usage: raspitest3 <freqhz>\n");
        return 1;
    }
    int freqhz = atoi (argv[1]);

    rdcycinit ();
    uint32_t rcfreq = rdcycfreq ();
    printf ("rcfreq = %u\n", rcfreq);
    uint32_t fullcycle = (freqhz == 0) ? 0 : rcfreq / freqhz;
    uint32_t thrdcycle = fullcycle / 3;

    gpio.open ();

    uint64_t nloops = 0;
    uint32_t errors = 0;

    time_t begtime = time (NULL);

    for ever {
        ++ nloops;
        if ((freqhz > 0) && (nloops % freqhz == 0)) {
            time_t nowtime = time (NULL);
            if (nowtime > begtime) {
                printf (" %u*%llu/%llu", errors, nloops, nloops / (nowtime - begtime));
                fflush (stdout);
            }
        }

        gpio.writegpio (true, ((nloops * G_DATA0) & G_DATA));
        char msg[24];
        sprintf (msg, "just output data %llX>", nloops % 16);
        delay (thrdcycle, msg);

        gpio.writegpio (true, ((nloops * G_DATA0) & G_DATA) | G_CLK);
        delay (thrdcycle, "just raised clock>");

        gpio.writegpio (false, 0);
        delay (thrdcycle, "about to read data>");

        uint32_t value = gpio.readgpio () / G_DATA0 % 16;
        uint32_t count = (nloops * 2 % 16) | (nloops / 8 % 2);
        if (count != value) {
            printf ("\nerror:  %llX -> %X sb %X\n", nloops % 16, value, count);
            ++ errors;
        } else if (freqhz == 0) {
            printf ("success:  %llX -> %X\n", nloops % 16, value);
        }
    }
}

static void delay (uint32_t cycles, char const *msg)
{
    char buf[16];
    if (cycles == 0) {
        fputs (msg, stderr);
        fgets (buf, sizeof buf, stdin);
    } else {
        uint32_t begcycle = rdcyc ();
        while (rdcyc () - begcycle < cycles) { }
    }
}
