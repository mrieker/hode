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
 * Interactive program for testing Raspi interface circuits.
 * Can be used on the pb2board.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "gpiolib.h"

#define LOCKMTX(mtx) do { if (pthread_mutex_lock (mtx) != 0) abort (); } while (0)
#define UNLKMTX(mtx) do { if (pthread_mutex_unlock (mtx) != 0) abort (); } while (0)
#define ever (;;)

struct Command {
    char const *str;
    void (*ent) (int nwords, char **words);
};

struct GpioPin {
    int num;
    char const *nam;
};

struct Oscillator {
    Oscillator *next;
    int num;
    uint32_t nsec;
    pthread_t pthid;
};

static GpioPin const gpiotbl[] = {
    {  2, "clk"   },
    {  3, "reset" },
    {  4, "d0"    },
    {  5, "d1"    },
    {  6, "d2"    },
    {  7, "d3"    },
    { 20, "irq"   },
    { 21, "qena"  },
    { 22, "dena"  },
    { 23, "word"  },
    { 24, "halt"  },
    { 25, "read"  },
    { 26, "write" },
    {  0, NULL    } };

static void cmd_clr (int nwords, char **words);
static void cmd_get (int nwords, char **words);
static void cmd_mon (int nwords, char **words);
static void cmd_osc (int nwords, char **words);
static void cmd_set (int nwords, char **words);

static Command const cmdtbl[] = {
    { "clr", cmd_clr },     // clear the named gpio pins
    { "get", cmd_get },     // display named gpio pin values
    { "mon", cmd_mon },     // monitor changes of the named gpio pins
    { "osc", cmd_osc },     // oscillate named pin at given frequency
    { "set", cmd_set },     // set the named gpio pins
    { NULL, NULL } };

static PhysLib gpio (DEFCPUHZ);
static Oscillator *oscillators;
static pthread_mutex_t oscmutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t volatile monmask;

static bool readprompt (char *buff, int size, char const *prompt);
static void *monthread (void *dummy);
static void *oscthread (void *oscv);
static int decodegpio (char const *name);
static void makeinputpins ();
static void makeoutputpins ();

int main ()
{
    // stay on one core so rdcyc() will work
    /**
    cpu_set_t cpuset;
    CPU_ZERO (&cpuset);
    CPU_SET (1, &cpuset);
    if (sched_setaffinity (0, sizeof cpuset, &cpuset) < 0) {
        abort ();
    }
    **/

    gpio.open ();

    pthread_t mtid;
    int rc = pthread_create (&mtid, NULL, monthread, NULL);
    if (rc != 0) abort ();

    for ever {
        char line[80], *words[41];

        if (! readprompt (line, sizeof line, "> ")) break;

        int nwords = 0;
        bool skip = true;
        char c;
        for (int i = 0; (c = line[i]) != 0; i ++) {
            if (c <= ' ') {
                skip = true;
                line[i] = 0;
            } else if (skip) {
                words[nwords++] = &line[i];
                skip = false;
            }
        }
        if (nwords == 0) continue;
        words[nwords] = NULL;

        Command const *cmd;
        for (int i = 0; (cmd = &cmdtbl[i])->str != NULL; i ++) {
            if (strcasecmp (cmd->str, words[0]) == 0) {
                cmd->ent (nwords, words);
                goto next;
            }
        }
        for (int i = 0; (cmd = &cmdtbl[i])->str != NULL; i ++) {
            printf (" %s", cmd->str);
        }
        fputs ("\n", stdout);
        for (int i = 0; gpiotbl[i].nam != NULL; i ++) {
            printf (" %s", gpiotbl[i].nam);
        }
        fputs ("\n", stdout);
    next:;
    }
}

// read a line from keyboard with a prompt string
// use readline history to save old lines for later recall
static bool readprompt (char *buff, int size, char const *prompt)
{
    char *expansion, *line;
    int result;

    static char *lastline = NULL;
    static int initted = 0;

    if (! initted) {
        using_history ();
        stifle_history (500);
        history_expansion_char = 0;
        rl_bind_key ('\t', rl_insert);
        initted = 1;
    }

readit:
    line = readline (prompt);
    if (line == NULL) {
        fputc ('\n', stderr);
        return false;
    }
    if ((line[0] != 0) && ((lastline == NULL) || (strcmp (line, lastline) != 0))) {
        result = history_expand (line, &expansion);
        if (result != 0) {
            fprintf (stderr, "%s\n", expansion);
        }
        if ((result < 0) || (result == 2)) {
            free (expansion);
            goto readit;
        }
        add_history (expansion);
        free (line);
        line = expansion;
    }
    if (lastline != NULL) free (lastline);
    lastline = line;
    strncpy (buff, line, -- size);
    buff[size] = 0;
    return true;
}

// set named gpio pin(s) to zero
static void cmd_clr (int nwords, char **words)
{
    uint32_t mask = 0;
    for (int i = 0; ++ i < nwords;) {
        int gpio = decodegpio (words[i]);
        if (gpio < 0) {
            fprintf (stderr, "bad gpio %s\n", words[i]);
            return;
        }
        mask |= 1U << gpio;
    }
    if (mask & G_DENA) makeinputpins ();
    gpio.clrpins (mask);
}

// display named gpio pin(s) value
static void cmd_get (int nwords, char **words)
{
    uint32_t mask = gpio.getpins ();
    for (int i = 0; ++ i < nwords;) {
        int gpio = decodegpio (words[i]);
        if (gpio < 0) {
            fprintf (stderr, "bad gpio %s\n", words[i]);
            return;
        }
        printf ("%8s = %u\n", words[i], (mask >> gpio) & 1U);
    }
}

// monitor named gpio pin(s)
static void cmd_mon (int nwords, char **words)
{
    uint32_t mask = monmask;
    bool addop = true;
    bool subop = false;

    for (int i = 0; ++ i < nwords;) {
        char *word = words[i];
        switch (word[0]) {
            case '=': {
                mask = 0;
                //fallthrough
            }
            case '+': {
                addop = true;
                subop = false;
                word ++;
                break;
            }
            case '-': {
                addop = false;
                subop = true;
                word ++;
                break;
            }
        }
        if (word[0] != 0) {
            int gpio = decodegpio (words[i]);
            if (gpio < 0) {
                fprintf (stderr, "bad gpio %s\n", words[i]);
                return;
            }
            if (addop) mask |=    1U << gpio;
            if (subop) mask &= ~ (1U << gpio);
        }
    }
    monmask = mask;
}

// oscillate named gpio pin
static void cmd_osc (int nwords, char **words)
{
    switch (nwords) {
        case 2: {
            int gpio = decodegpio (words[1]);
            if (gpio < 0) {
                fprintf (stderr, "bad gpio %s\n", words[1]);
                return;
            }
            LOCKMTX (&oscmutex);
            Oscillator *osc;
            for (osc = oscillators; osc != NULL; osc = osc->next) {
                if (osc->num == gpio) break;
            }
            if (osc == NULL) {
                printf ("%s not oscillating\n", words[1]);
            } else {
                printf ("%s oscillating %u nsec\n", words[1], osc->nsec);
            }
            UNLKMTX (&oscmutex);
            break;
        }
        case 3: {
            int gpio = decodegpio (words[1]);
            if (gpio < 0) {
                fprintf (stderr, "bad gpio %s\n", words[1]);
                return;
            }
            char *p;
            uint32_t nsec = strtoul (words[2], &p, 0);
            if (*p != 0) {
                fprintf (stderr, "bad nsec %s\n", words[2]);
                return;
            }
            LOCKMTX (&oscmutex);
            Oscillator *osc;
            for (osc = oscillators; osc != NULL; osc = osc->next) {
                if (osc->num == gpio) break;
            }
            if (osc != NULL) {
                osc->nsec = nsec;
            } else if (nsec != 0) {
                osc = new Oscillator ();
                osc->next   = oscillators;
                osc->num    = gpio;
                osc->nsec   = nsec;
                oscillators = osc;
                int rc = pthread_create (&osc->pthid, NULL, oscthread, osc);
                if (rc != 0) abort ();
            }
            UNLKMTX (&oscmutex);
            break;
        }
        default: {
            fprintf (stderr, "osc <pin> [<nsec>]\n");
            break;
        }
    }
}

// set named gpio pin(s) to one
static void cmd_set (int nwords, char **words)
{
    uint32_t mask = 0;
    for (int i = 0; ++ i < nwords;) {
        int gpio = decodegpio (words[i]);
        if (gpio < 0) {
            fprintf (stderr, "bad gpio %s\n", words[i]);
            return;
        }
        mask |= 1U << gpio;
    }
    gpio.setpins (mask);
    if (mask & G_DENA) makeoutputpins ();
}

// thread monitors given GPIO pins, prints changed values whenever seen
static void *monthread (void *dummy)
{
    struct timespec const onems = { 0, 1000000 };
    uint32_t last = 0;
    for ever {
        uint32_t mask = monmask;
        uint32_t bits = gpio.getpins ();
        if ((bits ^ last) & mask) {
            printf ("\n");
            for (int i = 0; gpiotbl[i].nam != NULL; i ++) {
                if ((mask >> gpiotbl[i].num) & 1) {
                    printf ("%8s = %u", gpiotbl[i].nam, (bits >> gpiotbl[i].num) & 1);
                }
            }
            printf ("\n");
        }
        last = bits;
        if (nanosleep (&onems, NULL) < 0) abort ();
    }
    return NULL;
}

// oscillate a gpio pin value
static void *oscthread (void *oscv)
{
    Oscillator *osc = (Oscillator *) oscv;
    int value = 0;

    uint32_t nsec;
    while ((nsec = osc->nsec) != 0) {
        struct timespec nowspec;
        if (clock_gettime (CLOCK_MONOTONIC, &nowspec) < 0) abort ();

        uint64_t nownsec = nowspec.tv_sec * ((uint64_t) 1000000000) + nowspec.tv_nsec;
        uint32_t sleepnsec = nsec - nownsec % nsec;

        struct timespec sleepfor;
        sleepfor.tv_sec = 0;
        sleepfor.tv_nsec = sleepnsec;
        if (nanosleep (&sleepfor, NULL) < 0) abort ();

        value = 1 - value;
        if (value) {
            gpio.setpins (1U << osc->num);
        } else {
            gpio.clrpins (1U << osc->num);
        }
    }
    LOCKMTX (&oscmutex);
    Oscillator **losc;
    for (losc = &oscillators; (osc = *losc) != oscv; losc = &osc->next) {
        if (osc == NULL) abort ();
    }
    *losc = osc->next;
    delete osc;
    UNLKMTX (&oscmutex);
    return NULL;
}

// decode GPIO pin name and return GPIO pin number
// can be either one of the names or an integer pin number
static int decodegpio (char const *name)
{
    for (int i = 0; gpiotbl[i].nam != NULL; i ++) {
        if (strcasecmp (name, gpiotbl[i].nam) == 0) return gpiotbl[i].num;
    }
    char *p;
    int n = strtol (name, &p, 0);
    if ((*p == 0) && (p != name) && (n >= 0) && (n <= 31)) return n;
    return -1;
}

// make the data pins readable
static void makeinputpins ()
{
    gpio.makeinputpins ();
    printf ("databus readable\n");
}

// make the data pins writable
static void makeoutputpins ()
{
    gpio.makeinputpins ();
    printf ("databus writable\n");
}
