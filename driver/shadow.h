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
#ifndef _SHADOW_H
#define _SHADOW_H

#include "gpiolib.h"
#include "miscdefs.h"

struct Shadow {
    enum State {
        RESET0, RESET1, FETCH1, FETCH2, 
        BCC1, ARITH1,
        STORE1, STORE2, LDA1, LOAD1, LOAD2, LOAD3,
        WRPS1, RDPS1, IRET1, IRET2, IRET3, IRET4,
        HALT1, IREQ1, IREQ2, IREQ3, IREQ4, IREQ5
    };

    bool chkacid;
    bool printinstr;
    bool printstate;

    State state;
    uint16_t ir;
    uint16_t regs[8];
    uint16_t psw;

    Shadow ();
    void open (GpioLib *gpiolib);
    void reset ();
    bool check (uint32_t sample);
    bool clock (uint16_t mq, bool irq);
    uint64_t getcycles ();
    uint32_t readgpio ();

private:
    bool fatal;
    GpioLib *gpiolib;
    int rawcmid;
    int rawcout;
    int rawvout;
    uint16_t alu;
    uint16_t newpsw;
    uint64_t cycle;

    State endOfInst (bool irq);
    bool branchTrue ();
    void loadPsw (uint16_t newpsw);
    void puque ();
    void check_g (uint32_t sample, uint32_t gtest, uint32_t gdont);
    void check_a (uint16_t asb);
    void check_b (uint16_t bsb);
    void check_c (uint32_t csb, uint32_t cdc);
    void check_d (uint16_t dsb);
    void check_d (uint32_t dsb, uint32_t mask);
    void check_i (uint16_t isb);

    static char const *boolstr (bool b);
    static char const *statestr (State s);
};

#endif
