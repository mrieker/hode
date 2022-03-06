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
// keeps track of what internal cpu state should be, cycle by cycle
// and verifies all connector pins cycle by cycle

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "shadow.h"

#define ARITHUPDATESPC 0
#define LDxR7IMMINCSPC 0

Shadow::Shadow ()
{
    printinstr = false;
    printstate = false;
}

// gpiolib = instance of PhysLib: verify physical cpu board
//                       PipeLib: verify simulated cpu board
void Shadow::open (GpioLib *gpiolib)
{
    this->gpiolib = gpiolib;
}

// force cpu to RESET0 state
void Shadow::reset ()
{
    state = RESET0;
    cycle = 0;
    fatal = false;
}

// check values on all the connectors at end of the current cycle
//  input:
//   sample = what is on GPIO pins right now
bool Shadow::check (uint32_t sample)
{
    // check the pin values based on what state we were in during the cycle that is ending
    switch (state) {
        case RESET0: {
            check_g (sample, G_DENA, G_DATA | G_IRQ | G_WORD);
            check_c (0, C_IRQ | C_PSW_IE | C_PSW_BT | C_ALU_BONLY | C_MEM_WORD);
            break;
        }

        case RESET1: {
            check_g (sample, G_DENA, G_IRQ | G_WORD);
            check_b (0);                    // bmux should be outputting 0
            check_c (C_BMUX_CON_0 | C_ALU_BONLY | C_GPRS_WE7 | C_PSW_WE | C__PSW_IECLR, C_IRQ | C_PSW_BT | C_MEM_WORD);
            check_d (0);                    // alu should be outputting 0
            break;
        }

        case FETCH1: {
            check_g (sample, regs[7] * G_DATA0 | G_DENA | G_WORD | G_READ, G_IRQ);
            check_a (regs[7]);              // alu A input should be the PC
            check_b (0);                    // alu B input should be a zero
            check_c (C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            check_d (regs[7]);              // alu output should be the PC
            break;
        }

        case FETCH2: {
            ir = sample / G_DATA0;          // save opcode sent to cpu this cycle
            check_g (sample, G__QENA, G_IRQ | G_WORD | G_DATA);
            check_a (regs[7]);              // alu A input should be the PC
            check_b (2);                    // alu B input should be a 2
            check_c (C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7 | C__IR_WE, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);                   // ir should have what we read from memory
            check_d (regs[7] + 2);          // alu output should be incremented PC
            break;
        }

        case BCC1: {
            uint16_t brof = ((ir & 0x3FE) ^ 0x200) - 0x200;
            uint16_t brto = regs[7] + brof;
            check_g (sample, brto * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_a (regs[7]);
            check_b (brof);
            if (branchTrue ()) {
                check_c (C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD | C_GPRS_WE7 | C_PSW_BT, C_IRQ | C_MEM_WORD | C_PSW_IE);
            } else {
                check_c (C_GPRS_REA7 | C_BMUX_IRBROF | C_ALU_ADD, C_IRQ | C_PSW_IE | C_MEM_WORD);
            }
            check_i (ir);
            check_d (brto);
            break;
        }

        case ARITH1: {
            uint16_t asb = regs[(ir>>REGA)&7];
            uint16_t bsb = regs[(ir>>REGB)&7];
            check_g (sample, alu * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_a (asb);
            check_b (bsb);
            uint32_t csb = C_GPRS_REA | C_GPRS_REB | C_ALU_IRFC | C_PSW_ALUWE;
            if (((ir >> REGD) & 7) != 7) csb |= C_GPRS_WED;
            check_c (csb, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);

            // things generated in module aluboard()
            uint16_t rawshrout = asb & 1;
            uint16_t rawshrmid = (asb >> 8) & 1;

            // things generated in module psw()
            uint16_t bnot     = (ir >> 2) & (ir >> 0) & 1;
            uint16_t rawcin   = ((ir >> 2) & (ir >> 1) & (psw | (~ ir >> 3)) & 1) ^ bnot;
            uint16_t rawshrin = ((asb >> 15) & 1 & ir) | (psw & 1 & (ir >> 1));

            // see what bits should be on the D connector
            ASSERT (D_DBUS == 0xFFFF);
            uint32_t dsb = alu;
            if (rawcin)      dsb |= D_RAWCIN;
            if (rawshrin)    dsb |= D_RAWSHRIN;
            if (rawshrout)   dsb |= D_RAWSHROUT;
            if (rawshrmid)   dsb |= D_RAWSHRMIDI | D_RAWSHRMIDO;
            if (rawcout > 0) dsb |= D_RAWCOUT;
            if (rawvout > 0) dsb |= D_RAWVOUT;
            if (rawcmid > 0) dsb |= D_RAWCMIDI | D_RAWCMIDO;

            uint32_t mask = D_DBUS | D_RAWCIN | D_RAWSHRIN | D_RAWSHROUT | D_RAWSHRMIDI | D_RAWSHRMIDO;
            if (rawcout >= 0) mask |= D_RAWCOUT;
            if (rawvout >= 0) mask |= D_RAWVOUT;
            if (rawcmid >= 0) mask |= D_RAWCMIDI | D_RAWCMIDO;
            check_d (dsb, mask);

            break;
        }

        case STORE1: {
            uint16_t rega = regs[(ir>>REGA)&7];
            uint16_t lsof = ((ir & 0x7F) ^ 0x40) - 0x40;
            uint16_t addr = rega + lsof;
            if (ir & 0x2000) {
                check_g (sample, addr * G_DATA0 | G_DENA | G_WRITE, G_IRQ);
            } else {
                check_g (sample, addr * G_DATA0 | G_DENA | G_WRITE | G_WORD, G_IRQ);
            }
            check_a (rega);
            check_b (lsof);
            if (ir & 0x2000) {
                check_c (C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE, C_IRQ | C_PSW_BT | C_PSW_IE);
            } else {
                check_c (C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_WRITE | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            }
            check_i (ir);
            check_d (addr);
            break;
        }

        case STORE2: {
            uint16_t val = regs[(ir>>REGD)&7];
            check_g (sample, val * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_b (val);
            check_c (C_GPRS_RED2B | C_ALU_BONLY, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (val);
            break;
        }

        case LDA1: {
            uint16_t rega = regs[(ir>>REGA)&7];
            uint16_t lsof = ((ir & 0x7F) ^ 0x40) - 0x40;
            uint16_t addr = rega + lsof;
            check_g (sample, addr * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_a (rega);
            check_b (lsof);
            check_c (C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_GPRS_WED, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (addr);
            break;
        }

        case LOAD1: {
            uint16_t rega = regs[(ir>>REGA)&7];
            uint16_t lsof = ((ir & 0x7F) ^ 0x40) - 0x40;
            uint16_t addr = rega + lsof;
            if (ir & 0x2000) {
                check_g (sample, addr * G_DATA0 | G_DENA | G_READ, G_IRQ);
            } else {
                check_g (sample, addr * G_DATA0 | G_DENA | G_READ | G_WORD, G_IRQ);
            }
            check_a (rega);
            check_b (lsof);
            if (ir & 0x2000) {
                check_c (C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ, C_IRQ | C_PSW_BT | C_PSW_IE);
            } else {
                check_c (C_GPRS_REA | C_BMUX_IRLSOF | C_ALU_ADD | C_MEM_READ | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            }
            check_i (ir);
            check_d (addr);
            break;
        }

        case LOAD2: {
            uint16_t mq = sample / G_DATA0;
            if (ir & 0x2000) {
                mq &= 0xFF;
                if (ir & 0x4000) mq = (mq ^ 0x80) - 0x80;
            }
            check_g (sample, G__QENA, G_IRQ | G_DATA | G_WORD);
            check_b (mq);
            check_c (C_MEM_REB | C_ALU_BONLY | C_GPRS_WED, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (mq);
            break;
        }

        case LOAD3: {
            uint16_t newpc = regs[7] + 2;
            check_g (sample, newpc * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_a (regs[7]);              // alu A input should be the PC
            check_b (2);                    // alu B input should be a 2
            check_c (C_GPRS_REA7 | C_BMUX_CON_2 | C_ALU_ADD | C_GPRS_WE7, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (newpc);                // alu output should be incremented PC
            break;
        }

        case IREQ1: {
            check_g (sample, 0xFFFE * G_DATA0 | G_DENA | G_WRITE | G_WORD, G_IRQ);
            check_b (0xFFFE);
            check_c (C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            check_d (0xFFFE);
            break;
        }

        case IREQ2: {
            check_g (sample, psw * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_b (psw);
            check_c (C__PSW_REB | C_ALU_BONLY, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_d (psw);
            break;
        }

        case IREQ3: {
            check_g (sample, 0xFFFC * G_DATA0 | G_DENA | G_WRITE | G_WORD, G_IRQ);
            check_c (C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_WRITE | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            check_b (0xFFFC);
            check_d (0xFFFC);
            break;
        }

        case IREQ4: {
            check_g (sample, regs[7] * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_a (regs[7]);
            check_b (0);
            check_c (C__PSW_IECLR | C_GPRS_REA7 | C_BMUX_CON_0 | C_ALU_ADD, C_IRQ | C_PSW_BT | C_MEM_WORD);
            check_d (regs[7]);
            break;
        }

        case IREQ5: {
            check_g (sample, 2 * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_b (2);
            check_c (C_BMUX_CON_2 | C_ALU_BONLY | C_GPRS_WE7, C_IRQ | C_PSW_BT | C_MEM_WORD);
            check_d (2);
            break;
        }

        case IRET1: {
            check_g (sample, 0xFFFC * G_DATA0 | G_DENA | G_READ | G_WORD, G_IRQ);
            check_b (0xFFFC);
            check_c (C_BMUX_CON__4 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            check_i (ir);
            check_d (0xFFFC);
            break;
        }

        case IRET2: {
            uint16_t mq = sample / G_DATA0;
            check_g (sample, G__QENA, G_IRQ | G_DATA | G_WORD);
            check_b (mq);
            check_c (C_MEM_REB | C_ALU_BONLY | C_GPRS_WE7, C_IRQ | C_MEM_WORD | C_PSW_BT | C_PSW_IE);
            check_i (ir);
            check_d (mq);
            break;
        }

        case IRET3: {
            check_g (sample, 0xFFFE * G_DATA0 | G_DENA | G_READ | G_WORD, G_IRQ);
            check_b (0xFFFE);
            check_c (C_BMUX_CON__4 | C_BMUX_CON_2 | C_ALU_BONLY | C_MEM_READ | C_MEM_WORD, C_IRQ | C_PSW_BT | C_PSW_IE);
            check_i (ir);
            check_d (0xFFFE);
            break;
        }

        case IRET4: {
            uint16_t mq = sample / G_DATA0;
            check_g (sample, G__QENA, G_IRQ | G_DATA | G_WORD);
            check_b (mq);
            check_c (C_MEM_REB | C_ALU_BONLY | C_PSW_WE, C_IRQ | C_MEM_WORD | C_PSW_BT | C_PSW_IE);
            check_i (ir);
            check_d (mq);
            break;
        }

        case HALT1: {
            uint16_t regb = regs[(ir>>REGB)&7];
            check_g (sample, regb * G_DATA0 | G_DENA | G_HALT, G_IRQ | G_WORD);
            check_b (regb);
            check_c (C_HALT | C_GPRS_REB | C_ALU_BONLY, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (regb);
            break;
        }

        case WRPS1: {
            uint16_t regb = regs[(ir>>REGB)&7];
            check_g (sample, regb * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_b (regb);
            check_c (C_GPRS_REB | C_ALU_BONLY | C_PSW_WE, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (regb);
            break;
        }

        case RDPS1: {
            check_g (sample, psw * G_DATA0 | G_DENA, G_IRQ | G_WORD);
            check_b (psw);
            check_c (C__PSW_REB | C_ALU_BONLY | C_GPRS_WED, C_IRQ | C_PSW_BT | C_MEM_WORD | C_PSW_IE);
            check_i (ir);
            check_d (psw);
            break;
        }

        default: abort ();
    }

    if (fatal) {
        uint32_t pins = sample;
        fprintf (stderr, "Shadow::check: G=%08X %s\n", pins, GpioLib::decocon (CON_G, pins).c_str ());
        for (int c = 0; c < 4; c ++) {
            if (gpiolib->readcon ((IOW56Con) c, &pins)) {
                fprintf (stderr, "Shadow::check: %c=%08X %s\n", CONLETS[c],
                        pins, GpioLib::decocon ((IOW56Con) c, pins).c_str ());
            } else {
                fprintf (stderr, "Shadow::check: %c=missing\n", CONLETS[c]);
            }
        }
        puque ();
    }

    return fatal;
}

// clock to next state
// call right after calling check()
//  input:
//   mq  = memory data sent to CPU during last cycle
//   irq = interrupt request line sent to CPU during last cycle
bool Shadow::clock (uint16_t mq, bool irq)
{
    // at end of state ... do ... then go to state ...
    switch (state) {
        case RESET0: {
            state = RESET1;
            break;
        }
        case RESET1: {
            regs[7] = 0;
            loadPsw (0);
            state = FETCH1;
            break;
        }
        case FETCH1: {
            state = FETCH2;
            break;
        }
        case FETCH2: {
            regs[7] = alu;
            ir = mq;
            switch ((ir >> 13) & 7) {
                case 0: {
                    if ((ir & 0b0001110000000001) != 0) {
                        state = BCC1; break;
                    } else {
                        switch ((ir & 0b0000000000001110) >> 1) {
                            case 0: state = HALT1; break;
                            case 1: state = IRET1; break;
                            case 2: state = WRPS1; break;
                            case 3: state = RDPS1; break;
                            default: {
                                fprintf (stderr, "Shadow::clock: bad opcode %04X\n", ir);
                                puque ();
                                return true;
                            }
                        }
                    }
                    break;
                }
                case 1: state = ARITH1; break;
                case 2:
                case 3: state = STORE1; break;
                case 4: state = LDA1; break;
                case 5:
                case 6:
                case 7: state = LOAD1; break;
            }
            break;
        }

        case BCC1: {
            if (branchTrue ()) {
                regs[7] = alu;
            }
            state = endOfInst (irq);
            break;
        }
        case ARITH1: {
            uint16_t regd = (ir >> REGD) & 7;
            if (ARITHUPDATESPC || (regd != 7)) regs[regd] = alu;
            loadPsw (newpsw);
            state = endOfInst (irq);
            break;
        }
        case STORE1: {
            state = STORE2;
            break;
        }
        case STORE2: {
            state = endOfInst (irq);
            break;
        }
        case LDA1: {
            regs[(ir>>REGD)&7] = alu;
            state = endOfInst (irq);
            break;
        }
        case LOAD1: {
            state = LOAD2;
            break;
        }
        case LOAD2: {
            if ((ir & 0b0010000000000000) != 0) {
                mq &= 0xFF;
                if ((ir & 0b0100000000000000) != 0) {
                    mq = (uint16_t) ((mq ^ 0x80) - 0x80);
                }
            }
            regs[(ir>>REGD)&7] = mq;
            if (((ir & 0x7F) == 0) && (((ir >> REGA) & 7) == 7) && (LDxR7IMMINCSPC || (((ir >> REGD) & 7) != 7))) {
                state = LOAD3;
            } else {
                state = endOfInst (irq);
            }
            break;
        }
        case LOAD3: {
            regs[7] = alu;
            state = endOfInst (irq);
            break;
        }

        case WRPS1: {
            state = endOfInst (irq);
            loadPsw (alu);
            break;
        }
        case RDPS1: {
            regs[(ir>>REGD)&7] = alu;
            state = endOfInst (irq);
            break;
        }
        case IRET1: {
            state = IRET2;
            break;
        }
        case IRET2: {
            regs[7] = mq;
            state = IRET3;
            break;
        }
        case IRET3: {
            state = IRET4;
            break;
        }
        case IRET4: {
            state = endOfInst (irq);
            loadPsw (mq);
            break;
        }

        case HALT1: {
            state = endOfInst (irq);
            break;
        }

        case IREQ1: {
            state = IREQ2;
            break;
        }
        case IREQ2: {
            state = IREQ3;
            break;
        }
        case IREQ3: {
            psw  &= 0x7FFF;
            state = IREQ4;
            break;
        }
        case IREQ4: {
            state = IREQ5;
            break;
        }
        case IREQ5: {
            regs[7] = alu;
            state = FETCH1;
            break;
        }
    }

    __atomic_add_fetch (&cycle, 1, __ATOMIC_RELAXED);

    if (printinstr | printstate) {
        switch (state) {
            case FETCH1:
            case IREQ1: {
                if (printstate) {
                    printf ("%llu STARTING STATE %-6s  R0=%04X R1=%04X R2=%04X R3=%04X R4=%04X R5=%04X R6=%04X  PC=%04X  PS=%04X\n",
                        cycle, statestr (state), regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], psw);
                } else {
                    printf ("%llu R0=%04X R1=%04X R2=%04X R3=%04X R4=%04X R5=%04X R6=%04X  PC=%04X  PS=%04X  ",
                        cycle, regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], psw);
                }
                break;
            }
            case IREQ2: {
                if (printstate) {
                    printf ("%llu STARTING STATE %s\n", cycle, statestr (state));
                } else  {
                    printf ("**INTERRUPT**\n");
                }
                break;
            }
            case BCC1:
            case ARITH1:
            case LDA1:
            case LOAD1:
            case IRET1:
            case HALT1:
            case WRPS1:
            case RDPS1: {
                if (printstate) {
                    printf ("%llu STARTING STATE %-6s  IR=%04X  %s\n", cycle, statestr (state), ir, disassemble (ir).c_str ());
                } else {
                    printf ("%s\n", disassemble (ir).c_str ());
                }
                break;
            }
            case STORE1: {
                if (printstate) {
                    printf ("%llu STARTING STATE %-6s  IR=%04X  %s\n", cycle, statestr (state), ir, disassemble (ir).c_str ());
                } else {
                    uint16_t addr = regs[(ir>>REGA)&7] + ((ir & 0x7F) ^ 0x40) - 0x40;
                    int      size = (ir & 0x2000) ? 2 : 4;
                    uint16_t data = regs[(ir>>REGD)&7] & ((ir & 0x2000) ? 0x00FF : 0xFFFF);
                    printf ("%-16s  %04X <= %0*X\n", disassemble (ir).c_str (), addr, size, data);
                }
                break;
            }
            default: {
                if (printstate) {
                    printf ("%llu STARTING STATE %s\n", cycle, statestr (state));
                }
                break;
            }
        }
    }

    // at beginning of state ... do ...
    switch (state) {
        case RESET0: case RESET1: break;
        case FETCH1: {
            alu = regs[7];
            break;
        }
        case FETCH2: {
            alu = (uint16_t) (regs[7] + 2);
            break;
        }
        case BCC1: {
            alu = (uint16_t) (regs[7] + ((ir & 0x03FE) ^ 0x0200) - 0x0200);
            break;
        }
        case ARITH1: {
            rawcmid = -1;
            rawcout = -1;
            rawvout = -1;
            uint32_t ua = regs[(ir>>REGA)&7] & 0xFFFF;
            uint32_t ub = regs[(ir>>REGB)&7] & 0xFFFF;
            sint32_t sa = (ua ^ 0x8000) - 0x8000;
            sint32_t sb = (ub ^ 0x8000) - 0x8000;
            bool newc = (psw & 1) != 0;
            bool newv = false;
            switch (ir & 15) {
                case  0: {  // lsr
                    alu = (uint16_t) (ua >> 1);
                    newc = (ua & 1) != 0;
                    break;
                }
                case  1: {  // asr
                    alu = (uint16_t) (sa >> 1);
                    newc = (ua & 1) != 0;
                    break;
                }
                case  2: {  // ror
                    alu = (uint16_t) ((ua >> 1) | (newc ? 0x8000 : 0));
                    newc = (ua & 1) != 0;
                    break;
                }
                case  4: {  // mov
                    alu = (uint16_t) ub;
                    break;
                }
                case  5: {  // neg
                    sint32_t ss = - sb;
                    alu = (uint16_t) ss;
                    newv = (ss < -32768) || (ss > 32767);
                    rawcmid = ((~ ub) & 0x00FF) + 1 > 0x00FF;
                    rawcout = ((~ ub) & 0xFFFF) + 1 > 0xFFFF;
                    rawvout = ((~ ub) & 0x7FFF) + 1 > 0x7FFF;
                    break;
                }
                case  6: {  // inc
                    sint32_t ss = sb + 1;
                    alu = (uint16_t) ss;
                    newv = (ss < -32768) || (ss > 32767);
                    rawcmid = (ub & 0x00FF) + 1 > 0x00FF;
                    rawcout = (ub & 0xFFFF) + 1 > 0xFFFF;
                    rawvout = (ub & 0x7FFF) + 1 > 0x7FFF;
                    break;
                }
                case  7: {  // com
                    alu = (uint16_t) ~ sb;
                    break;
                }
                case  8: {
                    alu = (uint16_t) (ua | ub);
                    break;
                }
                case  9: {
                    alu = (uint16_t) (ua & ub);
                    break;
                }
                case 10: {
                    alu = (uint16_t) (ua ^ ub);
                    break;
                }
                case 12: {  // ADD
                    newc = false;
                    // falllthrough
                }
                case 14: {  // ADC
                    sint32_t c = newc ? 1 : 0;
                    sint32_t ss = sa + sb + c;
                    uint32_t us = ua + ub + c;
                    alu = (uint16_t) ss;
                    newc = us > 65535;
                    newv = (ss < -32768) || (ss > 32767);
                    rawcmid = ((ua & 0x00FF) + (ub & 0x00FF) + c) > 0x00FF;
                    rawcout = ((ua & 0xFFFF) + (ub & 0xFFFF) + c) > 0xFFFF;
                    rawvout = ((ua & 0x7FFF) + (ub & 0x7FFF) + c) > 0x7FFF;
                    break;
                }
                case 13: {  // SUB
                    newc = false;
                    // falllthrough
                }
                case 15: {  // SBB
                    sint32_t c = newc ? 1 : 0;
                    sint32_t ss = sa - sb - c;
                    uint32_t us = ua - ub - c;
                    alu = (uint16_t) ss;
                    newc = us > 65535;
                    newv = (ss < -32768) || (ss > 32767);
                    rawcmid = ((ua & 0x00FF) + ((~ ub) & 0x00FF) + (c ^ 1)) > 0x00FF;
                    rawcout = ((ua & 0xFFFF) + ((~ ub) & 0xFFFF) + (c ^ 1)) > 0xFFFF;
                    rawvout = ((ua & 0x7FFF) + ((~ ub) & 0x7FFF) + (c ^ 1)) > 0x7FFF;
                    break;
                }
                default: {
                    fprintf (stderr, "Shadow::check: bad opcode %04X\n", ir);
                    puque ();
                    return true;
                }
            }
            newpsw = (uint16_t) ((psw & 0x8000) |
                    ((alu & 0x8000) ? 8 : 0) |   // n
                        ((alu == 0) ? 4 : 0) |   // z
                              (newv ? 2 : 0) |   // v
                              (newc ? 1 : 0));   // c
            break;
        }

        case STORE1: {
            alu = (uint16_t) (regs[(ir>>REGA)&7] + ((ir & 0x7F) ^ 0x40) - 0x40);
            break;
        }
        case STORE2: {
            alu = regs[(ir>>REGD)&7];
            break;
        }
        case LDA1: {
            int num = (ir >> REGA) & 7;
            int ofs = ((ir & 0x7F) ^ 0x40) - 0x40;
            alu = (uint16_t) (regs[num] + ofs);
            break;
        }
        case LOAD1: {
            alu = (uint16_t) (regs[(ir>>REGA)&7] + ((ir & 0x7F) ^ 0x40) - 0x40);
            break;
        }
        case LOAD2: break;
        case LOAD3: {
            alu = (uint16_t) (regs[7] + 2);
            break;
        }
        case WRPS1: {
            alu = regs[(ir>>REGB)&7];
            break;
        }
        case HALT1: {
            alu = regs[(ir>>REGB)&7];
            break;
        }
        case RDPS1: {
            alu = psw;
            break;
        }
        case IRET1: {
            alu = (uint16_t) 0xFFFC;
            break;
        }
        case IRET2: break;
        case IRET3: {
            alu = (uint16_t) 0xFFFE;
            break;
        }
        case IRET4: break;
        case IREQ1: {
            alu = (uint16_t) 0xFFFE;
            break;
        }
        case IREQ2: {
            alu = psw;
            break;
        }
        case IREQ3: {
            alu = (uint16_t) 0xFFFC;
            break;
        }
        case IREQ4: {
            alu = regs[7];
            break;
        }
        case IREQ5: {
            alu = (uint16_t) 0x0002;
            break;
        }
    }

    return fatal;
}

uint64_t Shadow::getcycles ()
{
    return __atomic_load_n (&cycle, __ATOMIC_RELAXED);
}

Shadow::State Shadow::getstate ()
{
    return state;
}

uint16_t Shadow::getreg (uint32_t i)
{
    assert (i < 8);
    return regs[i];
}

// get what should be on GPIO connector right now, assuming hw has had time to settle in new state
// assume GPIO connector is turned to connect data pins to ALU output
// returns what check() is expecting
uint32_t Shadow::readgpio ()
{
    switch (state) {
        case RESET0:
        case RESET1: {
            return 0;
        }

        case STORE1: {
            return alu * G_DATA0 | G_DENA | G_WRITE | ((ir & 0x2000) ? 0 : G_WORD);
        }

        case LOAD1: {
            return alu * G_DATA0 | G_DENA | G_READ | ((ir & 0x2000) ? 0 : G_WORD);
        }

        case IREQ1:
        case IREQ3: {
            return alu * G_DATA0 | G_DENA | G_WRITE | G_WORD;
        }

        case FETCH1:
        case IRET1:
        case IRET3: {
            return alu * G_DATA0 | G_DENA | G_READ | G_WORD;
        }

        case FETCH2:
        case LOAD2:
        case IRET2:
        case IRET4: {
            return G__QENA;
        }

        case HALT1: {
            return alu * G_DATA0 | G_DENA | G_HALT;
        }

        case BCC1:
        case ARITH1:
        case STORE2:
        case LDA1:
        case LOAD3:
        case IREQ2:
        case IREQ4:
        case IREQ5:
        case WRPS1:
        case RDPS1: {
            return alu * G_DATA0 | G_DENA;
        }

        default: abort ();
    }
}

// end of instruction's last cycle, transition to either fetch or start interrupt
Shadow::State Shadow::endOfInst (bool irq)
{
    return (irq && (psw & 0x8000)) ? IREQ1 : FETCH1;
}

// return that the branch condition in IR vs condition codes in PSW is true
bool Shadow::branchTrue ()
{
    bool N = (psw & 8) != 0;
    bool Z = (psw & 4) != 0;
    bool V = (psw & 2) != 0;
    bool C = (psw & 1) != 0;
    bool bt = false;
    switch ((ir >> 10) & 7) {
        case 1: {   // beq
            bt = Z;
            break;
        }
        case 2: {   // blt
            bt = N ^ V;
            break;
        }
        case 3: {   // ble
            bt = (N ^ V) | Z;
            break;
        }
        case 4: {   // blo
            bt = C;
            break;
        }
        case 5: {   // blos
            bt = C | Z;
            break;
        }
        case 6: {   // bmi
            bt = N;
            break;
        }
        case 7: {   // bvs
            bt = V;
            break;
        }
    }
    if ((ir & 1) != 0) bt = ! bt;
    return bt;
}

// fix up psw bits before writing it
void Shadow::loadPsw (uint16_t newpsw)
{
    psw = (uint16_t) (newpsw | 0x7FF0);
}

void Shadow::puque ()
{
    fprintf (stderr, "Shadow::puque: shadow state=%s psw=%04X cycle=%llu\n", statestr (state), psw, cycle);
    fprintf (stderr, "Shadow::puque: shadow ir=%04X  %s\n", ir, disassemble (ir).c_str ());
    fprintf (stderr, "Shadow::puque: shadow R0=%04X R1=%04X R2=%04X R3=%04X\n", regs[0], regs[1], regs[2], regs[3]);
    fprintf (stderr, "Shadow::puque: shadow R4=%04X R5=%04X R6=%04X R7=%04X\n", regs[4], regs[5], regs[6], regs[7]);
    fatal = true;
}

char const *Shadow::boolstr (bool b)
{
    return b ? "true" : "false";
}

char const *Shadow::statestr (State s)
{
    switch (s) {
        case RESET0: return "RESET0";
        case RESET1: return "RESET1";
        case FETCH1: return "FETCH1";
        case FETCH2: return "FETCH2";
        case BCC1: return "BCC1";
        case ARITH1: return "ARITH1";
        case STORE1: return "STORE1";
        case STORE2: return "STORE2";
        case LDA1: return "LDA1";
        case LOAD1: return "LOAD1";
        case LOAD2: return "LOAD2";
        case LOAD3: return "LOAD3";
        case WRPS1: return "WRPS1";
        case RDPS1: return "RDPS1";
        case IRET1: return "IRET1";
        case IRET2: return "IRET2";
        case IRET3: return "IRET3";
        case IRET4: return "IRET4";
        case HALT1: return "HALT1";
        case IREQ1: return "IREQ1";
        case IREQ2: return "IREQ2";
        case IREQ3: return "IREQ3";
        case IREQ4: return "IREQ4";
        case IREQ5: return "IREQ5";
    }
    return "bad state enum";
}

// check that the GPIO pins are what they should be
//  input:
//   sample = as read from GPIO connector
//   gtest  = bits we require to be set
//   gdont  = bits we don't care about
void Shadow::check_g (uint32_t sample, uint32_t gtest, uint32_t gdont)
{
    uint32_t gmask =
            G_CLK   |
            G_RESET |
            G_DATA  |
            G_IRQ   |
            G_WORD  |
            G_HALT  |
            G_READ  |
            G_WRITE;

    uint32_t con = sample;
    if (((con ^ gtest) & gmask & ~ gdont) != 0) {
        fprintf (stderr, "Shadow::check_g: actual=%08X gmask=%08X gdont=%08X gtest=%08X\n",
                con, gmask, gdont, gtest);
        con   &= gmask & ~ gdont;
        gtest &= gmask & ~ gdont;
        fprintf (stderr, "Shadow::check_g:  actual=%08X  %s\n", con,   GpioLib::decocon (CON_G, con).c_str ());
        fprintf (stderr, "Shadow::check_g:  expect=%08X  %s\n", gtest, GpioLib::decocon (CON_G, gtest).c_str ());
        fprintf (stderr, "Shadow::check_g:  shadow state=%s cycle=%llu\n", statestr (state), cycle);
        fatal = true;
    }
}

// check that the A operand into the physical ALU is what it should be
void Shadow::check_a (uint16_t asb)
{
    if (! chkacid) return;

    uint32_t con;
    if (! gpiolib->readcon (CON_A, &con)) return;

    uint16_t ais = GpioLib::abussort (con);

    if (ais != asb) {
        fprintf (stderr, "Shadow::check_a: asb=%04X ais=%04X\n", asb, ais);
        fatal = true;
    }
}

// check that the B operand into the physical ALU is what it should be
void Shadow::check_b (uint16_t bsb)
{
    if (! chkacid) return;

    uint32_t con;
    if (! gpiolib->readcon (CON_A, &con)) return;

    uint16_t bis = GpioLib::bbussort (con);

    if (bis != bsb) {
        fprintf (stderr, "Shadow::check_b: bsb=%04X bis=%04X\n", bsb, bis);
        fatal = true;
    }
}

// check that the sequencer is outputting what control bits it should be
void Shadow::check_c (uint32_t ctest, uint32_t cdont)
{
    if (! chkacid) return;

    uint32_t con;
    if (! gpiolib->readcon (CON_C, &con)) return;

    uint32_t cmask =
            C_IRQ         |
            C_PSW_IE      |
            C_PSW_BT      |
            C_CLK2        |
            C_MEM_REB     |
            C_PSW_ALUWE   |
            C_ALU_IRFC    |
            C_GPRS_REB    |
            C_BMUX_IRBROF |
            C_GPRS_WED    |
            C_MEM_READ    |
            C_PSW_WE      |
            C_HALT        |
            C_GPRS_REA    |
            C_GPRS_RED2B  |
            C_ALU_BONLY   |
            C_BMUX_IRLSOF |
            C__PSW_REB    |
            C_BMUX_CON__4 |
            C_MEM_WORD    |
            C_ALU_ADD     |
            C_GPRS_WE7    |
            C_GPRS_REA7   |
            C_BMUX_CON_2  |
            C__IR_WE      |
            C_BMUX_CON_0  |
            C_MEM_WRITE   |
            C__PSW_IECLR;

    uint32_t cinvt =
            C__PSW_REB    |
            C__IR_WE      |
            C__PSW_IECLR;

    if (((con ^ cinvt ^ ctest) & cmask & ~ cdont) != 0) {
        fprintf (stderr, "Shadow::check_c: actual=%08X cinvt=%08X cmask=%08X cdont=%08X ctest=%08X\n",
                con, cinvt, cmask, cdont, ctest);
        fatal = true;
    }
}

// check that the physical instruction register is outputting what it should be
void Shadow::check_i (uint16_t isb)
{
    if (! chkacid) return;

    uint32_t con;
    if (! gpiolib->readcon (CON_I, &con)) return;

    uint16_t  iis = GpioLib::ibussort (con);
    uint16_t _iis = GpioLib::_ibussort (con);

    if ((iis != isb) || (_iis != (uint16_t) ~ isb)) {
        fprintf (stderr, "Shadow::check_i: isb=%04X iis=%04X _iis=%04X\n", isb, iis, _iis);
        fatal = true;
    }
}

// check that the physical ALU output is what it should be
void Shadow::check_d (uint16_t dsb)
{
    ASSERT (D_DBUS == 0xFFFF);
    check_d (dsb, D_DBUS);
}
void Shadow::check_d (uint32_t dsb, uint32_t mask)
{
    if (! chkacid) return;

    uint32_t con;
    if (! gpiolib->readcon (CON_D, &con)) return;

    ASSERT (D_DBUS == 0xFFFF);
    uint32_t dis = (con & ~ D_DBUS) | GpioLib::dbussort (con);

    dis &= mask;
    dsb &= mask;
    if (dis != dsb) {
        fprintf (stderr, "Shadow::check_d: dsb=%08X dis=%08X\n", dsb, dis);
        fatal = true;
    }
}
