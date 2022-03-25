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

// access physical cpu boards
// assumes raspberry pi gpio connector is plugged into rasboard
// ...and iow56paddles are plugged into the 4 bus connectors

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <iowkit.h>

#include "gpiolib.h"
#include "rdcyc.h"

#define HZ2835 1400000000       // raspi rdcyc frequency
#define HZ2711 1500000000       // raspi rdcyc frequency

#define IGO 1

#define GPIO_FSEL0 (0)          // set output current / disable (readwrite)
#define GPIO_SET0 (0x1C/4)      // set gpio output bits (writeonly)
#define GPIO_CLR0 (0x28/4)      // clear gpio output bits (writeonly)
#define GPIO_LEV0 (0x34/4)      // read gpio bit levels (readonly)
#define GPIO_PUDMOD (0x94/4)    // pullup/pulldown enable (0=disable; 1=enable pulldown; 2=enable pullup)
#define GPIO_PUDCLK (0x98/4)    // which pins to change pullup/pulldown mode

// tell io-warrior-56 to set all open-drain outputs to 1 (high impeadance)
// this will let us read any signal sent to the pins
static IOWKIT56_IO_REPORT const allonepins = { 0, { 255, 255, 255, 255, 255, 255, 255 } };

static sigset_t sigintmask;

PhysLib::PhysLib (uint32_t cpuhz)
{
    ctor (cpuhz, false);
}

PhysLib::PhysLib (uint32_t cpuhz, bool nopads)
{
    ctor (cpuhz, nopads);
}

void PhysLib::ctor (uint32_t cpuhz, bool nopads)
{
    sigemptyset (&sigintmask);
    sigaddset (&sigintmask, SIGINT);
    sigaddset (&sigintmask, SIGTERM);

    this->cpuhz  = cpuhz;
    this->nopads = nopads;

    memset (&hafcycts, 0, sizeof hafcycts);
    hafcycts.tv_nsec = 500000000 / cpuhz;
    printf ("PhysLib::ctor: cpuhz=%u hafcycns=%u\n", cpuhz, (uint32_t) hafcycts.tv_nsec);
}

void PhysLib::open ()
{
    char cpubuff[80];
    int memfd;
    FILE *cpufile;
    void *memptr;

    // access gpio page in physical memory
    memfd = ::open ("/dev/gpiomem", O_RDWR | O_SYNC);
    if (memfd < 0) {
        fprintf (stderr, "PhysLib::open: error opening /dev/gpiomem: %m\n");
        goto nogpio;
    }
    memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "PhysLib::open: error mmapping /dev/gpiomem: %m\n");
        abort ();
    }
    ::close (memfd);

    // save pointer to gpio register page
    gpiopage = (uint32_t volatile *) memptr;

    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/gpio_pads_control.md
    // 111... means 2mA; 777... would be 14mA

    gpiomask = G_OUTS;                          // just drive output pins, not data pins, to start with
    ASSERT (G_OUTS == 0x870000C);
    gpiopage[GPIO_FSEL0+0] = IGO * 000001100;   // gpio<09:00> = data<05:00>, reset, clock, unused<1:0>
    gpiopage[GPIO_FSEL0+1] = 0;                 // gpio<19:10> = data<15:06>
    gpiopage[GPIO_FSEL0+2] = IGO * 010000111;   // gpio<27:20> = flag,write,read,halt,word,dena,qena,irq
    gpiovalu = 0;                               // make sure gpio pin state matches what we have cached
    gpiopage[GPIO_SET0] = 0xFFFFFFFF;

    /* we provide our own strong pullups
    gpiopage[GPIO_PUDMOD] = 2;                  // about to set pullup resistors
    delay150cyc ();                             // wait a little bit
    gpiopage[GPIO_PUDCLK] = G_INS | G_DATA;     // say which pins to pull up
    delay150cyc ();
    gpiopage[GPIO_PUDMOD] = 0;                  // turn off clocking
    gpiopage[GPIO_PUDCLK] = 0;
    */

    lastretries = 0;
nogpio:;

    // get host (raspi) cpu frequency
    cpufile = fopen ("/proc/cpuinfo", "r");
    if (cpufile == NULL) {
        fprintf (stderr, "PhysLib::open: error opening /proc/cpuinfo: %m\n");
        abort ();
    }
    is2711 = false;
    double rasphz;
    while (fgets (cpubuff, sizeof cpubuff, cpufile)) {
        if ((strstr (cpubuff, "Hardware") != NULL) && (strstr (cpubuff, "BCM2835") != NULL)) {
            rasphz = HZ2835;
            goto gotcpu;
        }
        if ((strstr (cpubuff, "Hardware") != NULL) && (strstr (cpubuff, "BCM2711") != NULL)) {
            is2711 = true;
            rasphz = HZ2711;
            goto gotcpu;
        }
        if (strstr (cpubuff, "cpu MHz") != NULL) {
            char const *colon = strchr (cpubuff, ':');
            if (colon != NULL) {
                rasphz = strtod (colon + 1, NULL) * 1000000.0;
                goto gotcpu;
            }
        }
    }
    fprintf (stderr, "PhysLib::open: neither BCM2835 nor BCM2711 nor X86\n");
    abort ();
gotcpu:;
    fclose (cpufile);

    double tclk = 1.0 / cpuhz;  // time from one clock up to next clock up
    double tphl = 0.000000040;  // time from GPIO[02] going high to C_CLK2 going low
    double tplh = 0.000000120;  // time from GPIO[02] going low to C_CLK2 going high

    // if hafcychi = hafcyclo, the resultant C_CLK2 would have a long low time and a short high time
    // so we must shorten hafcyclo so C_CLK2 will spemd less time being low
    // ...and lengthen hafcychi so C_CLK2 will spend more time being high

    // suppose we just cleared G_CLK (gpiovalu & G_CLK == 0),
    // so we just drove GPIO[02] high, meaning C_CLK2 is going low in tphl

    // in tgpio2hi from now, we will set G_CLK (gpiovalu & G_CLK != 0),
    // so we will drive GPIO[02] low, meaning C_CLK2 will go high tplh from then

    // time GPIO2 goes low to high: tgpio2lotohi = just now
    // time CCLK2 goes high to low: tcclk2hitolo = tgpio2lotohi + tphl
    // time GPIO2 goes high to low: tgpio2hitolo = tgpio2lotohi + tgpio2hi
    // time CCLK2 goes low to high: tcclk2lotohi = tgpio2hitolo + tplh

    // time CCLK2 is low:  tcclk2lo = tcclk2lotohi - tcclk2hitolo
    // time CCLK2 is high: tcclk2hi = tclk - tcclk2lo

    // compute tgpio2hi, tgpio2lo such that:
    //  tgpio2hi + tgpio2lo = tclk      whole period is given frequency
    //  tcclk2lo = tcclk2hi             C_CLK2 is symmetrical

    //  tcclk2lo = tcclk2hi
    //  tcclk2lo = tclk - tcclk2lo
    //  tcclk2lo = tclk / 2
    //  tcclk2lotohi - tcclk2hitolo = tclk / 2
    //  (tgpio2hitolo + tplh) - (tgpio2lotohi + tphl) = tclk / 2
    //  (tgpio2lotohi + tgpio2hi + tplh) - (tgpio2lotohi + tphl) = tclk / 2
    //  (tgpio2hi + tplh) - (tphl) = tclk / 2
    //  (tgpio2hi + tplh) = tclk / 2 + tphl
    //  tgpio2hi = tclk / 2 + tphl - tplh

    double tgpio2hi = tclk / 2 + tphl - tplh;   // time GPIO[02] is high = time G_CLK is low = shorter than half
    double tgpio2lo = tclk / 2 + tplh - tphl;   // time GPIO[02] is low = time G_CLK is high = longer than half

    hafcychi = (sint64_t) (rasphz * tgpio2lo);  // cycles G_CLK is high
    hafcyclo = (sint64_t) (rasphz * tgpio2hi);  // cycles G_CLK is low
    printf ("PhysLib::open: hafcychi=%u hafcyclo=%u\n", hafcychi, hafcyclo);

    // initialize rdcyc()
    rdcycinit ();

    // open io-warrior-56 paddles plugged into a,c,i,d connectors
    // set all the outputs to 1s to open the open drain outputs so we can read the pins
    if (! nopads) {
        blocksigint ();
        uint16_t snb[9];
        IowKitOpenDevice ();
        ULONG numdevs = IowKitGetNumDevs ();
        if (numdevs == 0) {
            fprintf (stderr, "PhysLib::open: didn't find any iow56 paddles, maybe not running as root\n");
        }
        for (int i = 0; i < 4; i ++) {
            iowhandles[i] = NULL;
            char env[12];
            sprintf (env, "iow56sn_%c", CONLETS[i] | 0x20);
            char const *esn = getenv (env);
            if (esn == NULL) {
                fprintf (stderr, "PhysLib::open: missing envar %s\n", env);
                continue;
            }
            IOWKIT_HANDLE iowh;
            for (ULONG j = 0; j < numdevs; j ++) {
                iowh = IowKitGetDeviceHandle (j + 1);
                if (IowKitGetSerialNumber (iowh, snb)) {
                    for (int k = 0; k < 9; k ++) {
                        if (snb[k] != esn[k]) goto notit;
                    }
                    goto gotit;
                }
            notit:;
            }
            fprintf (stderr, "PhysLib::open: missing device %s sn %s\n", env, esn);
            continue;
        gotit:;

            // make sure it is an io-warrior-56
            ULONG prodid = IowKitGetProductId (iowh);
            if (prodid != IOWKIT_PRODUCT_ID_IOW56) {
                fprintf (stderr, "PhysLib::open: io-warrior %s sn %s is not an io-warrior-56 but is product-id %lu\n", env, esn, prodid);
                continue;
            }

            // write 1s to all open-drain pins so we can read the pins
            ULONG rc = IowKitWrite (iowh, IOW_PIPE_IO_PINS, (char *) &allonepins, sizeof allonepins);
            if (rc != sizeof allonepins) {
                fprintf (stderr, "PhysLib::open: error %lu writing allonepins to %s sn %s: %m\n", rc, env, esn);
                abort ();
            }

            // drain any pending incoming messages
            IOWKIT56_IO_REPORT drainbuf;
            while (IowKitReadNonBlocking (iowh, IOW_PIPE_IO_PINS, (char *) &drainbuf, sizeof drainbuf)) { }
            IOWKIT56_SPECIAL_REPORT drainspec;
            while (IowKitReadNonBlocking (iowh, IOW_PIPE_SPECIAL_MODE, (char *) &drainspec, sizeof drainspec)) { }

            // save handle for later use
            iowhandles[i] = iowh;
        }
        allowsigint ();
    }
}

void PhysLib::close ()
{
    if (writecount > 0) {
        printf ("PhysLib::close: writecount=%u avgcyclesperwrite=%u\n", writecount, (uint32_t) (writecycles / writecount));
    }
}

void PhysLib::halfcycle ()
{
    if (cpuhz > 1000) {
        uint32_t start = rdcyc ();
        uint32_t delta = (gpiovalu & G_CLK) ? hafcychi : hafcyclo;
        while (rdcyc () - start < delta) { }
    } else {
        nanosleep (&hafcycts, NULL);
    }
}

uint32_t PhysLib::readgpio ()
{
    // all the pins go through an inverter converting the 5V to 3.3V
    return ~ gpiopage[GPIO_LEV0];
}

//  wdata = false: reading G_DATA pins; true: writing G_DATA pins
void PhysLib::writegpio (bool wdata, uint32_t value)
{
    // maybe update gpio pin direction
    uint32_t mask = wdata ? G_OUTS | G_DATA : G_OUTS;
    if (gpiomask != mask) {
        gpiomask = mask;
        ASSERT (G_OUTS == 0x870000C);
        ASSERT (G_DATA == 0x00FFFF0);
        if (wdata) {
            gpiopage[GPIO_FSEL0+0] = IGO * 01111111100;
            gpiopage[GPIO_FSEL0+1] = IGO * 01111111111;
        } else {
            gpiopage[GPIO_FSEL0+0] = IGO * 01100;
            gpiopage[GPIO_FSEL0+1] = 0;
        }
    }

    // insert G_DENA,G__QENA as appropriate for mask
    if (wdata) {

        // caller is writing data to the GPIO pins and wants it to go to MQ
        // clearing G_DENA blocks MD from getting to GPIO pins
        // clearing G__QENA enables the data from GPIO pins to MQ
        value &= ~ (G_DENA | G__QENA);
    } else {

        // caller isn't writing and may want to read the GPIO data pins
        // setting G_DENA enables data from MD to get to GPIO data pins
        // setting G__QENA blocks data from GPIO pins to MQ
        value |=    G_DENA | G__QENA;
    }

    // update pins
    if (((value ^ gpiovalu) & mask) != 0) {
        gpiovalu = value;

        // all the pins go through an inverter converting the 3.3V to 5V
        gpiopage[GPIO_CLR0] =   value;
        gpiopage[GPIO_SET0] = ~ value;

        // sometimes takes 1 or 2 retries to make sure signal gets out
        uint32_t retries = 0;
        while (true) {
            uint32_t readback = ~ gpiopage[GPIO_LEV0];
            uint32_t diff = (readback ^ value) & mask;
            if (diff == 0) break;
            if (++ retries > 1000) {
                fprintf (stderr, "PhysLib::writegpio: wrote %08X mask %08X, readback %08X diff %08X\n",
                            value, mask, readback, diff);
                abort ();
            }
        }
        if (lastretries < retries) {
            lastretries = retries;
            printf ("PhysLib::writegpio: retries %u\n", retries);
        }
    }
}

// make the data pins readable
void PhysLib::makeinputpins ()
{
    ASSERT (G_OUTS == 0x870000C);
    ASSERT (G_DATA == 0x00FFFF0);
    gpiopage[GPIO_FSEL0+0] = IGO * 01100;
    gpiopage[GPIO_FSEL0+1] = 0;
}

// make the data pins writable
void PhysLib::makeoutputpins ()
{
    ASSERT (G_OUTS == 0x870000C);
    ASSERT (G_DATA == 0x00FFFF0);
    gpiopage[GPIO_FSEL0+0] = IGO * 01111111100;
    gpiopage[GPIO_FSEL0+1] = IGO * 01111111111;
}

// clear the given pins
void PhysLib::clrpins (uint32_t mask)
{
    gpiopage[GPIO_CLR0] = mask;
}

// set the given pins
void PhysLib::setpins (uint32_t mask)
{
    gpiopage[GPIO_SET0] = mask;
}

// read the pins
uint32_t PhysLib::getpins ()
{
    return gpiopage[GPIO_LEV0];
}

// ask io-warrior-56 for state of all pins
static IOWKIT56_SPECIAL_REPORT const reqallpins[] = { 255 };

// mapping of 2x20 connector pins to io-warrior-56 port bits
static uint8_t const pinmapping[] = {
        4 * 8 + 6,  // bit 00 -> pin 01 -> P4.6
        2 * 8 + 2,  // bit 01 -> pin 02 -> P2.2
        4 * 8 + 2,  // bit 02 -> pin 03 -> P4.2
        2 * 8 + 6,  // bit 03 -> pin 04 -> P2.6
                    //           pin 05 -> GND
        0 * 8 + 0,  // bit 04 -> pin 06 -> P0.0
        3 * 8 + 6,  // bit 05 -> pin 07 -> P3.6
        2 * 8 + 4,  // bit 06 -> pin 08 -> P2.4
                    //           pin 09 -> GND
        2 * 8 + 0,  // bit 07 -> pin 10 -> P2.0
        3 * 8 + 2,  // bit 08 -> pin 11 -> P3.2
        4 * 8 + 4,  // bit 09 -> pin 12 -> P4.4
                    //           pin 13 -> GND
        4 * 8 + 0,  // bit 10 -> pin 14 -> P4.0
        5 * 8 + 6,  // bit 11 -> pin 15 -> P5.6
        3 * 8 + 4,  // bit 12 -> pin 16 -> P3.4
                    //           pin 17 -> GND
        3 * 8 + 0,  // bit 13 -> pin 18 -> P3.0
        5 * 8 + 2,  // bit 14 -> pin 19 -> P5.2
        5 * 8 + 4,  // bit 15 -> pin 20 -> P5.4

        3 * 8 + 3,  // bit 16 -> pin 21 -> P3.3
        3 * 8 + 5,  // bit 17 -> pin 22 -> P3.5
                    //           pin 23 -> GND
        4 * 8 + 1,  // bit 18 -> pin 24 -> P4.1
        3 * 8 + 7,  // bit 19 -> pin 25 -> P3.7
        4 * 8 + 5,  // bit 20 -> pin 26 -> P4.5
                    //           pin 27 -> GND
        2 * 8 + 1,  // bit 21 -> pin 28 -> P2.1
        4 * 8 + 3,  // bit 22 -> pin 29 -> P4.3
        2 * 8 + 5,  // bit 23 -> pin 30 -> P2.5
                    //           pin 31 -> GND
        0 * 8 + 1,  // bit 24 -> pin 32 -> P0.1
        4 * 8 + 7,  // bit 25 -> pin 33 -> P4.7
        0 * 8 + 5,  // bit 26 -> pin 34 -> P0.5
                    //           pin 35 -> GND
        0 * 8 + 6,  // bit 27 -> pin 36 -> P0.6
        2 * 8 + 3,  // bit 28 -> pin 37 -> P2.3
        0 * 8 + 7,  // bit 29 -> pin 38 -> P0.7
        2 * 8 + 7,  // bit 30 -> pin 39 -> P2.7
        0 * 8 + 3   // bit 31 -> pin 40 -> P0.3
};

// read iow56 connector pins
//  input:
//    c = which connector
//  output:
//    returns false: connector not opened
//             true: connector successfully read
//                   *pins = pins read from connector
bool PhysLib::readcon (IOW56Con c, uint32_t *pins)
{
    ASSERT ((c >= 0) && (c < sizeof iowhandles / sizeof iowhandles[0]));

    // read all the 2x20 connector pins
    IOWKIT_HANDLE iowh = iowhandles[c];
    if (iowh == NULL) return false;

    blocksigint ();
    *pins = readconblocked (c, iowh);
    allowsigint ();

    return true;
}

uint32_t PhysLib::readconblocked (IOW56Con c, IOWKIT_HANDLE iowh)
{
    IOWKIT56_SPECIAL_REPORT iowallpins;
    ULONG rc;

    rc = IowKitWrite (iowh, IOW_PIPE_SPECIAL_MODE, (char *) &reqallpins, sizeof reqallpins);
    if (rc != sizeof reqallpins) {
        fprintf (stderr, "PhysLib::readcon: error %lu writing reqallpins to %c: %m\n", rc, CONLETS[c]);
        abort ();
    }
    memset (&iowallpins, 0, sizeof iowallpins);
    rc = IowKitRead (iowh, IOW_PIPE_SPECIAL_MODE, (char *) &iowallpins, sizeof iowallpins);
    if (rc != sizeof iowallpins) {
        fprintf (stderr, "PhysLib::readcon: error %lu reading iowallpins from %c: %m\n", rc, CONLETS[c]);
        abort ();
    }
    if (iowallpins.ReportID != 255) {
        fprintf (stderr, "PhysLib::readcon: got report id %02X reading iowallpins from %c\n", iowallpins.ReportID, CONLETS[c]);
        abort ();
    }

    uint32_t val = 0;
    for (int j = 32; -- j >= 0;) {
        uint8_t pm = pinmapping[j];
        int p = pm >> 3;            // P0,P1,..,P6
        int m = pm & 7;             // .0,.1,...,.7
        val += val + ((iowallpins.Bytes[p] >> m) & 1);
    }
    return val;
}

// write iow56 connector pins
//  input:
//    c = which connector
//    mask = which pins are output pins (others are inputs or not connected)
//    pins = values for the pins listed in mask (others are don't care)
//  output:
//    returns false: connector not opened
//             true: connector successfully written
bool PhysLib::writecon (IOW56Con c, uint32_t mask, uint32_t pins)
{
    ASSERT ((c >= 0) && (c < sizeof iowhandles / sizeof iowhandles[0]));

    IOWKIT_HANDLE iowh = iowhandles[c];
    if (iowh == NULL) return false;

    // by default, set up to write ones to all pins so they are open-drained
    IOWKIT56_IO_REPORT updallpins;
    memset (&updallpins, 0xFF, sizeof updallpins);
    updallpins.ReportID = 0;

    // for any given pins in the mask that are zero, set that pin to zero
    for (int j = 0; j < 32; j ++) {
        if (((mask & ~ pins) >> j) & 1) {
            uint8_t pm = pinmapping[j];
            int p = pm >> 3;            // P0,P1,..,P6
            int m = pm & 7;             // .0,.1,...,.7
            updallpins.Bytes[p] &= ~ (1 << m);
        }
    }

    blocksigint ();
    int retry = 0;
    uint32_t startcycle = rdcyc ();
    while (true) {

        // send write command to IOW-56 paddle
        ULONG rc = IowKitWrite (iowh, IOW_PIPE_IO_PINS, (char *) &updallpins, sizeof updallpins);
        if (rc != sizeof updallpins) {
            fprintf (stderr, "PhysLib::writecon: error %lu writing updallpins to %c: %m\n", rc, CONLETS[c]);
            abort ();
        }

        // read back from the paddle to verify the write
        // sometimes (always?) takes a loop before new value gets written
        uint32_t verify = readconblocked (c, iowh);
        if (c == CON_A) break;      // aluboards sometime shunt A inputs to GND
        if (((pins ^ verify) & mask) == 0) break;

        // print message if more than two retries
        if (++ retry > 2) {
            fprintf (stderr, "PhysLib::writecon: verify %c error %d  sb %08X  but is %08X\n", CONLETS[c], retry, pins & mask, verify & mask);
        }
        if (retry > 9) {
            abort ();
        }
    }

    uint32_t endcycle = rdcyc ();
    writecount ++;
    writecycles += endcycle - startcycle;
    allowsigint ();

    return true;
}

// the iowarrior chips get really bent out of shape if the access functions are aborted
// so block signals while they are running
void PhysLib::blocksigint ()
{
    if (sigprocmask (SIG_BLOCK, &sigintmask, NULL) < 0) abort ();
}

void PhysLib::allowsigint ()
{
    if (sigprocmask (SIG_UNBLOCK, &sigintmask, NULL) < 0) abort ();
}
