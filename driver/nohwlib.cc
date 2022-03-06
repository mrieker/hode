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
// simulate processor via shadow

#include "gpiolib.h"
#include "shadow.h"

NohwLib::NohwLib (Shadow *shadow)
{
    this->shadow = shadow;
}

void NohwLib::open ()
{ }

void NohwLib::close ()
{ }

void NohwLib::halfcycle ()
{ }

uint32_t NohwLib::readgpio ()
{
    uint32_t value = shadow->readgpio ();
    value = (value & ~ G_OUTS) | (gpiowritten & G_OUTS);
    if (value & G__QENA) {
        value = (value & ~ G_DATA) | (gpiowritten & G_DATA);
    }
    return value;
}

void NohwLib::writegpio (bool wdata, uint32_t value)
{
    gpiowritten = value | (wdata ? G__QENA : G_DATA);
}

bool NohwLib::readcon (IOW56Con c, uint32_t *pins)
{
    return false;
}

bool NohwLib::writecon (IOW56Con c, uint32_t mask, uint32_t pins)
{
    return false;
}
