
CLASSDIR = classes

MACH = $(shell uname -m)

IOW    = ../iowarrior/iowkit_1.7_$(MACH)/libiowkit-1.7.0
IOWINC = $(IOW)/insdir/include
IOWJAV = $(IOW)/java/codemercs.jar

JNINC = jninc_$(MACH)
JAVAC = javac -Xlint:unchecked -d $(CLASSDIR) -sourcepath .

CP = $(CLASSDIR):jacl141/lib/tcljava1.4.1/tcljava.jar:jacl141/lib/tcljava1.4.1/jacl.jar:$(IOWJAV)

CLASSFILES = \
	BufLEDModule.class \
	Comp.class \
	CompTree.class \
	ConnModule.class \
	Const.class \
	DelayModule.class \
	DFFModule.class \
	DLatModule.class \
	Gate.class \
	GenCtx.class \
	GPIOModule.class \
	HaltedException.class \
	IOW56List.class \
	IOW56Module.class \
	IParam.class \
	MiniNand2.class \
	Module.class \
	NetClass.class \
	NetGen.class \
	Network.class \
	OParam.class \
	OldNetFile.class \
	OpndLhs.class \
	OpndRhs.class \
	ParsePCB.class \
	Placeable.class \
	PropNode.class \
	PropPrinter.class \
	PropRoot.class \
	PullUpModule.class \
	RasPiModule.class \
	Reduce.class \
	ReduceEx.class \
	RField.class \
	Shadow.class \
	Simulator.class \
	StandCell.class \
	Token.class \
	UserModule.class \
	WField.class \
	WiredAnd.class

default: $(addprefix $(CLASSDIR)/,$(CLASSFILES)) $(CLASSDIR)/libGPIOModule.$(MACH).so

clean:
	rm -rf $(CLASSDIR)

$(CLASSDIR)/%.class: %.java jacl141
	mkdir -p $(CLASSDIR)
	rm -f $@ $(@:.class=\$$*.class)
	CLASSPATH=$(CP) $(JAVAC) $<

$(CLASSDIR)/libGPIOModule.$(MACH).so: GPIOModule.cc $(CLASSDIR)/GPIOModule.h ../driver/physlib.cc $(IOW)
	mkdir -p $(CLASSDIR)
	rm -f $@
	g++ -Wall -O2 -shared -o $(CLASSDIR)/libGPIOModule.$(MACH).so -fPIC \
		-I$(JNINC) -I$(JNINC)/linux -I$(IOWINC) -I$(CLASSDIR) GPIOModule.cc ../driver/physlib.cc

$(CLASSDIR)/GPIOModule.h: $(CLASSDIR)/GPIOModule.class
	mkdir -p $(CLASSDIR)
	rm -f $(CLASSDIR)/GPIOModule.h $(CLASSDIR)/GPIOModule_*.h
	CLASSPATH=$(CP) javah -d $(CLASSDIR) -sourcepath . GPIOModule

$(IOW):
	cd ../iowarrior ; make iowkit_1.7_$(MACH)/libiowkit-1.7.0/insdir

jacl141: jaclBinary141.zip
	unzip jaclBinary141.zip

jaclBinary141.zip:
	wget --no-check-certificate https://phoenixnap.dl.sourceforge.net/project/tcljava/jacl/1.4.1/jaclBinary141.zip

