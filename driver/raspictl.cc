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
 * Program runs on the raspberry pi to control the computer.
 *
 *  ../asm/assemble.armv7l r6loop.asm r6loop.hex [cmdargs ...] > r6loop.lis
 *  . ./iow56sns.si
 *  sudo -E gdb --args ./raspictl [-chkacid] [-cpuhz <freq>] [-haltstop] [-mintimes] [-nohw] [-oddok] [-printstate] [-sim <pipename>] -randmem | r6loop.hex
 *      -chkacid    : check A,C,I,D connectors at end of each cycle (requires paddles)
 *      -cpuhz      : specify cpu frequency (default 470000Hz)
 *      -haltstop   : HALT instruction causes exit (else it is 'wait for interrupt')
 *      -mintimes   : print cpu cycle info once a minute
 *      -nohw       : don't use hardware, simulate processor internally
 *      -oddok      : odd addresses ok (swaps bytes) (else give warning message)
 *      -printinstr : print message at beginning of each instruction
 *      -printstate : print message at beginning of each state
 *      -randmem    : supply random opcodes and data for testing
 *      -sim        : simulate via pipe connected to NetGen
 *      -stopat     : stop simulating when accessing the address
 */

#include <errno.h>
#include <fcntl.h>
#include <map>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "shadow.h"

#define SHADOWCHECK(samp) shadowcheck (samp)

#define MAGIC 0xFFF0
#define ERRNO 0xFFF2

#define SCN_EXIT 0
#define SCN_PRINTLN 1
#define SCN_INTREQ 2
#define SCN_CLOSE 3
#define SCN_GETNOWNS 4
#define SCN_OPEN 5
#define SCN_READ 6
#define SCN_WRITE 7
#define SCN_IRQATNS 8
#define SCN_INTACK 9
#define SCN_UNLINK 10
#define SCN_GETCMDARG 11
#define SCN_ADD_DDD 12
#define SCN_ADD_FFF 13
#define SCN_SUB_DDD 14
#define SCN_SUB_FFF 15
#define SCN_MUL_DDD 16
#define SCN_MUL_FFF 17
#define SCN_DIV_DDD 18
#define SCN_DIV_FFF 19
#define SCN_CMP_DD 20
#define SCN_CMP_FF 21
#define SCN_CVT_FP 22
#define SCN_UDIV 23
#define SCN_UMUL 24
#define SCN_SETROSIZE 25
#define SCN_SETSTKLIM 26
#define SCN_PRINTINSTR 27
#define SCN_NEG_DD 28
#define SCN_NEG_FF 29
#define SCN_WATCHWRITE 30
#define SCN_GETENV 31
#define SCN_SETTTYECHO 32

#define IRQ_SCNINTREQ 0x1
#define IRQ_LINECLOCK 0x2
#define IRQ_RANDMEM   0x4

#define ever (;;)

typedef uint16_t MagicReader (uint32_t sample);
typedef void MagicWriter (uint32_t sample, uint16_t data);

static bool lineclockrun;
static char **cmdargv;
static GpioLib *gpio;
static int cmdargc;
static pthread_cond_t intreqcond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t lineclockcond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t intreqlock = PTHREAD_MUTEX_INITIALIZER;
static Shadow shadow;
static std::map<int,struct termios> savedtermioss;
static struct timespec lineclockabs;
static uint16_t readonlysize;
static uint16_t stacklimit;
static uint32_t intreqreg;
static uint32_t syncintreq;
static uint32_t watchwrite;
static uint8_t memory[0x10000];
static uint32_t readcounts[0x8000];

static uint16_t mr_syscall (uint32_t sample);
static void mw_syscall (uint32_t sample, uint16_t data);

static MagicReader *const magicreaders[] = { mr_syscall };
static MagicWriter *const magicwriters[] = { mw_syscall };

static void save_errno ();
static void *mintimesthread (void *dummy);
static void *lineclockthread (void *dummy);
static int twohexchars (char const *str);
static uint16_t randopcode ();
static uint16_t randuint16 ();
static void dumpregs ();
static void senddata (uint16_t data);
static uint16_t recvdata (void);
static uint16_t readmemword (uint32_t addr);
static uint32_t readmemlong (uint32_t addr);
static uint64_t readmemquad (uint32_t addr);
static double   readmemdoub (uint32_t addr);
static float    readmemflt  (uint32_t addr);
static void writememlong (uint32_t addr, uint32_t val);
static void writememquad (uint32_t addr, uint64_t val);
static void writememdoub (uint32_t addr, double   val);
static void writememflt  (uint32_t addr, float    val);
static void *getmemptr (uint32_t addr, uint32_t size, bool write);
static char const *getmemstr (uint32_t addr);
static void shadowcheck (uint32_t sample);
static void restorestdinattrs ();

int main (int argc, char **argv)
{
    bool haltstop = false;
    bool mintimes = false;
    bool nohw = false;
    bool oddok = false;
    bool randmem = false;
    char const *loadname = NULL;
    char const *simname = NULL;
    char *p;
    int randinit = 0;
    uint16_t lastmemread = 0;
    uint32_t cpuhz = DEFCPUHZ;
    uint32_t stopataddr = -1;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-chkacid") == 0) {
            shadow.chkacid = true;
            continue;
        }
        if (strcasecmp (argv[i], "-cpuhz") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing freq after -cpuhz\n");
                return 1;
            }
            cpuhz = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-haltstop") == 0) {
            haltstop = true;
            continue;
        }
        if (strcasecmp (argv[i], "-mintimes") == 0) {
            mintimes = true;
            continue;
        }
        if (strcasecmp (argv[i], "-nohw") == 0) {
            nohw = true;
            continue;
        }
        if (strcasecmp (argv[i], "-oddok") == 0) {
            oddok = true;
            continue;
        }
        if (strcasecmp (argv[i], "-printinstr") == 0) {
            shadow.printinstr = true;
            continue;
        }
        if (strcasecmp (argv[i], "-printstate") == 0) {
            shadow.printstate = true;
            continue;
        }
        if ((strcasecmp (argv[i], "-randmem") == 0) && (loadname == NULL)) {
            oddok = true;
            randmem = true;
            randinit = 14;
            continue;
        }
        if (strcasecmp (argv[i], "-sim") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing pipe name after -sim\n");
                return 1;
            }
            simname = argv[i];
            continue;
        }
        if (strcasecmp (argv[i], "-stopat") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing address after -stopat\n");
                return 1;
            }
            stopataddr = strtoul (argv[i], &p, 0);
            if ((*p != 0) || (stopataddr > 0xFFFF)) {
                fprintf (stderr, "raspictl: bad -stopat address '%s'\n", argv[i]);
                return 1;
            }
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "raspictl: unknown option %s\n", argv[i]);
            return 1;
        }
        if ((loadname != NULL) || randmem) {
            fprintf (stderr, "raspictl: unknown argument %s\n", argv[i]);
            return 1;
        }

        // hex filename, remainder of command line args available to program
        loadname = argv[i];
        cmdargc  = argc - i;
        cmdargv  = argv + i;
        break;
    }
    if ((loadname == NULL) && ! randmem) {
        fprintf (stderr, "raspictl: missing loadfile parameter\n");
        return 1;
    }

    // read loadfile contents into memory
    if (loadname != NULL) {
        FILE *loadfile = fopen (loadname, "r");
        if (loadfile == NULL) {
            fprintf (stderr, "raspictl: error opening loadfile %s: %m\n", loadname);
            return 1;
        }
        char loadline[144], *p;
        while (fgets (loadline, sizeof loadline, loadfile) != NULL) {
            uint32_t addr = strtoul (loadline, &p, 16);
            if (*(p ++) != ':') {
                fprintf (stderr, "raspictl: bad loadfile line %s", loadline);
                return 1;
            }
            while (*p != '\n') {
                int data = twohexchars (p);
                if ((data < 0) || (addr >= sizeof memory)) {
                    fprintf (stderr, "raspictl: bad loadfile line %s", loadline);
                    return 1;
                }
                memory[addr++] = data;
                p += 2;
            }
        }
        fclose (loadfile);
    }

    if (mintimes) {
        pthread_t pid;
        int rc = pthread_create (&pid, NULL, mintimesthread, NULL);
        if (rc != 0) abort ();
        pthread_detach (pid);
    }

    // access cpu circuitry
    // either physical circuit via gpio pins
    // ...or netgen simulator via pipes
    // ...or nothing but shadow
    gpio = (simname != NULL) ? (GpioLib *) new PipeLib (simname) :
                            (nohw ? (GpioLib *) new NohwLib (&shadow) :
                                        (GpioLib *) new PhysLib (cpuhz, ~ shadow.chkacid));
    gpio->open ();

    // reset CPU circuit for a couple cycles
    gpio->writegpio (false, G_RESET);
    gpio->halfcycle ();
    gpio->halfcycle ();
    gpio->halfcycle ();

    // tell shadowing that cpu is in reset0 state
    shadow.open (gpio);
    shadow.reset ();

    // drop reset and leave clock signal low
    // enable reading data from cpu
    gpio->writegpio (false, 0);
    gpio->halfcycle ();

    for ever {

        // invariant:
        //  clock has been low for half cycle
        //  G_DENA has been asserted for half cycle so we can read md coming from cpu

        // get input signals just before raising clock
        uint32_t sample = gpio->readgpio ();

        // maybe processor executed an HALT instruction
        if (sample & G_HALT) {

            // -haltstop means just print a message and exit
            if (haltstop) {
                fprintf (stderr, "raspictl: PC=%04X  HALT %04X\n", lastmemread, (sample & G_DATA) / G_DATA0);
                return 0;
            }

            // otherwise wait for an interrupt
            if (! randmem) {
                pthread_mutex_lock (&intreqlock);
                while (intreqreg == 0) {
                    pthread_cond_wait (&intreqcond, &intreqlock);
                }
                pthread_mutex_unlock (&intreqlock);
            }
        }

        // raise clock then wait for half a cycle
        SHADOWCHECK (sample);
        if (shadow.state == Shadow::FETCH2) {
            // keep IR LEDs steady with old opcode instead of turning all on
            // cosmetic only, otherwise can use the else's writegpio()
            gpio->writegpio (true, G_CLK | syncintreq | shadow.ir * G_DATA0);
        } else {
            gpio->writegpio (false, G_CLK | syncintreq);
        }
        gpio->halfcycle ();

        // process the signal sample from just before raising clock
        if (sample & (G_READ | G_WRITE)) {
            if (((sample & (G_WORD | G_DATA0)) == (G_WORD | G_DATA0)) && ! oddok) {
                fprintf (stderr, "raspictl: odd word access %s\n", GpioLib::decocon (CON_G, sample).c_str ());
                dumpregs ();
                abort ();
            }

            uint16_t addr = sample / G_DATA0;
            uint32_t mi = addr / 2 - MAGIC / 2;

            if (addr == stopataddr) {
                fprintf (stderr, "raspictl: stopat %04X; PC %04X\n", addr, shadow.regs[7]);
                return 2;
            }

            if (sample & G_READ) {
                readcounts[addr/2] ++;
                lastmemread = addr;
                uint16_t data;
                if (randmem) {
                    if ((randinit > 0) && (-- randinit & 1)) {
                        data = 0xC000 | ((randinit - 1) << (REGD - 1)) | (7 << REGA);
                    } else if (shadow.state == Shadow::FETCH2) {
                        data = randopcode ();
                    } else {
                        data = randuint16 ();
                    }
                } else if (mi < sizeof magicreaders / sizeof magicreaders[0]) {
                    MagicReader *mr = magicreaders[mi];
                    data = (*mr) (sample);
                } else {
                    data = memory[addr];
                    if (sample & G_WORD) {
                        data |= ((uint16_t) memory[addr^1]) << 8;
                    }
                }
                senddata (data);
            }

            if (sample & G_WRITE) {
                uint16_t data = recvdata ();
                if (randmem) {
                    intreqreg = (randuint16 () & 1) * IRQ_RANDMEM;
                } else {
                    if (mi < sizeof magicwriters / sizeof magicwriters[0]) {
                        MagicWriter *mw = magicwriters[mi];
                        (*mw) (sample, data);
                    } else {
                        if (((addr ^ watchwrite) & -2) == 0) {
                            fprintf (stderr, "raspictl: watch write addr %04X data %04X\n", addr, data);
                            dumpregs ();
                        }
                        if (addr < readonlysize) {
                            fprintf (stderr, "raspictl: write addr %04X below rosize %04X\n", addr, readonlysize);
                            dumpregs ();
                            abort ();
                        }
                        memory[addr] = data;
                        if (sample & G_WORD) {
                            memory[addr^1] = data >> 8;
                        }
                    }
                }
            }
        }

        if (shadow.regs[6] < stacklimit) {
            fprintf (stderr, "raspictl: stack pointer %04X below limit %04X\n", shadow.regs[6], stacklimit);
            dumpregs ();
            abort ();
        }

        // update irq pin
        syncintreq = intreqreg ? G_IRQ : 0;

        // drop the clock and enable data bus read
        // then wait half a clock cycle while clock is low
        gpio->writegpio (false, syncintreq);
        gpio->halfcycle ();
    }
}

static int twohexchars (char const *str)
{
    char buf[3], *p;
    buf[0] = str[0];
    buf[1] = str[1];
    buf[2] = 0;
    uint8_t data = strtoul (buf, &p, 16);
    if (p != &buf[2]) return -1;
    return data;
}

// assuming we are in FETCH2, generate random opcode such that all possible next states have equal probability
// do not generate undefined opcode so shadow won't puque
static uint16_t randopcode ()
{
    switch (randuint16 () % 9) {
        case 0: {  // BCC.1
            uint16_t cond = randuint16 () % 15 + 1;
            uint16_t offs = randuint16 () & 0x3FE;
            return 0x0000 | ((cond & 14) << 9) | offs | (cond & 1);
        }
        case 1: {  // ARITH.1
            uint16_t aop = randuint16 () % 14;
            uint16_t adb = randuint16 () & 0x1FF;
            if (aop >=  3) aop ++;
            if (aop >= 11) aop ++;
            return 0x2000 | (adb << 4) | aop;
        }
        case 2: {  // STORE.1
            return 0x4000 | (randuint16 () & 0x3FFF);
        }
        case 3: {  // LDA.1
            return 0x8000 | (randuint16 () & 0x1FFF);
        }
        case 4: {  // LOAD.1
            uint16_t r = randuint16 ();
            bool imm = (r & 7) == 0;
            uint16_t loadop = r / 8 % 3 + 5;
            uint16_t rardofs = imm ? (((randuint16 () & 7) << REGD) | (7 << REGA)) : (randuint16 () & 0x1FFF);
            return (loadop << 13) | rardofs;
        }
        case 5: {   // HALT.1
            return 0x0000 | ((randuint16 () & 7) << REGB);
        }
        case 6: {   // IRET.1
            return 0x0002;
        }
        case 7: {   // WRPS.1
            return 0x0004 | ((randuint16 () & 7) << REGB);
        }
        case 8: {   // RDPS.1
            return 0x0006 | ((randuint16 () & 7) << REGD);
        }
        default: abort ();
    }
}

static uint16_t randuint16 ()
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
    uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
    seed = (seed << 1) | (xnor & 1);

    return (uint16_t) seed;
}

static void dumpregs ()
{
    fprintf (stderr, "raspictl:  R0=%04X R1=%04X R2=%04X R3=%04X R4=%04X R5=%04X R6=%04X PC=%04X\n",
            shadow.regs[0], shadow.regs[1], shadow.regs[2], shadow.regs[3],
            shadow.regs[4], shadow.regs[5], shadow.regs[6], shadow.regs[7]);
}

// CPU wrote to syscall magic location
// assume it is a pointer to syscall parameter block
static uint16_t mr_syscall_rc;

static void mw_syscall (uint32_t sample, uint16_t data)
{
    if (data & 1) {
        mr_syscall_rc = -1;
        errno = -ENOSYS;
        save_errno ();
        fprintf (stderr, "raspictl: odd syscall addr %04X\n", data);
        return;
    }
    uint16_t scn = readmemword (data);
    switch (scn) {

        // exit() system call
        case SCN_EXIT: {
            uint16_t code = readmemword (data + 2);
            fprintf (stderr, "raspictl: SCN_EXIT:%u; %llu cycles\n", code, shadow.getcycles ());
            exit (code);
            break;
        }

        // print string with a newline
        case SCN_PRINTLN: {
            uint16_t addr = readmemword (data + 2);
            char const *str = getmemstr (addr);
            fprintf (stderr, "raspictl: SCN_PRINTLN:%s\n", str);
            break;
        }

        // force interrupt request
        //  .word   SCN_INTREQ
        //  .word   0: negate; 1: assert
        case SCN_INTREQ: {
            uint16_t code = readmemword (data + 2);
            fprintf (stderr, "raspictl: SCN_INTREQ:%s\n", ((code != 0) ? "true" : "false"));
            pthread_mutex_lock (&intreqlock);
            if (code != 0) intreqreg |= IRQ_SCNINTREQ;
                    else intreqreg &= ~ IRQ_SCNINTREQ;
            pthread_mutex_unlock (&intreqlock);
            break;
        }

        // close() system call
        case SCN_CLOSE: {
            int fd = (int)(sint16_t) readmemword (data + 2);
            if (fd <= 2) fd = -999;
            int rc = close (fd);
            if (rc < 0) save_errno ();
            mr_syscall_rc = rc;
            savedtermioss.erase (fd);
            break;
        }

        // get time via clock_gettime(CLOCK_REALTIME), ie, unix timestamp with ns precision
        //  .word   SCN_GETNOWNS
        //  .word   addr of uint64_t that gets the current timestamp
        case SCN_GETNOWNS: {
            struct timespec nowts;
            if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) abort ();
            writememquad (readmemword (data + 2), nowts.tv_sec * 1000000000ULL + nowts.tv_nsec);
            mr_syscall_rc = 0;
            break;
        }

        // open() system call
        case SCN_OPEN: {
            char const *path = getmemstr (readmemword (data + 2));
            uint16_t flags = readmemword (data + 4);
            uint16_t mode = readmemword (data + 6);
            int rc = open (path, flags, mode);
            if (rc < 0) save_errno ();
            mr_syscall_rc = rc;
            break;
        }

        // read() system call
        case SCN_READ: {
            int fd = (int)(sint16_t) readmemword (data + 2);
            uint16_t len = readmemword (data + 6);
            uint16_t adr = readmemword (data + 4);
            void *buf = (adr == 0) ? NULL : getmemptr (adr, len, true);
            int rc = read (fd, buf, len);
            if (rc < 0) save_errno ();
            mr_syscall_rc = rc;
            break;
        }

        // write() system call
        case SCN_WRITE: {
            int fd = (int)(sint16_t) readmemword (data + 2);
            uint16_t len = readmemword (data + 6);
            uint16_t adr = readmemword (data + 4);
            void *buf = (adr == 0) ? NULL : getmemptr (adr, len, false);
            int rc = write (fd, buf, len);
            if (rc < 0) save_errno ();
            mr_syscall_rc = rc;
            break;
        }

        // .word SCN_IRQATNS
        // .word addr of uint64_t when to interrupt or 0 to stop
        case SCN_IRQATNS: {
            uint16_t irqataddr = readmemword (data + 2);
            uint64_t irqatns = (irqataddr != 0) ? readmemquad (irqataddr) : 0;
            pthread_mutex_lock (&intreqlock);
            intreqreg &= ~ IRQ_LINECLOCK;
            if (irqatns != 0) {
                lineclockabs.tv_sec  = irqatns / 1000000000;
                lineclockabs.tv_nsec = irqatns % 1000000000;
                if (! lineclockrun) {
                    rdcycuninit ();
                    pthread_t pid;
                    int rc = pthread_create (&pid, NULL, lineclockthread, NULL);
                    if (rc != 0) abort ();
                    pthread_detach (pid);
                    lineclockrun = true;
                    rdcycinit ();
                }
            } else {
                memset (&lineclockabs, 0, sizeof lineclockabs);
            }
            pthread_cond_broadcast (&lineclockcond);
            pthread_mutex_unlock (&intreqlock);
            mr_syscall_rc = 0;
            break;
        }

        // get mask of interrupting devices
        case SCN_INTACK: {
            mr_syscall_rc = intreqreg;
            break;
        }

        // unlink() system call
        case SCN_UNLINK: {
            char const *path = getmemstr (readmemword (data + 2));
            int rc = unlink (path);
            if (rc < 0) save_errno ();
            mr_syscall_rc = rc;
            break;
        }

        // get command line arguments following hex filename
        //  .word   SCN_GETCMDARG
        //  .word   argindex
        //  .word   buffersize
        //  .word   bufferaddr or null
        // returns:
        //     -1 : beyond end
        //   else : length of item (might be gt buffersize)
        case SCN_GETCMDARG: {
            uint16_t i = readmemword (data + 2);
            mr_syscall_rc = -1;
            if (i < cmdargc) {
                char const *cmdptr = cmdargv[i];
                uint16_t cmdlen = strlen (cmdptr);
                uint16_t buflen = readmemword (data + 4);
                if (buflen > cmdlen + 1) buflen = cmdlen + 1;
                uint16_t bufadr = readmemword (data + 6);
                if (bufadr != 0) {
                    void *bufptr = getmemptr (bufadr, buflen, true);
                    memcpy (bufptr, cmdptr, buflen);
                }
                mr_syscall_rc = cmdlen;
            }
            break;
        }

        // floatingpoint functions
        //  .word   uint16_t SCN_xxx_DDD
        //  .word   double *resultaddr
        //  .word   double *leftopaddr
        //  .word   double *riteopaddr
        case SCN_ADD_DDD:
        case SCN_SUB_DDD:
        case SCN_MUL_DDD:
        case SCN_DIV_DDD: {
            double leftop = readmemdoub (readmemword (data + 4));
            double riteop = readmemdoub (readmemword (data + 6));
            double result;
            switch (scn) {
                case SCN_ADD_DDD: result = leftop + riteop; break;
                case SCN_SUB_DDD: result = leftop - riteop; break;
                case SCN_MUL_DDD: result = leftop * riteop; break;
                case SCN_DIV_DDD: result = leftop / riteop; break;
                default: abort ();
            }
            //if (! isfinite (leftop) || ! isfinite (riteop) || ! isfinite (result)) {
            //  fprintf (stderr, "raspictl*: %g = %g %u %g\n", result, leftop, scn, riteop);
            //  dumpregs ();
            //}
            writememdoub (readmemword (data + 2), result);
            break;
        }

        //  .word   uint16_t SCN_xxx_FFF
        //  .word   float *resultaddr
        //  .word   float *leftopaddr
        //  .word   float *riteopaddr
        case SCN_ADD_FFF:
        case SCN_SUB_FFF:
        case SCN_MUL_FFF:
        case SCN_DIV_FFF: {
            float leftop = readmemflt (readmemword (data + 4));
            float riteop = readmemflt (readmemword (data + 6));
            float result;
            switch (scn) {
                case SCN_ADD_FFF: result = leftop + riteop; break;
                case SCN_SUB_FFF: result = leftop - riteop; break;
                case SCN_MUL_FFF: result = leftop * riteop; break;
                case SCN_DIV_FFF: result = leftop / riteop; break;
                default: abort ();
            }
            writememflt (readmemword (data + 2), result);
            break;
        }

        //  .word   uint16_t SCN_CMP_DD
        //  .word   uint16_t comparecode
        //  .word   double *leftopaddr
        //  .word   double *riteopaddr
        case SCN_CMP_DD: {
            uint16_t code = readmemword (data + 2);
            double leftop = readmemdoub (readmemword (data + 4));
            double riteop = readmemdoub (readmemword (data + 6));
            int res = -1;
            switch (code >> 4) {
                case 0: res = leftop >= riteop; break;
                case 1: res = leftop >  riteop; break;
                case 2: res = leftop != riteop; break;
            }
            mr_syscall_rc = res ^ ((code >> 3) & 1);
            break;
        }

        //  .word   uint16_t SCN_CMP_FF
        //  .word   uint16_t comparecode
        //  .word   float *leftopaddr
        //  .word   float *riteopaddr
        case SCN_CMP_FF: {
            uint16_t code = readmemword (data + 2);
            float leftop  = readmemflt (readmemword (data + 4));
            float riteop  = readmemflt (readmemword (data + 6));
            int res = -1;
            switch (code >> 4) {
                case 0: res = leftop >= riteop; break;
                case 1: res = leftop >  riteop; break;
                case 2: res = leftop != riteop; break;
            }
            mr_syscall_rc = res ^ ((code >> 3) & 1);
            break;
        }

        //  .word   uint16_t SCN_CVT_PF
        //  .word   result       w W z : by value ; else : by ref
        //  .word   operand      w W   : by value ; else : by ref
        //  .byte   operandtype  w W l L q Q f d
        //  .byte   resulttype   w W l L q Q f d z
        case SCN_CVT_FP: {
            uint16_t r0 = readmemword (data + 2);
            uint16_t r1 = readmemword (data + 4);
            uint16_t r2 = readmemword (data + 6);
            double value = 0.0;
            switch (r2 & 0xFF) {
                case 'b': value = (double)  (sint8_t) r1; break;
                case 'B': value = (double)  (uint8_t) r1; break;
                case 'w': value = (double) (sint16_t) r1; break;
                case 'W': value = (double) (uint16_t) r1; break;
                case 'l': value = (double) (sint32_t) readmemlong (r1); break;
                case 'L': value = (double) (uint32_t) readmemlong (r1); break;
                case 'q': value = (double) (sint64_t) readmemquad (r1); break;
                case 'Q': value = (double) (uint64_t) readmemquad (r1); break;
                case 'f': value = (double)            readmemflt  (r1); break;
                case 'd': value = (double)            readmemdoub (r1); break;
                default: fprintf (stderr, "raspictl: invalid SCN_CVT_FP from %c\n", r2 & 0xFF);
            }
            //fprintf (stderr, "raspictl SCN_CVT_FP*: %c %g %c\n", r2 & 0xFF, value, r2 >> 8);
            switch (r2 >> 8) {
                case 'b': r0 = (sint16_t) (sint8_t) value; break;
                case 'B': r0 = (uint16_t) (uint8_t) value; break;
                case 'w': r0 = (sint16_t) value; break;
                case 'W': r0 = (uint16_t) value; break;
                case 'l': writememlong (r0, (sint32_t) value); break;
                case 'L': writememlong (r0, (uint32_t) value); break;
                case 'q': writememquad (r0, (sint64_t) value); break;
                case 'Q': writememquad (r0, (uint64_t) value); break;
                case 'f': writememflt  (r0, (float)    value); break;
                case 'd': writememdoub (r0, (double)   value); break;
                case 'z': r0 = (value != 0.0);
                default: fprintf (stderr, "raspictl: invalid SCN_CVT_FP to %c\n", r2 >> 8);
            }
            mr_syscall_rc = r0;
            break;
        }

        //  .word   SCN_UDIV
        //  .word   dividend<15:00>  ->  quotient
        //  .word   dividend<31:16>  ->  remainder
        //  .word   divisor
        case SCN_UDIV: {
            uint32_t dividend = readmemlong (data + 2);
            uint32_t divisor  = readmemword (data + 6);
            mr_syscall_rc = -1;
            if (divisor != 0) {
                uint32_t quotient  = dividend / divisor;
                uint32_t remainder = dividend % divisor;
                mr_syscall_rc = (quotient > 0xFFFF) ? -2 : 0;
                writememlong (data + 2, (quotient & 0xFFFF) | (remainder << 16));
            }
            break;
        }

        //  .word   SCN_UMUL
        //  .word   multiplicand<15:00>  ->  product<31:16>
        //  .word   multiplicand<31:16>  ->  product<31:16>
        //  .word   multiplier
        case SCN_UMUL: {
            uint32_t multiplicand = readmemlong (data + 2);
            uint32_t multiplier   = readmemword (data + 6);
            uint64_t product      = multiplicand * multiplier;
            writememlong (data + 2, product);
            mr_syscall_rc = product >> 32;
            break;
        }

        case SCN_SETROSIZE: {
            readonlysize = readmemword (data + 2);
            mr_syscall_rc = 0;
            break;
        }

        case SCN_SETSTKLIM: {
            stacklimit = readmemword (data + 2);
            mr_syscall_rc = 0;
            break;
        }

        case SCN_PRINTINSTR: {
            shadow.printinstr = (readmemword (data + 2) & 1) != 0;
            mr_syscall_rc = 0;
            break;
        }

        //  .word   SCN_NEG_DD
        //  .word   result
        //  .word   operand
        case SCN_NEG_DD: {
            double op = readmemdoub (readmemword (data + 4));
            writememdoub (readmemword (data + 2), - op);
            mr_syscall_rc = 0;
            break;
        }

        case SCN_NEG_FF: {
            float op = readmemflt (readmemword (data + 4));
            writememflt (readmemword (data + 2), - op);
            mr_syscall_rc = 0;
            break;
        }

        case SCN_WATCHWRITE: {
            watchwrite = readmemword (data + 2);
            mr_syscall_rc = 0;
            break;
        }

        //  .word   SCN_GETENV
        //  .word   bufaddr
        //  .word   bufsize
        //  .word   nameaddr
        // returns 0 if not found, or len incl null
        case SCN_GETENV: {
            mr_syscall_rc = 0;
            char const *name = getmemstr (readmemword (data + 6));
            char *envstr = getenv (name);
            if (envstr != NULL) {
                uint16_t envlen = strlen (envstr) + 1;
                mr_syscall_rc = envlen;
                uint16_t bufsize = readmemword (data + 4);
                if (bufsize > 0) {
                    if (envlen > bufsize) envlen = bufsize;
                    void *bufaddr = getmemptr (readmemword (data + 2), envlen, true);
                    memcpy (bufaddr, envstr, envlen);
                }
            }
            break;
        }

        //  .word   SCN_SETTTYECHO
        //  .word   fd
        //  .word   0 : disable echo
        //          1 : enable echo
        case SCN_SETTTYECHO: {
            struct termios ttyattrs;
            int  fd = (int)(sint16_t) readmemword (data + 2);
            bool echo = readmemword (data + 4) != 0;
            if (savedtermioss.count (fd) == 0) {
                if (tcgetattr (fd, &ttyattrs) < 0) {
                    mr_syscall_rc = -1;
                    save_errno ();
                    break;
                }
                savedtermioss[fd] = ttyattrs;
                if (fd == 0) atexit (restorestdinattrs);
            } else {
                ttyattrs = savedtermioss[fd];
            }
            if (! echo) {
                cfmakeraw (&ttyattrs);
                ttyattrs.c_oflag |= OPOST;
            }
            if (tcsetattr (fd, TCSANOW, &ttyattrs) < 0) {
                mr_syscall_rc = -2;
                save_errno ();
                break;
            }
            mr_syscall_rc = 0;
            break;
        }

        default: {
            mr_syscall_rc = -1;
            errno = -ENOSYS;
            save_errno ();
            fprintf (stderr, "raspictl: unknown syscall %u\n", scn);
            break;
        }
    }
}

static uint16_t mr_syscall (uint32_t sample)
{
    return mr_syscall_rc;
}

static void save_errno ()
{
    *(uint16_t *)(memory + ERRNO) = errno;
}

// runs in background to print cycle rate at beginning of every minute for testing
static void *mintimesthread (void *dummy)
{
    pthread_cond_t cond;
    pthread_mutex_t lock;
    struct timespec nowts, waits;

    pthread_cond_init (&cond, NULL);
    pthread_mutex_init (&lock, NULL);
    pthread_mutex_lock (&lock);

    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) abort ();
    uint64_t lastcycs = shadow.getcycles ();
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (true) {
        int rc;
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        do rc = pthread_cond_timedwait (&cond, &lock, &waits);
        while (rc == 0);
        if (rc != ETIMEDOUT) abort ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) abort ();
        uint64_t cycs = shadow.getcycles ();
        uint32_t secs = nowts.tv_sec;
        fprintf (stderr, "raspictl: %02d:%02d:%02d  %12llu cycles  avg %6llu Hz  %6.3f uS\n",
                    secs / 3600 % 24, secs / 60 % 60, secs % 60,
                    cycs, (cycs - lastcycs) / (secs - lastsecs),
                    (secs - lastsecs) * 1000000.0 / (cycs - lastcycs));
        FILE *profile = fopen ("raspictl.readcounts", "w");
        if (profile != NULL) {
            fwrite (readcounts, sizeof readcounts, 1, profile);
            fclose (profile);
        }
        lastcycs = cycs;
        lastsecs = secs;
    }
    return NULL;
}


// runs in background to post interrupt request whenever lineclockabs time is reached
// exits when woken with lineclockabs.tv == 0
static void *lineclockthread (void *dummy)
{
    pthread_mutex_lock (&intreqlock);
    while (lineclockabs.tv_sec != 0) {

        // if already requsting an interrupt,
        //  block until SCN_LINECLOCK is called
        // else,
        //  block until the given time is reached or SCN_LINECLOCK is called
        int rc = (intreqreg & IRQ_LINECLOCK) ?
                pthread_cond_wait (&lineclockcond, &intreqlock) :
                pthread_cond_timedwait (&lineclockcond, &intreqlock, &lineclockabs);

        // if reached the requested time, post an interrupt request
        if (rc != 0) {
            if (rc != ETIMEDOUT) abort ();
            intreqreg |= IRQ_LINECLOCK;
            pthread_cond_signal (&intreqcond);
        }
    }

    // SCN_LINECLOCK was passed a zero, so exit
    lineclockrun = false;
    pthread_mutex_unlock (&intreqlock);
    return NULL;
}

// send data byte/word to CPU in response to a MEM_READ cycle
// we are half way through the cycle after the CPU asserted MEM_READ with clock still high
// we must send the data to the cpu with time to let it soak in before raising clock again
// so we:
//  1) drop the clock and output data to CPU
//  2) leave the data there with clock negated for half a clock cycle
//  3) raise the clock
//  4) leave the data there with clock asserted for another half a clock cycle
// now we are halfway through a cpu cycle with the clock still asserted
// for example:
//  previous cycle was FETCH1 where the cpu sent us MEM_READ with the program counter
//  we are now halfway through FETCH2 with clock still high
//  we drop the clock and send the data to the cpu by dropping G_DENA and asserting G_QENA
//  we wait a half cycle whilst the data soaks into the instruction register latch and instruction decoding circuitry
//  we raise the clock leaving the data going to the cpu so it will get clocked in correctly
//  we wait a half cycle whilst cpu is closing the instruction register latch and transitioning to first state of the instruction
static void senddata (uint16_t data)
{
    // drop the clock and start sending data to cpu
    gpio->writegpio (true, (data * G_DATA0) | syncintreq);

    // let data soak into cpu (giving it a setup time)
    gpio->halfcycle ();

    // tell cpu data is ready to be read by raising the clock
    // hold data steady and keep sending data to cpu so it will be clocked in correctly
    SHADOWCHECK (gpio->readgpio ());
    gpio->writegpio (true, G_CLK | (data * G_DATA0) | syncintreq);

    // wait for cpu to clock data in (giving it a hold time)
    gpio->halfcycle ();
}

// recv data byte/word from CPU as part of a MEM_WRITE cycle
// we are half way through the cycle after the CPU asserted MEM_WRITE with clock still high
// give the CPU the rest of this cycle just up to the point where
// ...we are about to raise the clock for it to come up with the data
// so we:
//  1) drop the clock and make sure G_DENA is enabled so we can read the data
//  2) wait for the end of the cycle following MEM_WRITE cycle
//     this gives the CPU time to put the data on the bus
//  3) read the data from the CPU just before raising clock
//  4) raise clock
//  5) wait for half a clock cycle
// for example:
//  previous cycle was STORE1 where cpu sent us the address to write to with MEM_WRITE asserted
//  we are now halfway through STORE2 where cpu is sending us the data to write
//  we drop the clock
//  we wait a half cycle for the cpu to finish sending the data
//  it is now tge very end of the STORE2 cycle
//  we read the data from the gpio pins
static uint16_t recvdata (void)
{
    uint32_t sample;

    // drop clock and wait for cpu to finish sending the data
    gpio->writegpio (false, syncintreq);
    gpio->halfcycle ();

    // read data being sent by cpu just before raising clock
    // then raise clock and wait a half cycle to be at same timing as when called
    sample = gpio->readgpio ();
    SHADOWCHECK (sample);
    gpio->writegpio (false, G_CLK | syncintreq);
    gpio->halfcycle ();

    // return value received from cpu to be written to memory
    return sample / G_DATA0;
}

// read memory with bounds checking
static uint16_t readmemword (uint32_t addr)
{
    if (addr + 2 > MAGIC) {
        fprintf (stderr, "raspictl: bad readmemword %08X\n", addr);
        return 0;
    }
    return *(uint16_t *)(memory + addr);
}

static uint32_t readmemlong (uint32_t addr)
{
    uint32_t val;
    void *ptr = getmemptr (addr, sizeof val, false);
    memcpy (&val, ptr, sizeof val);
    return val;
}

static uint64_t readmemquad (uint32_t addr)
{
    uint64_t val;
    void *ptr = getmemptr (addr, sizeof val, false);
    memcpy (&val, ptr, sizeof val);
    return val;
}

static double readmemdoub (uint32_t addr)
{
    double val;
    void *ptr = getmemptr (addr, sizeof val, false);
    memcpy (&val, ptr, sizeof val);
    return val;
}

static float readmemflt (uint32_t addr)
{
    float val;
    void *ptr = getmemptr (addr, sizeof val, false);
    memcpy (&val, ptr, sizeof val);
    return val;
}

static void writememlong (uint32_t addr, uint32_t val)
{
    void *ptr = getmemptr (addr, sizeof val, true);
    memcpy (ptr, &val, sizeof val);
}

static void writememquad (uint32_t addr, uint64_t val)
{
    void *ptr = getmemptr (addr, sizeof val, true);
    memcpy (ptr, &val, sizeof val);
}

static void writememdoub (uint32_t addr, double val)
{
    void *ptr = getmemptr (addr, sizeof val, true);
    memcpy (ptr, &val, sizeof val);
}

static void writememflt  (uint32_t addr, float val)
{
    void *ptr = getmemptr (addr, sizeof val, true);
    memcpy (ptr, &val, sizeof val);
}

static void *getmemptr (uint32_t addr, uint32_t size, bool write)
{
    if (addr + size > MAGIC) {
        fprintf (stderr, "raspictl: bad getmemptr %04X size %04X\n", addr, size);
        dumpregs ();
        abort ();
    }
    if (write) {
        if ((watchwrite >= addr) && (watchwrite - addr < size)) {
            fprintf (stderr, "raspictl: watch write addr %04X by system service\n", watchwrite);
            dumpregs ();
        }
        if (addr < readonlysize) {
            fprintf (stderr, "raspictl: write address %04X below rosize %04X\n", addr, readonlysize);
            dumpregs ();
            abort ();
        }
    }
    return memory + addr;
}

static char const *getmemstr (uint32_t addr)
{
    uint32_t size;
    for (size = 0; addr + size < MAGIC; size ++) {
        if (memory[addr+size] == 0) return (char const *) (memory + addr);
    }
    fprintf (stderr, "raspictl: bad getmemstr %08X at PC=%04X\n", addr, shadow.regs[7]);
    abort ();
}

// verify CPU state and update shadow state
// should be called just before raising clock
//  input:
//   sample = value just read from gpio pins
static void shadowcheck (uint32_t sample)
{
    // check it, abort if error
    if (shadow.check (sample)) abort ();

    // update shadow state to match what CPU will be after is sees clock raised
    // ...based on what state it is in now and what we last sent out on GPIO
    uint16_t mq = sample / G_DATA0;
    bool irq = (sample & G_IRQ) != 0;
    if (shadow.clock (mq, irq)) abort ();
}

// terminated, maybe turn stdin echo back on
static void restorestdinattrs ()
{
    if (savedtermioss.count (0) != 0) {
        tcsetattr (0, TCSANOW, &savedtermioss[0]);
    }
}
