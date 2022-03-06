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
// Uses a pipe to access the GPIO and 2x20 connectors
// Useful for testing the raspictl.cc program using the simulation function of NetGen.java

// writes to the GPIO pins cause 'force' commands to be written to the pipe
// these are read by the simulation function of NetGen.java to set the internal state of the pins

// reads of the GPIO and 2x20 connector pins cause 'examine' commands to be written to the pipe
// the simulation function of NetGen.java replies with the corresponding internal state of the pins

// the module definitions loaded into NetGen.java must contain:
//   module master (..., out conapins[15:00], out concpins[15:00], out conipins[15:00], out condpins[15:00])
//   ...must contain RasPi internal module instance raspi/rbc/rasbd

// cd ../modules
// rm -f simpipe
// mknod simpipe p
// cat simpipe | ./master.sh -sim - | ../driver/raspictl_x86_64 -printstate -sim simpipe umul.hex

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpiolib.h"

PipeLib::PipeLib (char const *sendname)
{
    this->sendname = sendname;
}

// open connections to the java program
void PipeLib::open ()
{
    // write simulator TCL commands to this pipe
    sendfile = fopen (sendname, "w");
    if (sendfile == NULL) {
        fprintf (stderr, "PipeLib::openpipes: error opening %s: %m\n", sendname);
        abort ();
    }
    setlinebuf (sendfile);

    // tell simulator what module to simulate
    // ...it has the gpio and 2x20 connectors as inputs/outputs
    fprintf (sendfile, "module master\n");
    ////fprintf (sendfile, "monitor concpins MEM_READ/seq/seqbd MEM_WRITE/seq/seqbd MEM_WORD/seq/seqbd HALT/seq/seqbd\n");
    ////fprintf (sendfile, "monitor RESET/seq/seqbd MEM_READ/seq/seqbd O16/ctla/seqbd I16/ctla/rasbd mread/rbc/rasbd MREAD/raspi/rbc/rasbd\n");

    // read simulator output from this pipe
    recvfile = stdin;
}

void PipeLib::close ()
{ }

// delay half a clock cycle
void PipeLib::halfcycle ()
{
    fprintf (sendfile, "run 500\n");
    gota = gotc = goti = gotd = false;
}

// read raspi gpio pins
uint32_t PipeLib::readgpio ()
{
    return examine ("raspi raspi/rbc/rasbd examine");
}

// write raspi gpio pins
void PipeLib::writegpio (bool wdata, uint32_t valu)
{
    fprintf (sendfile, "raspi raspi/rbc/rasbd force %X\n", valu);
}

// read value of the 32 pins of a 2x20 connector
//  input:
//   c = CON_A,C,I,D connector selection
//  output:
//   returns 32 bits for the pins
bool PipeLib::readcon (IOW56Con c, uint32_t *pins)
{
    switch (c) {
        case CON_A: {
            if (! gota) {
                vala = examine ("examine conapins");
                gota = true;
            }
            *pins = vala;
            return true;
        }
        case CON_C: {
            if (! gotc) {
                valc = examine ("examine concpins");
                gotc = true;
            }
            *pins = valc;
            return true;
        }
        case CON_I: {
            if (! goti) {
                vali = examine ("examine conipins");
                goti = true;
            }
            *pins = vali;
            return true;
        }
        case CON_D: {
            if (! gotd) {
                vald = examine ("examine condpins");
                gotd = true;
            }
            *pins = vald;
            return true;
        }
        default: abort ();
    }
}

bool PipeLib::writecon (IOW56Con c, uint32_t mask, uint32_t pins)
{
    abort ();
    return false;
}

// read value of the given simulator variable
//  input:
//   varname = name of simulator variable
//  output:
//   returns binary value
uint32_t PipeLib::examine (char const *varname)
{
    char linebuf[4096], *p, *q;
    uint32_t recvseq;

    fprintf (sendfile, "echo @@@ %u [%s]\n", ++ sendseq, varname);

    while (true) {
        if (fgets (linebuf, sizeof linebuf, recvfile) == NULL) {
            if (errno == 0) errno = EPIPE;
            fprintf (stderr, "PipeLib::readgpio: error reading reply: %m\n");
            abort ();
        }
        p = strstr (linebuf, "@@@ ");
        if (p != NULL) break;
        fputs (linebuf, stdout);
    }

    *p = 0;
    fputs (linebuf, stdout);
    p += 4;

    recvseq = strtoul (p, &q, 10);
    if ((recvseq != sendseq) || (*q != ' ')) {
        fprintf (stderr, "PipeLib::readgpio: sendseq=%u linebuf=@@@ %s", sendseq, p);
        abort ();
    }

    uint32_t pinmask = 0;
    while (true) {
        switch (*(++ q)) {
            case '\n': return pinmask;
            case '0':  pinmask += pinmask;     break;
            case 'x':
            case '1':  pinmask += pinmask + 1; break;
            default: {
                fprintf (stderr, "PipeLib::readgpio: bad linebuf=@@@ %s", p);
                abort ();
            }
        }
    }
}
