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
 * Program runs on the raspberry pi to alternate contents of R3.
 * Requires RAS,REG23,ALULO,ALUHI boards.
 * Requires C paddle, others optional.
 * Must not have SEQ board, others optional (unused).
 *
 *  sudo -E ./comr3.armv7l [-cpuhz <freq>] [-reg <n>]
 *
 * gets hard fail on R3[14] (other bits are ok):
 *   sudo -E ./comr3.armv7l -cpuhz 600000 -reg 3
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "rdcyc.h"

#define ever (;;)

static GpioLib *gpio;
static uint64_t ncycles;

static void *mintimesthread (void *dummy);

int main (int argc, char **argv)
{
    uint16_t regno = 3;
    uint32_t cpuhz = 100;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-cpuhz") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "missing freq after -cpuhz\n");
                return 1;
            }
            cpuhz = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-reg") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "missing number after -reg\n");
                return 1;
            }
            regno = atoi (argv[i]);
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    {
        pthread_t pid;
        int rc = pthread_create (&pid, NULL, mintimesthread, NULL);
        if (rc != 0) abort ();
        pthread_detach (pid);
    }

    // access cpu circuitry
    gpio = new PhysLib (cpuhz, false);
    gpio->open ();

    // set IR to COM Rn,Rn
    gpio->writegpio (true, (0x2007 | (regno << REGD) | (regno << REGB)) * G_DATA0);
    if (! gpio->writecon (CON_C, C_SEQ, C_FLIP)) abort ();
    usleep (100000);
    gpio->writecon (CON_C, C_SEQ, C_FLIP ^ C__IR_WE);
    usleep (100000);
    gpio->writecon (CON_C, C_SEQ, C_FLIP);
    usleep (100000);

    // execute that instruction continuously
    gpio->writecon (CON_C, C_SEQ, C_FLIP ^ (C_ALU_IRFC | C_GPRS_REB | C_GPRS_WED));
    usleep (100000);

    uint16_t lastread = 0;

    for ever {
        ncycles ++;
        gpio->writegpio (false, G_CLK);
        gpio->halfcycle ();
        gpio->writegpio (false, 0);
        gpio->halfcycle ();
        uint16_t thisread = gpio->readgpio () / G_DATA0;
        uint16_t diff = ~ thisread ^ lastread;
        if (diff != 0) {
            printf ("%12llu  thisread=%04X lastread=%04X  diff=%04X\n", ncycles, thisread, lastread, diff);
        }
        lastread = thisread;
    }
}

// runs in background to print cycle rate at beginning of every minute for testing
static void *mintimesthread (void *dummy)
{
    pthread_cond_t cond;
    pthread_mutex_t lock;
    struct timespec nowts, waits;

    pthread_cond_init (&cond, NULL);
    pthread_mutex_init (&lock, NULL);
    pthread_mutex_lock (&lock);

    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) abort ();
    uint64_t lastcycs = ncycles;
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (true) {
        int rc;
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        do rc = pthread_cond_timedwait (&cond, &lock, &waits);
        while (rc == 0);
        if (rc != ETIMEDOUT) abort ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) abort ();
        uint64_t cycs = ncycles;
        uint32_t secs = nowts.tv_sec;
        printf ("%02d:%02d:%02d  %12llu cycles  avg %6llu Hz  %6.3f uS\n",
                    secs / 3600 % 24, secs / 60 % 60, secs % 60,
                    cycs, (cycs - lastcycs) / (secs - lastsecs),
                    (secs - lastsecs) * 1000000.0 / (cycs - lastcycs));
        lastcycs = cycs;
        lastsecs = secs;
    }
    return NULL;
}
