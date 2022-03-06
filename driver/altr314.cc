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
 * Program runs on the raspberry pi to alternate bit 14 of R3.
 * Assumes full computer is present.
 *
 *  gdb --args ./altr314 [-chkacid] [-cpuhz <freq>] [-mintimes] [-printstate]
 *      -chkacid    : check A,C,I,D connectors at end of each cycle (requires paddles)
 *      -cpuhz      : specify cpu frequency (default 100Hz)
 *      -mintimes   : print cpu cycle info once a minute
 *      -printstate : print message at beginning of each state
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "shadow.h"

#define SHADOWCHECK(samp) shadowcheck (samp)

#define ever (;;)

static GpioLib *gpio;
static Shadow shadow;

static void *mintimesthread (void *dummy);
static uint16_t randuint16 ();
static void senddata (uint16_t data);
static void shadowcheck (uint32_t sample);

int main (int argc, char **argv)
{
    bool mintimes = false;
    uint32_t cpuhz = 100;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-chkacid") == 0) {
            shadow.chkacid = true;
            continue;
        }
        if (strcasecmp (argv[i], "-cpuhz") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "missing freq after -cpuhz\n");
                return 1;
            }
            cpuhz = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-mintimes") == 0) {
            mintimes = true;
            continue;
        }
        if (strcasecmp (argv[i], "-printstate") == 0) {
            shadow.printstate = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    if (mintimes) {
        pthread_t pid;
        int rc = pthread_create (&pid, NULL, mintimesthread, NULL);
        if (rc != 0) abort ();
        pthread_detach (pid);
    }

    // access cpu circuitry
    gpio = new PhysLib (cpuhz, ~ shadow.chkacid);
    gpio->open ();

    // reset CPU circuit for a couple cycles
    gpio->writegpio (false, G_RESET);
    gpio->halfcycle ();
    gpio->halfcycle ();
    gpio->halfcycle ();

    // tell shadowing that cpu is in reset0 state
    shadow.open (gpio);
    shadow.reset ();

    // drop reset and leave clock signal low
    // enable reading data from cpu
    gpio->writegpio (false, 0);
    gpio->halfcycle ();

    uint16_t regnum = 3;
    uint16_t bitmsk = 0x4000;
    uint16_t lastr3 = 0;
    uint32_t stepno = 0;

    for ever {

        // invariant:
        //  clock has been low for half cycle
        //  G_DENA has been asserted for half cycle so we can read md coming from cpu

        // get input signals just before raising clock
        uint32_t sample = gpio->readgpio ();

        if (shadow.getstate () == Shadow::LDA1) {
            uint16_t data = sample / G_DATA0;
            if (data != lastr3) {
                printf ("%12llu R%u is %04X sb %04X diff %04X\n",
                    shadow.getcycles (), regnum, data, lastr3, lastr3 ^ data);
                sample ^= (lastr3 ^ data) * G_DATA0;
            }
        }

        // raise clock then wait for half a cycle
        SHADOWCHECK (sample);
        gpio->writegpio (false, G_CLK);
        gpio->halfcycle ();

        // process the signal sample from just before raising clock
        if (sample & (G_READ | G_WRITE)) {
            if (((sample & (G_WORD | G_DATA0)) == (G_WORD | G_DATA0))) {
                fprintf (stderr, "odd word access %08X\n", sample);
            }

            if (sample & G_READ) {
                uint16_t data;
                switch (stepno) {
                    // LDW R3,2(PC)
                    case 0: {
                        if (shadow.getstate () != Shadow::FETCH2) abort ();
                        data = 0xC002 | (regnum << REGD) | (7 << REGA);
                        stepno = 1;
                        break;
                    }
                    // value for R3
                    //  alternate bit 14
                    //  random for others
                    case 1: {
                        if (shadow.getstate () != Shadow::LOAD2) abort ();
                        lastr3 ^= bitmsk;
                        lastr3 = data = (randuint16 () & ~ bitmsk) | (lastr3 & bitmsk);
                        stepno = 2;
                        break;
                    }
                    // LDA R2,0(R3)
                    case 2: {
                        if (shadow.getstate () != Shadow::FETCH2) abort ();
                        data = 0x8000 | ((regnum ^ 1) << REGD) | (regnum << REGA);
                        stepno = 3;
                        break;
                    }
                    // BR .-4
                    //  keep PC upper bits zeroes
                    case 3: {
                        if (shadow.getstate () != Shadow::FETCH2) abort ();
                        data = 0x03FB;
                        stepno = 0;
                        break;
                    }
                    default: abort ();
                }
                senddata (data);
            }

            if (sample & G_WRITE) abort ();
        }

        // drop the clock and enable data bus read
        // then wait half a clock cycle while clock is low
        gpio->writegpio (false, 0);
        gpio->halfcycle ();
    }
}

static uint16_t randuint16 ()
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
    uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
    seed = (seed << 1) | (xnor & 1);

    return (uint16_t) seed;
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
    uint64_t lastcycs = shadow.getcycles ();
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (true) {
        int rc;
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        do rc = pthread_cond_timedwait (&cond, &lock, &waits);
        while (rc == 0);
        if (rc != ETIMEDOUT) abort ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) abort ();
        uint64_t cycs = shadow.getcycles ();
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

// send data byte/word to CPU in response to a MEM_READ cycle
// we are half way through the cycle after the CPU asserted MEM_READ with clock still high
// we must send the data to the cpu with time to let it soak in before raising clock again
// so we:
//  1) drop the clock and output data to CPU
//  2) leave the data there with clock negated for half a clock cycle
//  3) raise the clock
//  4) leave the data there with clock asserted for another half a clock cycle
// now we are halfway through a cpu cycle with the clock still asserted
// for example:
//  previous cycle was FETCH1 where the cpu sent us MEM_READ with the program counter
//  we are now halfway through FETCH2 with clock still high
//  we drop the clock and send the data to the cpu by dropping G_DENA and asserting G_QENA
//  we wait a half cycle whilst the data soaks into the instruction register latch and instruction decoding circuitry
//  we raise the clock leaving the data going to the cpu so it will get clocked in correctly
//  we wait a half cycle whilst cpu is closing the instruction register latch and transitioning to first state of the instruction
static void senddata (uint16_t data)
{
    // drop the clock and start sending data to cpu
    gpio->writegpio (true, (data * G_DATA0));

    // let data soak into cpu (giving it a setup time)
    gpio->halfcycle ();

    // tell cpu data is ready to be read by raising the clock
    // hold data steady and keep sending data to cpu so it will be clocked in correctly
    SHADOWCHECK (gpio->readgpio ());
    gpio->writegpio (true, G_CLK | (data * G_DATA0));

    // wait for cpu to clock data in (giving it a hold time)
    gpio->halfcycle ();
}

// verify CPU state and update shadow state
// should be called just before raising clock
//  input:
//   sample = value just read from gpio pins
static void shadowcheck (uint32_t sample)
{
    // check it, abort if error
    if (shadow.check (sample)) abort ();

    // update shadow state to match what CPU will be after is sees clock raised
    // ...based on what state it is in now and what we last sent out on GPIO
    uint16_t mq = sample / G_DATA0;
    bool irq = (sample & G_IRQ) != 0;
    if (shadow.clock (mq, irq)) abort ();
}
