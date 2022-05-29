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
#ifndef _GPIOLIB_H
#define _GPIOLIB_H

#include <signal.h>
#include <stdio.h>
#include <string>

#include <iowkit.h>

#include "miscdefs.h"

struct Shadow;

#define DEFCPUHZ 472000

#define G_CLK         0x4   // GPIO[02]   out: send clock signal to cpu
#define G_RESET       0x8   // GPIO[03]   out: send reset signal to cpu
#define G_DATA    0xFFFF0   // GPIO[19:04] bi:  data bus bits
#define G_IRQ    0x100000   // GPIO[20]   out: send interrupt request to cpu
#define G__QENA  0x200000   // GPIO[21]   out: we are sending data out to cpu
                            //                 voltage must be high on raspi pin to enable G_DATA->MQ
                            //                 since we go through an inverter, this is considered active low
#define G_DENA   0x400000   // GPIO[22]   out: we are receiving data from cpu
                            //                 voltage must be low on raspi pin to enable MD->G_DATA
                            //                 since we go through an inverter, this is considered active high
#define G_WORD   0x800000   // GPIO[23]    in: 1 = cpu wants to transact a word; 0 = byte
#define G_HALT  0x1000000   // GPIO[24]    in: HALT instruction (in state HALT.1)
#define G_READ  0x2000000   // GPIO[25]    in: cpu wants to read from us
#define G_WRITE 0x4000000   // GPIO[26]    in: cpu wants to write to us
#define G_FLAG  0x8000000   // GPIO[27]   out: use for scope sync
#define G_OUTS (G_CLK|G_RESET|G_IRQ|G__QENA|G_DENA|G_FLAG)  // these are always output pins
#define G_INS (G_READ|G_WRITE|G_WORD|G_HALT)                // these are always input pins
#define G_DATA0 (G_DATA & -G_DATA)                          // low-order data pin

// control pins on the C connector as returned by readcon(CON_C)
#define C_IRQ         (1U <<  0)
#define C_PSW_IE      (1U <<  1)
#define C_PSW_BT      (1U <<  2)
#define C_RESET       (1U <<  3)
#define C_CLK2        (1U <<  4)
#define C_MEM_REB     (1U <<  6)
#define C_PSW_ALUWE   (1U <<  7)
#define C_ALU_IRFC    (1U <<  8)
#define C_GPRS_REB    (1U <<  9)
#define C_BMUX_IRBROF (1U << 10)
#define C_GPRS_WED    (1U << 11)
#define C_MEM_READ    (1U << 12)
#define C_PSW_WE      (1U << 13)
#define C_HALT        (1U << 14)
#define C_GPRS_REA    (1U << 15)
#define C_GPRS_RED2B  (1U << 17)
#define C_ALU_BONLY   (1U << 18)
#define C_BMUX_IRLSOF (1U << 19)
#define C__PSW_REB    (1U << 20)
#define C_BMUX_CON__4 (1U << 21)
#define C_MEM_WORD    (1U << 22)
#define C_ALU_ADD     (1U << 23)
#define C_GPRS_WE7    (1U << 24)
#define C_GPRS_REA7   (1U << 25)
#define C_BMUX_CON_2  (1U << 26)
#define C__IR_WE      (1U << 27)
#define C_BMUX_CON_0  (1U << 29)
#define C_MEM_WRITE   (1U << 30)
#define C__PSW_IECLR  (1U << 31)

#define C_FLIP (C__IR_WE | C__PSW_IECLR | C__PSW_REB)
#define C_RAS  (C_CLK2 | C_IRQ | C_RESET)
#define C_SEQ  ( \
    C_ALU_ADD | C_ALU_BONLY | C_ALU_IRFC | \
    C_BMUX_CON_0 | C_BMUX_CON_2 | C_BMUX_CON__4 | C_BMUX_IRBROF | C_BMUX_IRLSOF | \
    C_GPRS_REA | C_GPRS_REA7 | C_GPRS_REB | C_GPRS_RED2B | C_GPRS_WE7 | C_GPRS_WED | \
    C_HALT | C__IR_WE | \
    C_MEM_READ | C_MEM_REB | C_MEM_WORD | C_MEM_WRITE | \
    C_PSW_ALUWE | C_PSW_BT | C_PSW_IE | C__PSW_IECLR | C__PSW_REB | C_PSW_WE)

// extended pins on the D connector as returned by readcon(CON_D)
#define D_DBUS       0xFFFF         // data bits (scrambled)
#define D_RAWCIN     (1U << 16)     // carry into alu[00]
#define D_RAWCMIDI   (1U << 17)     // carry into alu[08]
#define D_RAWCMIDO   (1U << 18)     // carry outof alu[07]
#define D_RAWCOUT    (1U << 19)     // carry outof alu[15]
#define D_RAWVOUT    (1U << 20)     // carry outof alu[14]
#define D_RAWSHRMIDI (1U << 21)     // shift right into alu[07]
#define D_RAWSHRIN   (1U << 22)     // shift right into alu[15]
#define D_RAWSHROUT  (1U << 24)     // shift right outof alu[00]
#define D_RAWSHRMIDO (1U << 25)     // shift right outof alu[08]

// A,C,I,D connectors
#define CONLETS "ACIDG"
enum IOW56Con {
    CON_A = 0,
    CON_C = 1,
    CON_I = 2,
    CON_D = 3,
    CON_G = 4
};

struct GpioLib {
    virtual void open () = 0;
    virtual void close () = 0;
    virtual void halfcycle () = 0;
    virtual uint32_t readgpio () = 0;
    virtual void writegpio (bool wdata, uint32_t valu) = 0;

    // read the 32 data pins of one of the A,C,I,D connectors
    // omits the 8 ground pins
    //  input:
    //   c = CON_A,C,I,D
    //  output:
    //                      A           C          I        D
    //    <00> = pin  1  abus[00]  irq           ir[00]  dbus[15]
    //    <01> = pin  2  bbus[00]  psw_ie       _ir[00]  dbus[07]
    //    <02> = pin  3  abus[08]  psw_bt        ir[01]  dbus[14]
    //    <03> = pin  4  bbus[08]  reset        _ir[01]  dbus[06]
    //    <04> = pin  6  abus[01]  clk2          ir[02]  dbus[13]
    //    <05> = pin  7  bbus[01]  --           _ir[02]  dbus[05]
    //    <06> = pin  8  abus[09]  mem_reb       ir[03]  dbus[12]
    //    <07> = pin 10  bbus[09]  psw_aluwe    _ir[03]  dbus[04]
    //    <08> = pin 11  abus[02]  alu_irfc      ir[04]  dbus[11]
    //    <09> = pin 12  bbus[02]  gprs_reb     _ir[04]  dbus[03]
    //    <10> = pin 14  abus[10]  bmux_irbrof   ir[05]  dbus[10]
    //    <11> = pin 15  bbus[10]  gprs_wed     _ir[05]  dbus[02]
    //    <12> = pin 16  abus[03]  mem_read      ir[06]  dbus[09]
    //    <13> = pin 18  bbus[03]  psw_we       _ir[06]  dbus[01]
    //    <14> = pin 19  abus[11]  halt          ir[07]  dbus[08]
    //    <15> = pin 20  bbus[11]  gprs_rea     _ir[07]  dbus[00]
    //    <16> = pin 21  abus[04]  --            ir[08]  rawcin
    //    <17> = pin 22  bbus[04]  gprs_red2b   _ir[08]  rawcmid
    //    <18> = pin 24  abus[12]  alu_bonly     ir[09]  rawcmid
    //    <19> = pin 25  bbus[12]  bmux_irlsof  _ir[09]  rawcout
    //    <20> = pin 26  abus[05]  _psw_re       ir[10]  rawvout
    //    <21> = pin 28  bbus[05]  bmux_con__4  _ir[10]  rawshrmid
    //    <22> = pin 29  abus[13]  mem_word      ir[11]  rawshrin
    //    <23> = pin 30  bbus[13]  alu_add      _ir[11]  --
    //    <24> = pin 32  abus[06]  gprs_we7      ir[12]  rawshrout
    //    <25> = pin 33  bbus[06]  gprs_rea7    _ir[12]  rawshrmid
    //    <26> = pin 34  abus[14]  bmux_con_2    ir[13]  --
    //    <27> = pin 36  bbus[14]  _ir_we       _ir[13]  --
    //    <28> = pin 37  abus[07]  --            ir[14]  --
    //    <29> = pin 38  bbus[07]  bmux_con_0   _ir[14]  --
    //    <30> = pin 39  abus[15]  mem_write     ir[15]  --
    //    <31> = pin 40  bbus[15]  _psw_ieclr   _ir[15]  --
    virtual bool readcon (IOW56Con c, uint32_t *pins) = 0;

    virtual bool writecon (IOW56Con c, uint32_t mask, uint32_t pins) = 0;

    static uint32_t  abusshuf (uint16_t abus);
    static uint16_t  abussort (uint32_t acon);
    static uint32_t  bbusshuf (uint16_t bbus);
    static uint16_t  bbussort (uint32_t acon);
    static uint32_t  dbusshuf (uint16_t dbus);
    static uint16_t  dbussort (uint32_t dcon);
    static uint32_t  ibusshuf (uint16_t ibus);
    static uint16_t  ibussort (uint32_t icon);
    static uint16_t _ibussort (uint32_t icon);

    static std::string decocon (IOW56Con c, uint32_t bits);
};

struct NohwLib : GpioLib {
    NohwLib (Shadow *shadow);
    void open ();
    void close ();
    void halfcycle ();
    uint32_t readgpio ();
    void writegpio (bool wdata, uint32_t valu);
    bool readcon (IOW56Con c, uint32_t *pins);
    bool writecon (IOW56Con c, uint32_t mask, uint32_t pins);

private:
    Shadow *shadow;
    uint32_t gpiowritten;
};

struct PhysLib : GpioLib {
    PhysLib (uint32_t cpuhz);
    PhysLib (uint32_t cpuhz, bool nopads);
    void open ();
    void close ();
    void halfcycle ();
    uint32_t readgpio ();
    void writegpio (bool wdata, uint32_t valu);
    bool readcon (IOW56Con c, uint32_t *pins);
    bool writecon (IOW56Con c, uint32_t mask, uint32_t pins);

    void makeinputpins ();
    void makeoutputpins ();
    void clrpins (uint32_t mask);
    void setpins (uint32_t mask);
    uint32_t getpins ();

private:
    bool is2711;
    bool nopads;
    int memfd;
    IOWKIT_HANDLE iowhandles[4];
    sigset_t allowedsigs;
    struct timespec hafcycts;
    uint32_t cpuhz;
    uint32_t gpiomask;
    uint32_t gpiovalu;
    uint32_t volatile *gpiopage;
    uint32_t hafcychi, hafcyclo;
    uint32_t lastretries;
    uint32_t vals[4];
    uint32_t writecount;
    uint64_t writecycles;
    void *memptr;

    void ctor (uint32_t cpuhz, bool nopads);
    uint32_t readconblocked (IOW56Con c, IOWKIT_HANDLE iowh);
    void blocksigint ();
    void allowsigint ();
};

struct PipeLib : GpioLib {
    PipeLib (char const *sendname);
    void open ();
    void close ();
    void halfcycle ();
    uint32_t readgpio ();
    void writegpio (bool wdata, uint32_t valu);
    bool readcon (IOW56Con c, uint32_t *pins);
    bool writecon (IOW56Con c, uint32_t mask, uint32_t pins);

private:
    bool gota, gotc, goti, gotd;
    char const *sendname;
    FILE *recvfile;
    FILE *sendfile;
    uint32_t sendseq;
    uint32_t vala, valc, vali, vald;

    uint32_t examine (char const *varname);
};
#endif
