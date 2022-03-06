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

#include <stdio.h>
#include <stdlib.h>

#include "disassemble.h"

// disassemble the given opcode
std::string disassemble (uint16_t opc)
{
    std::string str;

    switch ((opc >> 13) & 7) {
        case 0: {
            if (opc & 0x1C01) {
                switch (((opc >> 9) & 0xE) | (opc & 1)) {
                    case  1: str.append ("BR   ."); break;
                    case  2: str.append ("BEQ  ."); break;
                    case  3: str.append ("BNE  ."); break;
                    case  4: str.append ("BLT  ."); break;
                    case  5: str.append ("BGE  ."); break;
                    case  6: str.append ("BLE  ."); break;
                    case  7: str.append ("BGT  ."); break;
                    case  8: str.append ("BLO  ."); break;
                    case  9: str.append ("BHIS ."); break;
                    case 10: str.append ("BLOS ."); break;
                    case 11: str.append ("BHI  ."); break;
                    case 12: str.append ("BMI  ."); break;
                    case 13: str.append ("BPL  ."); break;
                    case 14: str.append ("BVS  ."); break;
                    case 15: str.append ("BVC  ."); break;
                    default: abort ();
                }
                int dispint = ((opc & 0x03FE) ^ 0x0200) - 0x0200 + 2;
                if (dispint != 0) {
                    if (dispint < 0) {
                        str.append (1, '-');
                        dispint = - dispint;
                    } else {
                        str.append (1, '+');
                    }
                    char dispstr[12];
                    sprintf (dispstr, "%d", dispint);
                    str.append (dispstr);
                }
            } else {
                switch ((opc >> 1) & 3) {
                    case 0: str.append ("HALT R"); str.append (1, '0' + ((opc >> REGB) & 7)); break;
                    case 1: str.append ("IRET"); break;
                    case 2: str.append ("WRPS R"); str.append (1, '0' + ((opc >> REGB) & 7)); break;
                    case 3: str.append ("RDPS R"); str.append (1, '0' + ((opc >> REGD) & 7)); break;
                    default: abort ();
                }
            }
            break;
        }
        case 1: {
            switch (opc & 15) {
                case  0: str.append ("LSR  R"); break;
                case  1: str.append ("ASR  R"); break;
                case  2: str.append ("ROR  R"); break;
                case  3: str.append ("A03  R"); break;
                case  4: str.append ("MOV  R"); break;
                case  5: str.append ("NEG  R"); break;
                case  6: str.append ("INC  R"); break;
                case  7: str.append ("COM  R"); break;
                case  8: str.append ("OR   R"); break;
                case  9: str.append ("AND  R"); break;
                case 10: str.append ("XOR  R"); break;
                case 11: str.append ("A11  R"); break;
                case 12: str.append ("ADD  R"); break;
                case 13: str.append ("SUB  R"); break;
                case 14: str.append ("ADC  R"); break;
                case 15: str.append ("SBB  R"); break;
            }
            str.append (1, '0' + ((opc >> REGD) & 7));
            if ((opc & 12) != 4) {
                str.append (",R");  // RA applies to LSR...,OR....
                str.append (1, '0' + ((opc >> REGA) & 7));
            }
            if ((opc & 12) != 0) {
                str.append (",R");  // RB applies to MOV...,OR...
                str.append (1, '0' + ((opc >> REGB) & 7));
            }
            break;
        }
        default: {
            switch ((opc >> 13) & 7) {
                case 2: str.append ("STW  R"); break;
                case 3: str.append ("STB  R"); break;
                case 4: str.append ("LDA  R"); break;
                case 5: str.append ("LDBU R"); break;
                case 6: str.append ("LDW  R"); break;
                case 7: str.append ("LDBS R"); break;
                default: abort ();
            }
            str.append (1, '0' + ((opc >> REGD) & 7));
            str.append (",");
            int dispint = ((opc & 0x7F) ^ 0x40) - 0x40;
            char dispstr[12];
            sprintf (dispstr, "%d", dispint);
            str.append (dispstr);
            str.append ("(R");
            str.append (1, '0' + ((opc >> REGA) & 7));
            str.append (1, ')');
            break;
        }
    }

    return str;
}
