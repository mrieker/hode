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
 * Program runs on raspberry pi to generate clock signal.
 * sudo insmod km/enabtsc.ko
 * sudo ./clockit [200]
 */

#include <stdio.h>
#include <stdlib.h>

#include "gpiolib.h"
#include "miscdefs.h"

int main (int argc, char **argv)
{
    uint32_t cpuhz = DEFCPUHZ;

    if (argc > 1) cpuhz = atoi (argv[1]);

    printf ("cpuhz=%u\n", cpuhz);

    GpioLib *gpio = new PhysLib (cpuhz);
    gpio->open ();

    while (1) {
        gpio->writegpio (false, G_RESET);
        gpio->halfcycle ();
        gpio->writegpio (false, G_RESET | G_CLK);
        gpio->halfcycle ();
    }

    return 0;
}
