MACH   = $(shell uname -m)
IOWKIT = ../iowarrior/iowkit_1.7_$(MACH)/libiowkit-1.7.0/insdir
GPP    = g++ -g -Wall -I$(IOWKIT)/include
ASM    = ../asm/assemble.$(MACH)
LNK    = ../asm/link.$(MACH)

ifeq ($(MACH),armv6l)
	HASTSC = 0
	UNIPROC = 1
else
	HASTSC = 1
	UNIPROC = 0
endif

default: \
	altr314.$(MACH) \
	aluspeedtest.$(MACH) \
	alutester.$(MACH) \
	clockit.$(MACH) \
	comr3.$(MACH) \
	dumpreadcount.$(MACH) \
	flipflag.$(MACH) \
	haltloop.$(MACH) \
	iow56list.$(MACH) \
	ledstest.$(MACH) \
	ledtester.$(MACH) \
	raseqhaltloop.$(MACH) \
	raseqspeed.$(MACH) \
	raseqtest.$(MACH) \
	raspictl.$(MACH) \
	raspitest.$(MACH) \
	raspitest2.$(MACH) \
	raspitest3.$(MACH) \
	rastester.$(MACH) \
	regtester.$(MACH) \
	regtiming.$(MACH) \
	resethaltloop.$(MACH) \
	seqtester.$(MACH) \
	timingchain.$(MACH) \
	writeallonestopc.$(MACH) \
	writezeroestoir.$(MACH) \
	writezeroestorn.$(MACH) \

altr314.$(MACH): altr314.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc shadow.cc gpiolib.h miscdefs.h rdcyc.h shadow.h $(IOWKIT)
	$(GPP) -o altr314.$(MACH) -DHASTSC=$(HASTSC) -DUNIPROC=$(UNIPROC) altr314.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc shadow.cc $(IOWKIT)/lib/libiowkit.a -lpthread

aluspeedtest.$(MACH): aluspeedtest.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o aluspeedtest.$(MACH) -DHASTSC=$(HASTSC) aluspeedtest.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

alutester.$(MACH): alutester.cc alu8.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o alutester.$(MACH) -DHASTSC=$(HASTSC) alutester.cc alu8.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

clockit.$(MACH): clockit.cc physlib.cc rdcyc.cc $(IOWKIT)
	$(GPP) -o clockit.$(MACH) -DHASTSC=$(HASTSC) clockit.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

comr3.$(MACH): comr3.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o comr3.$(MACH) -DHASTSC=$(HASTSC) disassemble.cc comr3.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a -lpthread

dumpreadcount.$(MACH): dumpreadcount.c
	cc -g -o dumpreadcount.$(MACH) dumpreadcount.c

flipflag.$(MACH): flipflag.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o flipflag.$(MACH) -DHASTSC=$(HASTSC) flipflag.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

haltloop.$(MACH): haltloop.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o haltloop.$(MACH) -DHASTSC=$(HASTSC) haltloop.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

iow56list.$(MACH): iow56list.cc rdcyc.cc $(IOWKIT)
	$(GPP) -o iow56list.$(MACH) -DHASTSC=$(HASTSC) iow56list.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

ledstest.$(MACH): ledstest.cc leds.cc gpiolib.cc gpiolib.h leds.h
	$(GPP) -o ledstest.$(MACH) ledstest.cc gpiolib.cc leds.cc -lcurses

ledtester.$(MACH): ledtester.cc gpiolib.cc leds.cc physlib.cc rdcyc.cc gpiolib.h leds.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o ledtester.$(MACH) -DHASTSC=$(HASTSC) ledtester.cc gpiolib.cc leds.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a -lcurses

raseqhaltloop.$(MACH): raseqhaltloop.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o raseqhaltloop.$(MACH) -DHASTSC=$(HASTSC) raseqhaltloop.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

raseqspeed.$(MACH): raseqspeed.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o raseqspeed.$(MACH) -DHASTSC=$(HASTSC) raseqspeed.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

raseqtest.$(MACH): raseqtest.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o raseqtest.$(MACH) -DHASTSC=$(HASTSC) raseqtest.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

raspictl.$(MACH): raspictl.cc disassemble.cc gpiolib.cc nohwlib.cc physlib.cc pipelib.cc rdcyc.cc shadow.cc gpiolib.h miscdefs.h rdcyc.h shadow.h $(IOWKIT)
	$(GPP) -O2 -o raspictl.$(MACH) -DHASTSC=$(HASTSC) -DUNIPROC=$(UNIPROC) raspictl.cc disassemble.cc gpiolib.cc nohwlib.cc physlib.cc pipelib.cc rdcyc.cc shadow.cc $(IOWKIT)/lib/libiowkit.a -lpthread

raspitest.$(MACH): raspitest.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc gpiolib.h miscdefs.h $(IOWKIT)
	$(GPP) -o raspitest.$(MACH) -DHASTSC=$(HASTSC) raspitest.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a -lpthread -lreadline

raspitest2.$(MACH): raspitest2.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o raspitest2.$(MACH) -DHASTSC=$(HASTSC) raspitest2.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

raspitest3.$(MACH): raspitest3.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o raspitest3.$(MACH) -DHASTSC=$(HASTSC) raspitest3.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

rastester.$(MACH): rastester.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o rastester.$(MACH) -DHASTSC=$(HASTSC) rastester.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

regtester.$(MACH): regtester.cc gpiolib.cc physlib.cc rdcyc.cc gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o regtester.$(MACH) -DHASTSC=$(HASTSC) regtester.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

regtiming.$(MACH): regtiming.cc gpiolib.cc physlib.cc rdcyc.cc gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o regtiming.$(MACH) -DHASTSC=$(HASTSC) regtiming.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

resethaltloop.$(MACH): resethaltloop.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc alu8.h disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o resethaltloop.$(MACH) -DHASTSC=$(HASTSC) resethaltloop.cc alu8.cc disassemble.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

rollights.hex: rollights.asm
	$(ASM) rollights.asm rollights.obj > rollights.lis
	$(LNK) -o rollights.hex rollights.obj > rollights.map

seqtester.$(MACH): seqtester.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o seqtester.$(MACH) -DHASTSC=$(HASTSC) seqtester.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

timingchain.$(MACH): timingchain.cc gpiolib.cc physlib.cc rdcyc.cc gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o timingchain.$(MACH) -DHASTSC=$(HASTSC) timingchain.cc gpiolib.cc physlib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

writeallonestopc.$(MACH): writeallonestopc.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o writeallonestopc.$(MACH) -DHASTSC=$(HASTSC) writeallonestopc.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

writezeroestoir.$(MACH): writezeroestoir.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o writezeroestoir.$(MACH) -DHASTSC=$(HASTSC) writezeroestoir.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

writezeroestorn.$(MACH): writezeroestorn.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc disassemble.h gpiolib.h miscdefs.h rdcyc.h $(IOWKIT)
	$(GPP) -o writezeroestorn.$(MACH) -DHASTSC=$(HASTSC) writezeroestorn.cc disassemble.cc gpiolib.cc physlib.cc pipelib.cc rdcyc.cc $(IOWKIT)/lib/libiowkit.a

$(IOWKIT):
	cd ../iowarrior ; make iowkit_1.7_$(MACH)/libiowkit-1.7.0/insdir

