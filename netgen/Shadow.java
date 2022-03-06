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
 * Shadow the CPU state while simulating
 * ...to verify CPU state as we go along.
 */

public class Shadow {
    public static enum State {
        RESET0, RESET1, FETCH1, FETCH2, 
        BCC1, ARITH1,
        STORE1, STORE2, LDA1, LOAD1, LOAD2, LOAD3,
        WRPS1, RDPS1, IRET1, IRET2, IRET3, IRET4,
        HALT1, IREQ1, IREQ2, IREQ3, IREQ4, IREQ5 }

    private final static int REGA = 10;
    private final static int REGD =  7;
    private final static int REGB =  4;

    public boolean printstate;

    private short alu;      // ALU output = going to memory
    private short ir;
    private short newpsw;   // new psw at end of ARITH1
    private short psw;
    private short[] regs = new short[8];
    private State state;

    public void reset ()
    {
        state = State.RESET0;
    }

    // check values received from the CPU at end of current cycle
    //  input:
    //   mread,mword,mwrite,md = values sampled from CPU just before positive clock edge
    public void check (boolean mread, boolean mword, boolean mwrite, short md)
    {
        boolean aluv = true;
        boolean mrd  = false;
        boolean mwd  = false;
        boolean mwr  = false;

        switch (state) {

            // alu output valid but is not a memory address
            case FETCH2:
            case BCC1:
            case ARITH1:
            case STORE2:
            case LDA1:
            case LOAD3:
            case HALT1:
            case WRPS1:
            case RDPS1:
            case IREQ2:
            case IREQ4:
            case IREQ5: break;

            // initiating a memory word read (alu has memory address)
            case FETCH1:
            case IRET1:
            case IRET3: {
                mrd = mwd = true;
                break;
            }

            // initiating a memory word write (alu has memory address)
            case IREQ1:
            case IREQ3: {
                mwr = mwd = true;
                break;
            }

            // read/write word/byte sized memory
            case STORE1: {
                mwr = true;
                mwd = (ir & 0x2000) == 0;
                break;
            }
            case LOAD1: {
                mrd = true;
                mwd = (ir & 0x2000) == 0;
                break;
            }

            // alu output undefined this cycle
            default: {
                aluv = false;
                break;
            }
        }

        if (mread  != mrd) throw new RuntimeException ("mread is " + mread + " at end of " + state);
        if (mwrite != mwr) throw new RuntimeException ("mwrite is " + mwrite + " at end of " + state);
        if ((mread || mwrite) && (mword != mwd)) throw new RuntimeException ("mword is " + mword + " at end of " + state);

        if (aluv && (md != alu)) {
            throw new RuntimeException ("alu is " + hexstr (md) + " should be " + hexstr (alu) + " at end of " + state);
        }
    }

    // clock to next state
    // call right after calling check()
    //  input:
    //   time = sim time (ns)
    //   mq  = memory data sent to CPU during last cycle
    //   irq = interrupt request line sent to CPU during last cycle
    public void clock (int time, short mq, boolean irq)
    {
        // at end of state ... do ... then go to state ...
        switch (state) {
            case RESET0: {
                state = State.RESET1;
                break;
            }
            case RESET1: {
                regs[7] = 0;
                loadPsw (0);
                state = State.FETCH1;
                break;
            }
            case FETCH1: {
                state = State.FETCH2;
                break;
            }
            case FETCH2: {
                regs[7] = alu;
                ir = mq;
                switch ((ir >> 13) & 7) {
                    case 0: {
                        if ((ir & 0b0001110000000001) != 0) {
                            state = State.BCC1; break;
                        } else {
                            switch ((ir & 0b0000000000001110) >> 1) {
                                case 0: state = State.HALT1; break;
                                case 1: state = State.IRET1; break;
                                case 2: state = State.WRPS1; break;
                                case 3: state = State.RDPS1; break;
                                default: throw new RuntimeException ("bad opcode " + hexstr (ir));
                            }
                        }
                        break;
                    }
                    case 1: state = State.ARITH1; break;
                    case 2:
                    case 3: state = State.STORE1; break;
                    case 4: state = State.LDA1; break;
                    case 5:
                    case 6:
                    case 7: state = State.LOAD1; break;
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
                int regd = (ir >> REGD) & 7;
                if (regd != 7) regs[regd] = alu;
                loadPsw (newpsw);
                state = endOfInst (irq);
                break;
            }
            case STORE1: {
                state = State.STORE2;
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
                state = State.LOAD2;
                break;
            }
            case LOAD2: {
                if ((ir & 0b0010000000000000) != 0) {
                    mq &= 0xFF;
                    if ((ir & 0b0100000000000000) != 0) {
                        mq = (short) ((mq ^ 0x80) - 0x80);
                    }
                }
                regs[(ir>>REGD)&7] = mq;
                if (((ir & 0x7F) == 0) && (((ir >> REGA) & 7) == 7) && (((ir >> REGD) & 7) != 7)) {
                    state = State.LOAD3;
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
                state = State.IRET2;
                break;
            }
            case IRET2: {
                regs[7] = mq;
                state = State.IRET3;
                break;
            }
            case IRET3: {
                state = State.IRET4;
                break;
            }
            case IRET4: {
                loadPsw (mq);
                state = State.FETCH1;
                break;
            }

            case HALT1: {
                state = State.FETCH1;
                break;
            }

            case IREQ1: {
                state = State.IREQ2;
                break;
            }
            case IREQ2: {
                state = State.IREQ3;
                break;
            }
            case IREQ3: {
                psw  &= 0x7FFF;
                state = State.IREQ4;
                break;
            }
            case IREQ4: {
                state = State.IREQ5;
                break;
            }
            case IREQ5: {
                regs[7] = alu;
                state = State.FETCH1;
                break;
            }
        }

        if (printstate) System.out.println (time + " STARTING STATE " + state);

        // at beginning of state ... do ...
        switch (state) {
            case FETCH1: {
                alu = regs[7];
                break;
            }
            case FETCH2: {
                alu = (short) (regs[7] + 2);
                break;
            }
            case BCC1: {
                alu = (short) (regs[7] + ((ir & 0x03FE) ^ 0x0200) - 0x0200);
                break;
            }
            case ARITH1: {
                int sa = regs[(ir>>REGA)&7];
                int sb = regs[(ir>>REGB)&7];
                int ua = sa & 0xFFFF;
                int ub = sb & 0xFFFF;
                boolean newc = (psw & 1) != 0;
                boolean newv = false;
                switch (ir & 15) {
                    case  0: {
                        alu = (short) (ua >> 1);
                        newc = (ua & 1) != 0;
                        break;
                    }
                    case  1: {
                        alu = (short) (sa >> 1);
                        newc = (ua & 1) != 0;
                        break;
                    }
                    case  2: {
                        alu = (short) ((ua >> 1) | (newc ? 0x8000 : 0));
                        newc = (ua & 1) != 0;
                        break;
                    }
                    case  4: {  // mov
                        alu = (short) ub;
                        break;
                    }
                    case  5: {  // neg
                        int ss = - sb;
                        alu = (short) ss;
                        newv = (ss < -32768) || (ss > 32767);
                        break;
                    }
                    case  6: {  // inc
                        int ss = sb + 1;
                        alu = (short) ss;
                        newv = (ss < -32768) || (ss > 32767);
                        break;
                    }
                    case  7: {  // com
                        alu = (short) ~ sb;
                        break;
                    }
                    case  8: {
                        alu = (short) (ua | ub);
                        break;
                    }
                    case  9: {
                        alu = (short) (ua & ub);
                        break;
                    }
                    case 10: {
                        alu = (short) (ua ^ ub);
                        break;
                    }
                    case 12: {  // ADD
                        newc = false;
                        // falllthrough
                    }
                    case 14: {  // ADC
                        int ss = sa + sb;
                        int us = ua + ub;
                        if (newc) {
                            ss ++;
                            us ++;
                        }
                        alu = (short) ss;
                        newc = us > 65535;
                        newv = (ss < -32768) || (ss > 32767);
                        break;
                    }
                    case 13: {  // SUB
                        newc = false;
                        // falllthrough
                    }
                    case 15: {  // SBB
                        int ss = sa - sb;
                        int us = ua - ub;
                        if (newc) {
                            -- ss;
                            -- us;
                        }
                        alu = (short) ss;
                        newc = us < 0;
                        newv = (ss < -32768) || (ss > 32767);
                        break;
                    }
                    default: throw new RuntimeException ("bad opcode " + hexstr (ir));
                }
                newpsw = (short) ((psw & 0x8000) |
                        ( (alu < 0) ? 8 : 0) |   // n
                        ((alu == 0) ? 4 : 0) |   // z
                        (      newv ? 2 : 0) |   // v
                        (      newc ? 1 : 0));   // c
                break;
            }

            case STORE1: {
                alu = (short) (regs[(ir>>REGA)&7] + ((ir & 0x7F) ^ 0x40) - 0x40);
                break;
            }
            case STORE2: {
                alu = regs[(ir>>REGD)&7];
                break;
            }
            case LDA1: {
                int num = (ir >> REGA) & 7;
                int ofs = ((ir & 0x7F) ^ 0x40) - 0x40;
                alu = (short) (regs[num] + ofs);
                break;
            }
            case LOAD1: {
                alu = (short) (regs[(ir>>REGA)&7] + ((ir & 0x7F) ^ 0x40) - 0x40);
                break;
            }
            case LOAD3: {
                alu = (short) (regs[7] + 2);
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
                alu = (short) 0xFFFC;
                break;
            }
            case IRET3: {
                alu = (short) 0xFFFE;
                break;
            }
            case IREQ1: {
                alu = (short) 0xFFFE;
                break;
            }
            case IREQ2: {
                alu = psw;
                break;
            }
            case IREQ3: {
                alu = (short) 0xFFFC;
                break;
            }
            case IREQ4: {
                alu = regs[7];
                break;
            }
            case IREQ5: {
                alu = (short) 0x0002;
                break;
            }
        }
    }

    // end of instruction's last cycle, transition to either fetch or start interrupt
    private State endOfInst (boolean irq)
    {
        return (irq && (psw < 0)) ? State.IREQ1 : State.FETCH1;
    }

    // return that the branch condition in IR vs condition codes in PSW is true
    private boolean branchTrue ()
    {
        boolean N = (psw & 8) != 0;
        boolean Z = (psw & 4) != 0;
        boolean V = (psw & 2) != 0;
        boolean C = (psw & 1) != 0;
        boolean bt = false;
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
    private void loadPsw (int newpsw)
    {
        psw = (short) (newpsw | 0x7FF0);
    }

    private static String hexstr (short val)
    {
        String hex = Integer.toHexString (val & 0xFFFF).toUpperCase ();
        while (hex.length () < 4) hex = "0" + hex;
        return hex;
    }
}
