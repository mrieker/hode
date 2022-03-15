MACH = $(shell uname -m)

CC  = ../cc/compile.sh
AR  = ../asm/archive.$(MACH)
ASM = ../asm/assemble.$(MACH)
LNK = ../asm/link.$(MACH)

LIB = abort.hode.o arith.hode.o assert.hode.o bootpage.hode.o comp.hode.o complexd.hode.o complexf.hode.o endtags.hode.o float.hode.o fpmp.hode.o malloc.hode.o mathd.hode.o mathf.hode.o qsort.hode.o sserv.hode.o start.hode.o stdio.hode.o strerror.hode.o string.hode.o strtod.hode.o strtoul.hode.o throw.hode.o vxprintf.hode.o vxprintf_fp.hode.o

default: assemble.hex circus.hex printpi.hex

assemble.hex: ../asm/assemble.hode.o crtl.hode.a
	$(LNK) -o assemble.hex ../asm/assemble.hode.o crtl.hode.a > assemble.map

../asm/assemble.hode.o: ../asm/assemble.c
	$(CC) ../asm/assemble

circus.hex: circus.hode.o crtl.hode.a
	$(LNK) -o circus.hex circus.hode.o crtl.hode.a > circus.map

circus.hode.o: circus.c
	$(CC) circus

printpi.hex: printpi.hode.o crtl.hode.a
	$(LNK) -o printpi.hex printpi.hode.o crtl.hode.a > printpi.map

printpi.hode.o: printpi.c
	$(CC) printpi

x.hex:	x.hode.o crtl.hode.a
	$(LNK) -o x.hex x.hode.o crtl.hode.a > x.map

x.hode.o: x.c
	$(CC) x

sltest.hex: sltest.hode.o crtl.hode.a
	$(LNK) -o sltest.hex sltest.hode.o crtl.hode.a > sltest.map

sltest.hode.o: sltest.c
	$(CC) sltest


crtl.hode.a: $(LIB)
	$(AR) -o crtl.hode.a $(LIB)


abort.hode.o: abort.c
	$(CC) abort

arith.hode.o: arith.asm
	$(ASM) arith.asm arith.hode.o > arith.lis

assert.hode.o: assert.c
	$(CC) assert

bootpage.hode.o: bootpage.asm
	$(ASM) bootpage.asm bootpage.hode.o > bootpage.lis

comp.hode.o: comp.asm
	$(ASM) comp.asm comp.hode.o > comp.lis

complexd.hode.o: complexd.c complex_.c
	$(CC) complexd

complexf.hode.o: complexf.c complex_.c
	$(CC) complexf

endtags.hode.o: endtags.asm
	$(ASM) endtags.asm endtags.hode.o > endtags.lis

float.hode.o: float.asm
	$(ASM) float.asm float.hode.o > float.lis

fpmp.hode.o: fpmp.asm
	$(ASM) fpmp.asm fpmp.hode.o > fpmp.lis

malloc.hode.o: malloc.c
	$(CC) malloc

mathd.hode.o: mathd.c math_.c
	$(CC) mathd

mathf.hode.o: mathf.c math_.c
	$(CC) mathf

qsort.hode.o: qsort.c
	$(CC) qsort

sserv.hode.o: sserv.asm
	$(ASM) sserv.asm sserv.hode.o > sserv.lis

start.hode.o: start.c
	$(CC) start

stdio.hode.o: stdio.c
	$(CC) stdio

strerror.hode.o: strerror.c
	$(CC) strerror

string.hode.o: string.asm
	$(ASM) string.asm string.hode.o > string.lis

strtod.hode.o: strtod.c
	$(CC) strtod

strtoul.hode.o: strtoul.c
	$(CC) strtoul

throw.hode.o: throw.asm
	$(ASM) throw.asm throw.hode.o > throw.lis

vxprintf.hode.o: vxprintf.c
	$(CC) vxprintf

vxprintf_fp.hode.o: vxprintf_fp.c
	$(CC) vxprintf_fp

