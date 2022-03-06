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

// lists out serial numbers for all found io-warrior-56 boards plugged into the USB
// also sends test values out the first one found (#0) and reads them in from second one found (#1)

// as root:
//  ./iow56list_`uname -m`

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iowkit.h>

#include "miscdefs.h"
#include "rdcyc.h"

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

static void writepins (IOWKIT_HANDLE iowh, uint32_t val);
static uint32_t readpins (IOWKIT_HANDLE iowh);
static void get_iow_name(ULONG pid, char* name);

int main ()
{
    if (getuid () != 0) {
        printf ("must run as root to access iow56s\n");
    }

    rdcycinit ();
    uint32_t rcfreq = rdcycfreq ();
    printf ("rdcycfreq=%u\n", rcfreq);

    IowKitOpenDevice ();
    ULONG numdevs = IowKitGetNumDevs ();
    printf ("numdevs=%lu\n", numdevs);

    IOWKIT_HANDLE iowh, iowh0, iowh1;
    iowh0 = NULL;
    iowh1 = NULL;
    for (ULONG j = 0; j < numdevs; j ++) {
        printf ("device %lu\n", j);
        iowh = IowKitGetDeviceHandle (j + 1);

        char pidname[64];
        ULONG pid = IowKitGetProductId (iowh);
        get_iow_name (pid, pidname);
        printf ("  pid %s\n", pidname);

        char sns[9];
        uint16_t snb[9];
        if (IowKitGetSerialNumber (iowh, snb)) {
            for (int k = 0; k < 9; k ++) {
                sns[k] = snb[k];
            }
            printf ("  sn %s\n", sns);
        }

        writepins (iowh, 0xFFFFFFFF);
        usleep (2000);
        uint32_t val = readpins (iowh);
        printf ("  pins %08X\n", val);
        switch (j) {
            case 0: iowh0 = iowh; break;
            case 1: iowh1 = iowh; break;
        }
    }

    if ((iowh0 != NULL) && (iowh1 != NULL)) {
        uint32_t v = 0;
        while (true) {
            v += 987654321;
            writepins (iowh0, v);
            uint32_t begdelay = rdcyc ();
            int nreads = 0;
            uint32_t w;
            while (true) {
                w = readpins (iowh0);
                if (w == v) break;
                if (++ nreads > 99) break;
            }
            uint32_t enddelay = rdcyc ();
            uint32_t u = readpins (iowh1);
            uint32_t us = (enddelay - begdelay) * 1000000ULL / rcfreq;
            if (w == v) {
                printf (" v=%08X   delay=%8u=%5uus   u=%08X   nreads=%d\n", v, enddelay - begdelay, us, u, nreads);
            } else {
                printf (" v=%08X   delay=%8u=%5uus   u=%08X   w=%08X\n", v, enddelay - begdelay, us, u, w);
            }
        }
    }

    return 0;
}

static void writepins (IOWKIT_HANDLE iowh, uint32_t val)
{
    IOWKIT56_IO_REPORT updallpins;

    memset (&updallpins, 0xFF, sizeof updallpins);
    updallpins.ReportID = 0;

    for (int j = 0; j < 32; j ++) {
        if (! (val & 1)) {
            uint8_t pm = pinmapping[j];
            int p = pm >> 3;            // P0,P1,..,P6
            int m = pm & 7;             // .0,.1,...,.7
            updallpins.Bytes[p] &= ~ (1 << m);
        }
        val >>= 1;
    }

    ULONG rc = IowKitWrite (iowh, IOW_PIPE_IO_PINS, (char *) &updallpins, sizeof updallpins);
    if (rc < sizeof updallpins) {
        fprintf (stderr, "writepins: error %lu writing updallpins: %m\n", rc);
        abort ();
    }
}

static uint32_t readpins (IOWKIT_HANDLE iowh)
{
    IOWKIT56_SPECIAL_REPORT iowallpins;
    ULONG rc;

    // read all the 2x20 connector pins
    rc = IowKitWrite (iowh, IOW_PIPE_SPECIAL_MODE, (char *) &reqallpins, sizeof reqallpins);
    if (rc != sizeof reqallpins) {
        fprintf (stderr, "PhysLib::readcon: error %lu writing reqallpins: %m\n", rc);
        abort ();
    }
    rc = IowKitRead (iowh, IOW_PIPE_SPECIAL_MODE, (char *) &iowallpins, sizeof iowallpins);
    if (rc != sizeof iowallpins) {
        fprintf (stderr, "PhysLib::readcon: error %lu reading iowallpins: %m\n", rc);
        abort ();
    }
    if (iowallpins.ReportID != 255) {
        fprintf (stderr, "PhysLib::readcon: got report id %02X reading iowallpins\n", iowallpins.ReportID);
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

// .../samples/info.cpp
static void get_iow_name(ULONG pid, char* name)
{
    switch (pid) {
        case IOWKIT_PID_IOW24: strcpy(name, "IO-Warrior24"); break;
        case IOWKIT_PID_IOWPV1: strcpy(name, "IO-Warrior24PV"); break;
        case IOWKIT_PID_IOW40: strcpy(name, "IO-Warrior40"); break;
        case IOWKIT_PID_IOW56: strcpy(name, "IO-Warrior56"); break;
        case IOWKIT_PID_IOW28: strcpy(name, "IO-Warrior28"); break;
        case IOWKIT_PID_IOW100: strcpy(name, "IO-Warrior100"); break;
        default: sprintf (name, "unknown %lu", pid);
    }
}
