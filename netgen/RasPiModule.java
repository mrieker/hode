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
 * Built-in RaspberryPi module.
 * It emulates memory and memory-mapped IO devices.
 */

import java.io.IOException;
import java.io.PrintStream;

public class RasPiModule extends Module {
    public final static int CLKNS = 1000;
    public final static int MAGIC = 0xFFF0;
    public final static int M_EXIT = 0;
    public final static int M_PRINTLN = 1;
    public final static int M_INTREQ = 2;

    private boolean intreq;
    private boolean shaden;
    private double moduleleftx;
    private double moduletopy;
    private int clkns;
    private int nbits;
    private long forcev;
    private Shadow shadow;
    private String instvarsuf;

    private Network clkcpunet;      // cpu side of clock level converter
    private Network irqcpunet;      // cpu side of irq level converter
    private Network rescpunet;      // cpu side of reset level converter
    private Network v33net;         // 3.3V coming from raspi for level converters
    private Network mreadcpunet;    // cpu side of mread level converter
    private Network mwritecpunet;   // cpu side of mwrite level converter
    private Network mwordcpunet;    // cpu side of mword level converter
    private Network haltcpunet;     // cpu side of halt level converter
    private Network[] mdcpunets;    // cpu side of data being sent to raspi level converter
    private Network[] mqcpunets;    // cpu side of data being read from raspi level converter
    private OpndLhs clk;
    private OpndLhs irq;
    private OpndLhs res;
    private OpndLhs mq;
    private OpndLhs md;
    private OpndLhs mword;
    private OpndLhs mread;
    private OpndLhs mwrite;
    private OpndLhs halt;

    public RasPiModule (String instvarsuf)
    {
        this (instvarsuf, 1000, 16);
    }

    public RasPiModule (String instvarsuf, int clkns, int nbits)
    {
        name = "RasPi" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.clkns = clkns;
        this.nbits = nbits;

        clk = new CLKOutput ("CLK" + instvarsuf, 0);
        irq = new IRQOutput ("IRQ" + instvarsuf, 1);
        res = new RESOutput ("RES" + instvarsuf, 2);
         mq = new  MQOutput ( "MQ" + instvarsuf, 3);

        if (instvarsuf.equals ("")) {
                md = new IParam (    "MD", 4);
             mword = new IParam ( "MWORD", 5);
             mread = new IParam ( "MREAD", 6);
            mwrite = new IParam ("MWRITE", 7);
              halt = new IParam (  "HALT", 8);
        } else {
                md = new OpndLhs (    "MD" + instvarsuf);
             mword = new OpndLhs ( "MWORD" + instvarsuf);
             mread = new OpndLhs ( "MREAD" + instvarsuf);
            mwrite = new OpndLhs ("MWRITE" + instvarsuf);
              halt = new OpndLhs (  "HALT" + instvarsuf);
        }

        mq.hidim = md.hidim = nbits - 1;

        params = new OpndLhs[] { clk, irq, res, mq, md, mword, mread, mwrite, halt };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }

        shadow = new Shadow ();
    }

    // this gets the cycle time token after the module name and before the (
    // eg, inst : RasPi 800 test4 ( ... );
    //                   ^ token is here
    //                            ^ return token here
    // by default, use 1000
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        int clkns = CLKNS;
        int nbits = 16;
        while (! token.isDel (Token.DEL.OPRN)) {
            if (token instanceof Token.Int) {
                Token.Int tokint = (Token.Int) token;
                clkns = (int) tokint.value;
                if ((clkns <= 0) || (clkns % OpndLhs.SIM_STEPNS != 0)) {
                    throw new Exception ("clkns " + clkns + " must be gt 0 and multiple of " + OpndLhs.SIM_STEPNS);
                }
                token = token.next;
                continue;
            }
            if ("test4".equals (token.getSym ())) {
                nbits = 4;
                token = token.next;
                continue;
            }
            throw new Exception ("bad raspi config " + token.toString ());
        }
        instconfig_r[0] = new int[] { clkns, nbits };
        return token;
    }

    // make a new RasPi module instance with wires for parameters
    // the parameter names are suffixed by instvarsuf
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        RasPiModule raspiinst = new RasPiModule (instvarsuf,
                ((int[]) instconfig)[0], ((int[]) instconfig)[1]);

        // make connector's pin variables available to target module
        target.variables.putAll (raspiinst.variables);

        // add output pins to list of top-level things that need to have circuits generated
        // this is how we guarantee circuits that drive these pins will be generated
        target.generators.add (    md);
        target.generators.add ( mword);
        target.generators.add ( mread);
        target.generators.add (mwrite);
        target.generators.add (  halt);

        return raspiinst.params;
    }

    // the internal circuitry that writes to the output wires

    private class CLKOutput extends OParam {
        public CLKOutput (String name, int index)
        {
            super (name, index);
        }

        public RasPiModule getRasPi ()
        {
            return RasPiModule.this;
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// GENERATOR ////

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            assert rbit == 0;
            getCircuit (genctx);
            return clkcpunet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("RasPi", this, rbit);
        }

        //// SIMULATOR ////

        @Override
        protected int busWidthWork ()
        {
            return 1;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    private class IRQOutput extends OParam {
        public IRQOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// GENERATOR ////

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            assert rbit == 0;
            getCircuit (genctx);
            return irqcpunet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("RasPi", this, rbit);
        }

        //// SIMULATOR ////

        @Override
        protected int busWidthWork ()
        {
            return 1;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    private class RESOutput extends OParam {
        public RESOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// GENERATOR ////

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            assert rbit == 0;
            getCircuit (genctx);
            return rescpunet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("RasPi", this, rbit);
        }

        //// SIMULATOR ////

        @Override
        protected int busWidthWork ()
        {
            return 1;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    private class MQOutput extends OParam {
        public MQOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// GENERATOR ////

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            getCircuit (genctx);
            return mqcpunets[rbit];
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("RasPi", this, rbit);
        }

        //// SIMULATOR ////

        @Override
        protected int busWidthWork ()
        {
            return nbits;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    // generate circuitry, ie, connector and level converters, for the raspberry pi
    // connect networks to its pins
    private void getCircuit (GenCtx genctx)
    {
        if (mqcpunets == null) {

            // connectors and holes always go in lower right corner of board
            moduleleftx = genctx.boardwidth - NetGen.CORNER10THS - 25;
            moduletopy  = genctx.boardheight - 25;

            // set up networks (circuit board traces) that the CPU uses to read memory data from the RasPi
            mqcpunets = new Network[nbits];
            for (int i = 0; i < nbits; i ++) {
                mqcpunets[i] = new Network (genctx, String.format ("mq%2d.%s", i, name));
            }

            // set up networks (circuit board traces) that the CPU uses to get these other signals from the RasPi
            NetClass v33class = new NetClass ("v33wires",
                    "\"3.3V from RasPi\"\n" +
                    "    (clearance 0.2)\n" +
                    "    (trace_width 0.5)\n" +
                    "    (via_dia 0.6)\n" +
                    "    (via_drill 0.4)\n" +
                    "    (uvia_dia 0.3)\n" +
                    "    (uvia_drill 0.1)");
            genctx.netclasses.put (v33class.name, v33class);
            v33net    = new Network (genctx, "v33."   + name, v33class);    // raspi provides 3.3V for pullup resistors
            clkcpunet = new Network (genctx, "clock." + name);              // raspi writes clock signal to this pin
            rescpunet = new Network (genctx, "reset." + name);              // raspi writes reset signal to this pin
            irqcpunet = new Network (genctx, "intrq." + name);              // raspi writes irq signal to this pin

            // get the networks that the CPU sends memory address and data to the RasPi on
            // - generates those circuits if not already generated
            mdcpunets = new Network[nbits];
            for (int i = 0; i < nbits; i ++) {
                genctx.addPropRoot (md, i);
                mdcpunets[i] = md.generate (genctx, i);
            }

            // likewise for other signals coming from the CPU going to the RasPi
            genctx.addPropRoot ( mread, 0);
            genctx.addPropRoot (mwrite, 0);
            genctx.addPropRoot ( mword, 0);
            genctx.addPropRoot (  halt, 0);
            mreadcpunet  =  mread.generate (genctx, 0);    // generate circuitry that drives the MREAD signal
            mwritecpunet = mwrite.generate (genctx, 0);    // generate circuitry that drives the MWRITE signal
            mwordcpunet  =  mword.generate (genctx, 0);    // generate circuitry that drives the MWORD signal
            haltcpunet   =   halt.generate (genctx, 0);    // generate circuitry that drives the HALT signal

            // make cells that drive uni-directional PI inputs without putting 5V on the RasPi
            // they assume the RasPi is programmed with internal pull-ups
            PiInCell mreadcell  = new PiInCell (genctx,  "mread." + name, mreadcpunet,  new Network (genctx,  "_mread3V." + name));
            PiInCell mwritecell = new PiInCell (genctx, "mwrite." + name, mwritecpunet, new Network (genctx, "_mwrite3V." + name));
            PiInCell mwordcell  = new PiInCell (genctx,  "mword." + name, mwordcpunet,  new Network (genctx,  "_mword3V." + name));
            PiInCell haltcell   = new PiInCell (genctx,   "halt." + name, haltcpunet,   new Network (genctx,   "_halt3V." + name));

            // make cells that read uni-directional PI outputs without putting 5V on the RasPi
            PiOutCell clockcell = new ClockCell (genctx, "clock." + name, clkcpunet, new Network (genctx, "_clock3V." + name));
            PiOutCell resetcell = new PiOutCell (genctx, "reset." + name, rescpunet, new Network (genctx, "_reset3V." + name));
            PiOutCell intrqcell = new PiOutCell (genctx, "intrq." + name, irqcpunet, new Network (genctx, "_intrq3V." + name));
            Network _denpinet = new Network (genctx, "_denab3V." + name);
            Network _qenpinet = new Network (genctx, "_qenab3V." + name);
            PiOutCell[] denacells = new PiOutCell[] {
                new PiOutCell (genctx, "dena0." + name, new Network (genctx, "dena0." + name), _denpinet),
                new PiOutCell (genctx, "dena1." + name, new Network (genctx, "dena1." + name), _denpinet)
            };
            PiOutCellFET[] qenacells = new PiOutCellFET[] {
                new PiOutCellFET (genctx, "qena0." + name, new Network (genctx, "qena0." + name), _qenpinet),
                new PiOutCellFET (genctx, "qena1." + name, new Network (genctx, "qena1." + name), _qenpinet)
            };

            // make a set of 3.3V bi-directional cells for the data networks
            // there aren't enough GPIO pins on the RasPi for separate mdbus and mqbus
            // ...so it has to be multiplexed with the wdata signal coming from the Pi
            PiBiCell[] bdcells = new PiBiCell[nbits];
            for (int i = 0; i < nbits; i ++) {
                bdcells[i] = new PiBiCell (genctx, i + ".m." + name,
                                denacells[i/8].cpunet, qenacells[i/8].cpunet, mdcpunets[i], mqcpunets[i]);
            }

            // make a connector object and wire up all the pins
            Comp.Conn connector = new Comp.Conn (genctx, genctx.conncol, name, 20, 2);
            connector.orien = Comp.Conn.Orien.OVER;

            // the PI doc has pins numbered:
            //            1  2
            //    B       3  4
            //    U       5  6
            //    L       7  8
            //    K       9 10
            //    O      11 12
            //    F      13 14
            //    B      15 16
            //    O      17 18
            //    A      19 20
            //    R      21 22
            //    D      23 24
            //           25 26
            //    F      27 28
            //    A      29 30
            //    C      31 32
            //    E      33 34
            //    U      35 36
            //    P      37 38
            //           39 40
            // [ETHERNET  USB]

            //  GPIO[19:04] = data[15:00]   35,12,11,36, 10, 8, 33,32, 23,19,21,24, 26,31,29, 7
            int[] mpins = new int[] { 35,12,11,36, 10, 8, 33,32, 23,19,21,24, 26,31,29, 7 };
            for (int i = 0; i < nbits; i ++) {
                bdcells[i]._pinet.addConn (connector, Integer.toString (mpins[15-i]));
            }

            //  GPIO[23]    = word          16
            //  GPIO[24]    = halt          18
            //  GPIO[25]    = read          22
            //  GPIO[26]    = write         37
             mwordcell._pinet.addConn (connector, "16");
              haltcell._pinet.addConn (connector, "18");
             mreadcell._pinet.addConn (connector, "22");
            mwritecell._pinet.addConn (connector, "37");

            //  GPIO[02]    = clock          3
            //  GPIO[03]    = reset          5
            //  GPIO[20]    = irq           38
            //  GPIO[21]    = qenab         40
            //  GPIO[22]    = denab         15
            clockcell._pinet.addConn (connector,  "3");
            resetcell._pinet.addConn (connector,  "5");
            intrqcell._pinet.addConn (connector, "38");
            _qenpinet.addConn (connector, "40");
            _denpinet.addConn (connector, "15");

            //  power supplied to raspi (+5V)  2,4
            Comp.Conn pwrjumper = new Comp.Conn (genctx, null, "pwr." + name, 2, 2);
            Network vccnet = genctx.nets.get ("VCC");
            vccnet.addConn (pwrjumper, "1");
            vccnet.addConn (pwrjumper, "2");
            NetClass v50class = new NetClass ("v50wires",
                    "\"5.0V to RasPi\"\n" +
                    "    (clearance 0.2)\n" +
                    "    (trace_width 0.75)\n" +
                    "    (via_dia 0.6)\n" +
                    "    (via_drill 0.4)\n" +
                    "    (uvia_dia 0.3)\n" +
                    "    (uvia_drill 0.1)");
            genctx.netclasses.put (v50class.name, v50class);
            Network pwr1net = new Network (genctx, "pwr1." + name, v50class);
            Network pwr2net = new Network (genctx, "pwr2." + name, v50class);
            pwr1net.addConn (pwrjumper, "3");
            pwr1net.addConn (connector, "4");
            pwr2net.addConn (pwrjumper, "4");
            pwr2net.addConn (connector, "2");

            //  pullup power from raspi (+3.3V)  1,17
            v33net.addConn (connector, "1");

            //  ground   6, 9,14,20,25,30,34,39
            Network gndnet = genctx.nets.get ("GND");
            gndnet.addConn (connector,  "6");
            gndnet.addConn (connector,  "9");
            gndnet.addConn (connector, "14");
            gndnet.addConn (connector, "20");
            gndnet.addConn (connector, "25");
            gndnet.addConn (connector, "30");
            gndnet.addConn (connector, "34");
            gndnet.addConn (connector, "39");

            // fill in unconnected pins so we get holes drilled on board
            for (int p = 1; p <= 40; p ++) {
                String s = Integer.toString (p);
                if (connector.getPinNetwork (s) == null) {
                    Network n = new Network (genctx, s + "-nc." + name);
                    n.addConn (connector, s);
                }
            }

            // position on circuit board
            connector.setPosXY (moduleleftx, moduletopy + 2);
            pwrjumper.setPosXY (moduleleftx - 3, moduletopy - 0.5);

            // place 4 mounting holes
            for (int cy = 0; cy < 2; cy ++) {
                for (int cx = 0; cx < 2; cx ++) {
                    Comp.Hole hole = new Comp.Hole (genctx, null, cx + "." + cy + "." + name);
                    hole.setPosXY (moduleleftx + 0.5 + cx * 19.5, moduletopy + cy * 23);
                }
            }
        }
    }

    //// SIMULATOR ////

    private byte[] memarray = new byte[65536];

    private boolean savemread;
    private boolean savemwrite;
    private boolean savemword;
    private int savemaddr;

    private void stepSimulator (int t)
            throws HaltedException
    {
        // raspictl -sim (driver/pipelib.cc) being used to generate gpio output
        // use the provided values instead of computing values internally
        if (forcev != 0) {
            clk.setSimOutput (t, (forcev >>  2) & 0x000100000001L);     // gpio[02]
            res.setSimOutput (t, (forcev >>  3) & 0x000100000001L);     // gpio[03]
             mq.setSimOutput (t, (forcev >>  4) & 0xFFFF0000FFFFL);     // gpio[19:04]
            irq.setSimOutput (t, (forcev >> 20) & 0x000100000001L);     // gpio[20]
            return;
        }

        // add extra 2 gate delays for 6-transistor configuration
        // ie, the input gates see inputs 2 more gate delays ago
        int t2 = t - 2 * OpndRhs.SIM_PROP;

        // get state all up front in case of recursion
        // this works the CPU circuits to come up with these values
        long mdval     = md.getSimInput (t2);       // address or data it is sending us
        long mreadval  = mread.getSimInput (t2);    // it wants to read memory
        long mwordval  = mword.getSimInput (t2);    // it wants to access a word (not a byte)
        long mwriteval = mwrite.getSimInput (t2);   // it wants to write memory
        long haltval   = halt.getSimInput (t2);     // it has halted

        long mqval     = mq.getSimOutput (t - 1);   // also get our previous output value

        int nsintoclk  = t * OpndRhs.SIM_STEPNS % clkns;

        // we don't request interrupts for now
        irq.setSimOutput (t, intreq ? 0x100000000L : 1L);

        // take care of clocking
        // reset for a half clock period to begin with
        if (t * OpndRhs.SIM_STEPNS < clkns / 2) {
            if (t == 0) shadow.reset ();
            res.setSimOutput (t, 0x100000000L);
            clk.setSimOutput (t, 1L);
        } else {
            res.setSimOutput (t, 1L);

            if (haltval == 0x100000000L) {
                throw new HaltedException ();
            }

            boolean clkup = (nsintoclk < clkns / 2);
            clk.setSimOutput (t, clkup ? 0x100000000L : 1L);

            // see if just going clkup
            if (nsintoclk == 0) {

                if (shaden) {

                    // check all input from CPU
                    shadow.check (
                            mreadval  == 0x100000000L,
                            mwordval  == 0x100000000L,
                            mwriteval == 0x100000000L,
                            (short) (mdval >> 32));

                    // tell shadow we are starting new clock cycle
                    shadow.clock (t * OpndRhs.SIM_STEPNS, (short) (mqval >> 32), intreq);
                }

                // if last cycle said to write memory, it gave us the address
                // this cycle it is giving us the data to be written
                if (savemwrite) {
                    int mask = savemword ? 0xFFFF : 0xFF;
                    if ((mwordval == 0) || (((mdval | (mdval >>> 32)) & mask) != mask)) {
                        error (t, name + " raspi memdata setup/hold violation");
                    }
                    int savemdata = (int) (mdval >>> 32);
                    memarray[savemaddr] = (byte) savemdata;
                    if (savemword) {
                        memarray[savemaddr^1] = (byte) (savemdata >> 8);
                    }
                    savemwrite = false;
                    if (savemaddr == MAGIC) {
                        domagic (t);
                    }
                }

                // if this cycle has read or write set, it has sent us the address to read or write
                // ...along with saying if it is a byte or word transfer
                if ((mreadval == 0) || (mwriteval == 0)) {
                    error (t, name + " raspi memread/memwrite setup/hold violation");
                }
                if (((mreadval | mwriteval) & 0x100000000L) != 0) {
                    if ((mwordval == 0) || (((mdval | (mdval >>> 32)) & 0xFFFF) != 0xFFFF)) {
                        error (t, name + " raspi memword/memaddr setup/hold violation");
                    }
                    savemread  = (mreadval  & 0x100000000L) != 0;   // cpu wants to read memory
                    savemwrite = (mwriteval & 0x100000000L) != 0;   // cpu wants to write memory
                    savemword  = (mwordval  & 0x100000000L) != 0;   // cpu wants a word transfer (else byte)
                    savemaddr  = (int) (mdval >>> 32);              // cpu is giving us the address
                    if (savemword && ((savemaddr & 1) != 0)) {
                        error (t, name + " raspi odd address word access " + Integer.toHexString (savemaddr));
                    }
                }
            }

            // quarter way into cycle clears outgoing read data
            if (nsintoclk == clkns / 4) {
                mqval = 0;
            }

            // if just going clock down, execute the memory read request
            if (savemread & (nsintoclk == clkns / 2)) {
                if (savemword) {
                    mqval = readword (savemaddr);
                    mqval = (mqval << 32) | (mqval ^ 0xFFFF);
                } else {
                    mqval = (memarray[savemaddr] & 0xFF);
                    mqval = (mqval << 32) | (mqval ^ 0xFF);  // leave top bits 'undefined'
                }
                savemread = false;
            }
        }

        // output data update or maintain previous value
        mq.setSimOutput (t, mqval);
    }

    private void domagic (int t)
    {
        int sp = readword (MAGIC);
        int mi = readword (sp);
        switch (mi) {
            case M_EXIT: {
                int code = readword (sp + 2);
                System.out.println ("M_EXIT:" + code);
                System.exit (code);
                break;
            }
            case M_PRINTLN: {
                int msg = readword (sp + 2);
                StringBuilder sb = new StringBuilder ();
                sb.append ("M_PRINTLN:");
                while (true) {
                    char c = (char) (memarray[msg] & 0xFF);
                    if (c == 0) break;
                    sb.append (c);
                    msg = (msg + 1) & 0xFFFF;
                }
                System.out.println (sb);
                break;
            }
            case M_INTREQ: {
                intreq = readword (sp + 2) != 0;
                System.out.println ("M_INTREQ:" + intreq);
                break;
            }
            default: {
                error (t, "bad magic code " + mi);
                break;
            }
        }
    }

    private int readword (int addr)
    {
        return ((memarray[addr^1] & 0xFF) << 8) | (memarray[addr] & 0xFF);
    }

    private static void error (int t, String msg)
    {
        StringBuilder sb = new StringBuilder ();
        sb.append (t * OpndRhs.SIM_STEPNS);
        sb.append (' ');
        sb.append (msg);
        System.err.println (sb);
    }

    // process 'raspi' sim script command
    public static String simCmd (Module module, String instname, String[] args, int simtime)
            throws Exception
    {
        OpndLhs clkvar = module.variables.get ("CLK/" + instname);
        if (! (clkvar instanceof CLKOutput)) {
            throw new Exception ("no raspi instance " + instname + " found");
        }
        CLKOutput clkout = (CLKOutput) clkvar;
        RasPiModule zhis = clkout.getRasPi ();

        switch (args[0]) {
            case "dump": {
                if (args.length != 3) {
                    throw new Exception ("missing <addr> <count> in raspi dump command");
                }
                int addr  = Integer.parseInt (args[1], 16);
                int count = Integer.parseInt (args[2], 16);
                StringBuilder sb = new StringBuilder (count * 3);
                while (-- count >= 0) {
                    if (sb.length () > 0) sb.append (' ');
                    sb.append (Integer.toHexString (zhis.memarray[addr++] & 0xFF).toUpperCase ());
                }
                return sb.toString ();
            }

            // used by driver/pipelib.cc to read gpio input from simulated circuitry
            case "examine": {
                StringBuilder sb = new StringBuilder (27);
                appendsim (sb, simtime, zhis.mwrite);    // cpu telling raspi to write memory
                appendsim (sb, simtime, zhis.mread);     // cpu telling raspi to read memory
                appendsim (sb, simtime, zhis.halt);      // cpu telling raspi that cpu is halted
                appendsim (sb, simtime, zhis.mword);     // cpu telling raspi to transfer word not byte
                appendsim (sb, 2, zhis.forcev >> 21);    // echo dena[22],qena[21] back
                appendsim (sb, simtime, zhis.irq);       // echo irq back
                if (((zhis.forcev >> (21 + 32)) & 1) != 0) {
                    appendsim (sb, simtime, zhis.mq);    // cpu passing raspi mem addr or data
                } else {
                    appendsim (sb, simtime, zhis.md);    // echo md back
                }
                appendsim (sb, simtime, zhis.res);       // echo res back
                appendsim (sb, simtime, zhis.clk);       // echo clk back
                appendsim (sb, 2, zhis.forcev);          // echo unused back
                return sb.toString ();
            }

            // used by driver/pipelib.cc to write gpio output to simulated circuitry
            case "force": {
                switch (args.length) {
                    case 1: {   // force                turns forcing off
                        zhis.forcev = 0;
                        break;
                    }
                    case 2: {   // force <hexvalue>     turns forcing on
                        int f = Integer.parseInt (args[1], 16) & 0x07FFFFFC;
                        zhis.forcev = (((long) f) << 32) | (f ^ 0x07FFFFFC);
                        break;
                    }
                    default: throw new Exception ("force or force <value>");
                }
                break;
            }

            case "load": {
                if (args.length < 3) {
                    throw new Exception ("missing <addr> <bytes...> in raspi load command");
                }
                int addr = Integer.parseInt (args[1], 16);
                for (int i = 2; i < args.length; i ++) {
                    zhis.memarray[addr++] = (byte) Integer.parseInt (args[i], 16);
                }
                break;
            }

            case "printstate": {
                zhis.shadow.printstate = true;
                break;
            }

            case "shadow": {
                if (args.length != 2) {
                    throw new Exception ("missing <enable/disable>");
                }
                switch (args[1]) {
                    case "disable": { zhis.shaden = false; break; }
                    case "enable":  { zhis.shaden = true;  break; }
                    default: throw new Exception ("bad <enable/disable> " + args[1]);
                }
                break;
            }

            default: {
                throw new Exception ("unknown raspi command " + args[0]);
            }
        }
        return null;
    }

    // append binary digits from var onto the string
    private static void appendsim (StringBuilder sb, int simtime, OpndLhs var)
            throws HaltedException
    {
        long bits = var.getSimOutput (simtime - 1);
        appendsim (sb, var.hidim - var.lodim + 1, bits);
    }

    // append binary digits from bits onto the string
    private static void appendsim (StringBuilder sb, int n, long bits)
    {
        for (int i = n; -- i >= 0;) {
            if (((bits >> (i + 32)) & 1) != 0) sb.append ('1');
              else if (((bits >> i) & 1) != 0) sb.append ('0');
                                          else sb.append ('x');
        }
    }

    // contains components for a single pin's level converter
    private class PiCell extends Placeable {
        public Network _pinet;          // connects to raspi connector signal pin

        protected Comp[][] columns;     // various parts arranged in columns
        protected double[][] colxofs;
        protected double[][] colyofs;
        protected String name;

        private GenCtx genctx;

        protected PiCell (GenCtx genctx, String name)
        {
            this.name = name;
            this.genctx = genctx;
            genctx.addPlaceable (this);
        }

        @Override  // Placeable
        public String getName ()
        {
            return name;
        }

        @Override  // Placeable
        public void flipIt ()
        { }

        @Override  // Placeable
        public int getWidth ()
        {
            int totalwidth = 0;
            for (Comp[] column : columns) {
                int colwidth = 0;
                for (Comp comp : column) {
                    int w = (comp == null) ? Comp.Resis.WIDTH : comp.getWidth ();
                    if (colwidth < w) colwidth = w;
                }
                totalwidth += colwidth;
            }
            return totalwidth;
        }

        @Override  // Placeable
        public int getHeight ()
        {
            int totalheight = 0;
            for (Comp[] column : columns) {
                int colheight = 0;
                for (Comp comp : column) {
                    colheight += (comp == null) ? Comp.Resis.HEIGHT : comp.getHeight ();
                }
                if (totalheight < colheight) totalheight = colheight;
            }
            return totalheight;
        }

        private double saveleftx, savetopy;

        @Override  // Placeable
        public void setPosXY (double leftx, double topy)
        {
            saveleftx = leftx;
            savetopy  = topy;

            for (int i = 0; i < columns.length; i ++) {
                Comp[] column = columns[i];
                double y = topy;
                int colwidth = 0;
                for (int j = 0; j < column.length; j ++) {
                    Comp comp = column[j];
                    if (comp != null) {
                        double xof = (colxofs == null) ? 0 : colxofs[i][j];
                        double yof = (colyofs == null) ? 0 : colyofs[i][j];
                        comp.setPosXY (leftx + xof, y + yof);
                        if (comp.name.startsWith ("Q")) {
                            Comp bycap = genctx.comps.get ("BC" + comp.name.substring (1));
                            bycap.setPosXY (leftx + xof, y + yof);
                        }
                    }
                    y += (comp == null) ? Comp.Resis.HEIGHT : comp.getHeight ();
                    int w = (comp == null) ? Comp.Resis.WIDTH : comp.getWidth ();
                    if (colwidth < w) colwidth = w;
                }
                leftx += colwidth;
            }
        }

        @Override  // Placeable
        public double getPosX ()
        {
            return saveleftx;
        }

        @Override  // Placeable
        public double getPosY ()
        {
            return savetopy;
        }

        @Override  // Placeable
        public Network[] getExtNets ()
        {
            Network[] extnets = new Network[nbits*2+7];

            int j = 0;
            for (Network mqcpunet : mqcpunets) {
                extnets[j++] = mqcpunet;
            }
            extnets[j++] = clkcpunet;
            extnets[j++] = rescpunet;
            extnets[j++] = irqcpunet;
            for (Network mdcpunet : mdcpunets) {
                extnets[j++] = mdcpunet;
            }
            extnets[j++] = mreadcpunet;
            extnets[j++] = mwritecpunet;
            extnets[j++] = mwordcpunet;
            extnets[j++] = haltcpunet;

            return extnets;
        }
    }

    private final static double[][] piincolxofs = new double[][] {
        new double[] { 0, 0 },
        new double[] { 0, 0 },
        new double[] { 1, 0, 0 }    // push the rcoll resistor over 0.1 inch
    };

    private final static double[][] piincolyofs = new double[][] {
        new double[] { 0, 0 },
        new double[] { 0, 0 },
        new double[] { 0, -1, -1 }  // lift the transistor and base resistor up 0.1 inch
    };

    /**
     * Cell for signal coming from CPU to RasPi input
     * Like standard inverter cell but with 3.3V pullup
     *
                             (VCC)                                             (V33)
                               |                                                 |
                               _                                                 _
                             orres                                             rcoll
                               _                                                 _
                               |                                                 |
                               |                                              (_pinet)
                               |                                                 |
                               |                                                /
       (cpunet)----[|<]----(andnet)----[>|]----(ornet)----[>|]----(basenet)----|
                   andio               ordio             dbase        |         \
                                                                      _          |
                                                                    rbase      (GND)
                                                                      _
                                                                      |
                                                                    (GND)
     */
    private class PiInCell extends PiCell implements PreDrawable {
        private Comp.ByCap bycap;
        private Comp.Diode andio;
        private Comp.Diode ordio;
        private Comp.Resis orres;
        private Comp.Diode dbase;
        private Comp.Trans trans;
        private Comp.Resis rbase;
        private Comp.Resis rcoll;

        private Network andnet;
        private Network ornet;
        private Network basenet;

        public PiInCell (GenCtx genctx, String name, Network cpunet, Network _pinet)
        {
            super (genctx, name);

            this._pinet = _pinet;

            andio = new Comp.Diode (genctx, null, "D3." + name, false, true);
            ordio = new Comp.Diode (genctx, null, "D2." + name, true, true);
            orres = new Comp.Resis (genctx, null, "R1." + name, Comp.Resis.R_OR, true, true);
            dbase = new Comp.Diode (genctx, null, "D1." + name, true, true);
            trans = new Comp.Trans (genctx, null, "Q."  + name);
            bycap = new Comp.ByCap (genctx, null, "BC." + name);
            rbase = new Comp.Resis (genctx, null, "RB." + name, Comp.Resis.R_BAS, false, true);
            rcoll = new Comp.Resis (genctx, null, "R2." + name, Comp.Resis.R_RPC, true, true);

            columns = new Comp[][] {
                new Comp[] { andio, ordio },
                new Comp[] { orres, dbase },
                new Comp[] { rcoll, trans, rbase }
            };

            colxofs = piincolxofs;
            colyofs = piincolyofs;

            andnet  = new Network (genctx, "A." + name);
            ornet   = new Network (genctx, "O." + name);
            basenet = new Network (genctx, "B." + name);
            Network gndnet = genctx.nets.get ("GND");
            Network vccnet = genctx.nets.get ("VCC");

            cpunet.addConn  (andio, Comp.Diode.PIN_C, 1);
            andnet.addConn  (andio, Comp.Diode.PIN_A);
            andnet.addConn  (orres, Comp.Resis.PIN_A);
            andnet.addConn  (ordio, Comp.Diode.PIN_A);
            ornet.addConn   (dbase, Comp.Diode.PIN_A);
            ornet.addConn   (ordio, Comp.Diode.PIN_C);
            basenet.addConn (trans, Comp.Trans.PIN_B);
            basenet.addConn (dbase, Comp.Diode.PIN_C);
            basenet.addConn (rbase, Comp.Resis.PIN_C);
            _pinet.addConn  (trans, Comp.Trans.PIN_C);
            _pinet.addConn  (rcoll, Comp.Resis.PIN_A);

            v33net.addConn  (rcoll, Comp.Resis.PIN_C);
            v33net.addConn  (bycap, Comp.ByCap.PIN_Y);
            vccnet.addConn  (orres, Comp.Resis.PIN_C);
            gndnet.addConn  (trans, Comp.Trans.PIN_E);
            gndnet.addConn  (bycap, Comp.ByCap.PIN_Z);
            gndnet.addConn  (rbase, Comp.Resis.PIN_A);

            genctx.predrawables.add (this);
        }

        @Override  // PiCell
        public int getHeight ()
        {
            return 8;
        }

        @Override  // PreDrawable
        public void preDraw (PrintStream ps)
        {
            double orresax = orres.pcbPosXPin (Comp.Resis.PIN_A);
            double orresay = orres.pcbPosYPin (Comp.Resis.PIN_A);
            double andioax = andio.pcbPosXPin (Comp.Diode.PIN_A);
            double andioay = andio.pcbPosYPin (Comp.Diode.PIN_A);
            double ordioax = ordio.pcbPosXPin (Comp.Diode.PIN_A);
            double ordioay = ordio.pcbPosYPin (Comp.Diode.PIN_A);
            double ordiocx = ordio.pcbPosXPin (Comp.Diode.PIN_C);
            double ordiocy = ordio.pcbPosYPin (Comp.Diode.PIN_C);
            double dbaseax = dbase.pcbPosXPin (Comp.Diode.PIN_A);
            double dbaseay = dbase.pcbPosYPin (Comp.Diode.PIN_A);
            double dbasecx = dbase.pcbPosXPin (Comp.Diode.PIN_C);
            double dbasecy = dbase.pcbPosYPin (Comp.Diode.PIN_C);
            double transbx = trans.pcbPosXPin (Comp.Trans.PIN_B);
            double transby = trans.pcbPosYPin (Comp.Trans.PIN_B);

            andnet.putseg (ps, orresax,       orresay,       andioax,       andioay,       "F.Cu");
            andnet.putseg (ps, andioax,       andioay,       andioax - 0.5, andioay + 0.5, "B.Cu");
            andnet.putseg (ps, andioax - 0.5, andioay + 0.5, andioax - 0.5, ordioay - 0.5, "B.Cu");
            andnet.putvia (ps, andioax - 0.5, ordioay - 0.5, "B.Cu", "F.Cu");
            andnet.putseg (ps, andioax - 0.5, ordioay - 0.5, ordioax + 0.5, ordioay - 0.5, "F.Cu");
            andnet.putseg (ps, ordioax + 0.5, ordioay - 0.5, ordioax,       ordioay,       "F.Cu");

            ornet.putseg (ps, ordiocx, ordiocy, dbaseax, dbaseay, "F.Cu");

            double rbasecx = rbase.pcbPosXPin (Comp.Resis.PIN_C);
            double rbasecy = rbase.pcbPosYPin (Comp.Resis.PIN_C);

            basenet.putseg (ps, dbasecx, dbasecy, transbx, transby, "F.Cu");
            basenet.putseg (ps, transbx, transby, rbasecx, rbasecy, "B.Cu");

            double rcollax = rcoll.pcbPosXPin (Comp.Resis.PIN_A);
            double rcollay = rcoll.pcbPosYPin (Comp.Resis.PIN_A);
            double rcollcx = rcoll.pcbPosXPin (Comp.Resis.PIN_C);
            double rcollcy = rcoll.pcbPosYPin (Comp.Resis.PIN_C);
            double transcx = trans.pcbPosXPin (Comp.Trans.PIN_C);
            double transcy = trans.pcbPosYPin (Comp.Trans.PIN_C);
            double bycapyx = bycap.pcbPosXPin (Comp.ByCap.PIN_Y);
            double bycapyy = bycap.pcbPosYPin (Comp.ByCap.PIN_Y);

            _pinet.putseg (ps, rcollax, rcollay, transcx, transcy, "B.Cu");

            assert rcollcx == bycapyx + 1.0;
            v33net.putseg (ps, rcollcx,       rcollcy,       rcollcx - 0.5, rcollcy + 0.5, "B.Cu");
            v33net.putseg (ps, rcollcx - 0.5, rcollcy + 0.5, bycapyx + 0.5, bycapyy - 0.5, "B.Cu");
            v33net.putseg (ps, bycapyx + 0.5, bycapyy - 0.5, bycapyx,       bycapyy,       "B.Cu");
        }
    }

    /**
     * Cell for signal coming from RasPi output to CPU
     * Just a resistor from RasPi to transistor base without diodes or pullup
     * Cuz we don't want pullup putting 5V into RasPi and no diodes cuz the
     * Transistor is only driven by one input line
     *
                                  (VCC)
                                    |
                                    _
                                   rout
                                    _
                                    |
                                 (cpunet)
                                    |
                                   /
        (_pinet)-------[rin]--+---|
                              |    \
                              _     |
                            rbas  (GND)
                              _
                              |
                            (GND)
     */
    private class PiOutCell extends PiCell {
        public Network cpunet;

        public PiOutCell (GenCtx genctx, String name, Network cpunet, Network _pinet)
        {
            super (genctx, name);

            this.cpunet = cpunet;

            Comp.Resis rin  = new Comp.Resis (genctx, null, "R1." + name, Comp.Resis.R_RPB, false, true);
            Comp.Trans trn  = new Comp.Trans (genctx, null, "Q."  + name);
            Comp.ByCap byc  = new Comp.ByCap (genctx, null, "BC." + name);
            Comp.Resis rout = new Comp.Resis (genctx, null, "R2." + name, Comp.Resis.R_COL, false, true);
            Comp.Resis rbas = new Comp.Resis (genctx, null, "R3." + name, "1.8K", true, true);

            columns = new Comp[][] {
                new Comp[] { rout, rin },
                new Comp[] { trn, rbas }
            };

            this._pinet = _pinet;
            _pinet.addConn (rin, Comp.Resis.PIN_C);

            Network brnet = new Network (genctx, "B." + name);
            brnet.preWire  (trn, Comp.Trans.PIN_B, rbas, Comp.Resis.PIN_A);
            brnet.preWire  (rin, Comp.Resis.PIN_A, rbas, Comp.Resis.PIN_A);

            cpunet.preWire (trn, Comp.Trans.PIN_C, rout, Comp.Resis.PIN_A);

            Network gndnet = genctx.nets.get ("GND");
            Network vccnet = genctx.nets.get ("VCC");

            gndnet.addConn (trn,  Comp.Trans.PIN_E);
            gndnet.addConn (byc,  Comp.ByCap.PIN_Z);
            gndnet.addConn (rbas, Comp.Resis.PIN_C);
            vccnet.addConn (byc,  Comp.ByCap.PIN_Y);
            vccnet.addConn (rout, Comp.Resis.PIN_C);
        }
    }

    /**
     * Cell for signal coming from RasPi output to CPU
     * Use a MOSFET with no gate resistor so RasPi can drive it directly
     *
                                  (VCC)
                                    |
                                    _
                                   rout
                                    _
                                    |
                                 (cpunet)
                                    |
                                   /
        (_pinet)--------------+---|
                              |    \
                              _     |
                            rbas  (GND)
                              _
                              |
                            (GND)
     */
    private class PiOutCellFET extends PiCell {
        public Network cpunet;

        public PiOutCellFET (GenCtx genctx, String name, Network cpunet, Network _pinet)
        {
            super (genctx, name);

            this.cpunet = cpunet;

            Comp.Trans trn  = new Comp.Trans (genctx, null, "Q."  + name, "2N7000");
            Comp.ByCap byc  = new Comp.ByCap (genctx, null, "BC." + name);
            Comp.Resis rout = new Comp.Resis (genctx, null, "R2." + name, Comp.Resis.R_COL, false, true);
            Comp.Resis rbas = new Comp.Resis (genctx, null, "R3." + name, "1.8K", true, true);

            columns = new Comp[][] {
                new Comp[] { rout },
                new Comp[] { trn, rbas }
            };

            this._pinet = _pinet;
            _pinet.addConn (trn, Comp.Trans.PIN_B);

            cpunet.preWire (trn, Comp.Trans.PIN_C, rout, Comp.Resis.PIN_A);
            _pinet.preWire (trn, Comp.Trans.PIN_B, rbas, Comp.Resis.PIN_A);

            Network gndnet = genctx.nets.get ("GND");
            Network vccnet = genctx.nets.get ("VCC");

            gndnet.addConn (trn,  Comp.Trans.PIN_E);
            gndnet.addConn (byc,  Comp.ByCap.PIN_Z);
            gndnet.addConn (rbas, Comp.Resis.PIN_C);
            vccnet.addConn (byc,  Comp.ByCap.PIN_Y);
            vccnet.addConn (rout, Comp.Resis.PIN_C);
        }
    }

    // make out cell for clock line special just so we can output copper exclusion zones
    // ... so an hole for heatsink can be drilled in board
    private class ClockCell extends PiOutCell implements PreDrawable {
        public ClockCell (GenCtx genctx, String name, Network cpunet, Network _pinet)
        {
            super (genctx, name, cpunet, _pinet);
            genctx.predrawables.add (this);
        }

        @Override  // PreDrawable
        public void preDraw (PrintStream ps)
        {
            String leftmm = String.format ("%4.2f", (moduleleftx +  4.0) * 2.54);
            String ritemm = String.format ("%4.2f", (moduleleftx + 19.5) * 2.54);
            String topmm  = String.format ("%4.2f", (moduletopy  +  2.5) * 2.54);
            String botmm  = String.format ("%4.2f", (moduletopy  + 22.5) * 2.54);
            String[] layers = new String[] { "F.Cu", "In1.Cu", "In2.Cu", "B.Cu" };
            for (String layer : layers) {
                String tstamp = String.format ("%08X", layer.hashCode ());
                ps.println ("  (zone (net 0) (net_name \"\") (layer " + layer + ") (tstamp " + tstamp + ") (hatch edge 0.508)");
                ps.println ("     (connect_pads (clearance 0.508))");
                ps.println ("     (min_thickness 0.254)");
                ps.println ("     (keepout (tracks not_allowed) (vias not_allowed) (copperpour not_allowed))");
                ps.println ("     (fill (arc_segments 16) (thermal_gap 0.508) (thermal_bridge_width 0.508))");
                ps.println ("     (polygon");
                ps.println ("       (pts");
                ps.println ("         (xy " + leftmm + " " + topmm + ")");
                ps.println ("         (xy " + ritemm + " " + topmm + ")");
                ps.println ("         (xy " + ritemm + " " + botmm + ")");
                ps.println ("         (xy " + leftmm + " " + botmm + ")");
                ps.println ("       )");
                ps.println ("     )");
                ps.println ("  )");
            }

            leftmm = String.format ("%4.2f", (moduleleftx +  5.0) * 2.54);
            ritemm = String.format ("%4.2f", (moduleleftx + 18.5) * 2.54);
            topmm  = String.format ("%4.2f", (moduletopy  +  3.5) * 2.54);
            botmm  = String.format ("%4.2f", (moduletopy  + 21.5) * 2.54);
            ps.println ("  (gr_line (start " + leftmm + " " + topmm + ") (end " + ritemm + " " + topmm + ") (angle 90) (layer Edge.Cuts) (width 0.1))");
            ps.println ("  (gr_line (start " + ritemm + " " + topmm + ") (end " + ritemm + " " + botmm + ") (angle 90) (layer Edge.Cuts) (width 0.1))");
            ps.println ("  (gr_line (start " + ritemm + " " + botmm + ") (end " + leftmm + " " + botmm + ") (angle 90) (layer Edge.Cuts) (width 0.1))");
            ps.println ("  (gr_line (start " + leftmm + " " + botmm + ") (end " + leftmm + " " + topmm + ") (angle 90) (layer Edge.Cuts) (width 0.1))");
            ps.println ("  (gr_line (start " + leftmm + " " + topmm + ") (end " + ritemm + " " + topmm + ") (angle 90) (layer F.SilkS) (width 0.2))");
            ps.println ("  (gr_line (start " + ritemm + " " + topmm + ") (end " + ritemm + " " + botmm + ") (angle 90) (layer F.SilkS) (width 0.2))");
            ps.println ("  (gr_line (start " + ritemm + " " + botmm + ") (end " + leftmm + " " + botmm + ") (angle 90) (layer F.SilkS) (width 0.2))");
            ps.println ("  (gr_line (start " + leftmm + " " + botmm + ") (end " + leftmm + " " + topmm + ") (angle 90) (layer F.SilkS) (width 0.2))");

            String midxmm = String.format ("%4.2f", (moduleleftx + 23.5/2) * 2.54);
            String midymm = String.format ("%4.2f", (moduletopy  + 25.0/2) * 2.54);
            ps.println ("  (gr_text \"CUT-OUT\" (at " + midxmm + " " + midymm + ") (layer F.SilkS)");
            ps.println ("    (effects (font (size 2.5 2.5) (thickness 0.3)))");
            ps.println ("  )");
        }
    }

    /**
     * BiDirectional data line
     *                                                                                               (VCC)
                             (VCC)                                             (V33)                   |
                               |                                                 |                     _
                               _                                                 _                   rocol
                             orres                                             rcoll                   _
                               _                                                 _                     |--------(mqnet)
                               |                                                 |                    /
                               |                                              (_pinet)----[robas]-+--|  otran
                               |                                                 |                |   \
                               |                                                /                 _    |
        (mdnet)----[|<]----(andnet)----[>|]----(ornet)----[>|]----(basenet)----|  trans         roshn  |
                   mddio       |       ordio             dbase        |         \                 _    |
                               |                                      _          |                |    |
                               |                                    rbase      (GND)             (qenanet)
        (wdnet)----[|<]--------+                                      _
                   wddio                                              |
                                                                    (GND)
     */
    private class PiBiCell extends PiCell implements PreDrawable {
        private Comp.Diode wddio;       // MDENA from raspi
        private Comp.Diode mddio;       // MD[n] from CPU
        private Comp.Diode ordio;
        private Comp.Diode dbase;
        private Comp.Resis orres;
        private Comp.Trans trans;
        private Comp.ByCap bycap;
        private Comp.Resis rbase;
        private Comp.Resis rcoll;

        private Comp.Resis robas;
        private Comp.Resis rocol;
        private Comp.Resis roshn;
        private Comp.Trans otran;
        private Comp.ByCap obycp;

        private Network andnet;
        private Network ornet;
        private Network basenet;
        private Network rcvnet;
        private Network mqcpunet;

        public PiBiCell (GenCtx genctx, String name, Network denacpunet, Network qenacpunet, Network mdcpunet, Network mqcpunet)
        {
            super (genctx, name);

            this.mqcpunet = mqcpunet;

            wddio = new Comp.Diode (genctx, null, "D4." + name, false, true);
            mddio = new Comp.Diode (genctx, null, "D3." + name, false, true);
            ordio = new Comp.Diode (genctx, null, "D2." + name, true, true);
            dbase = new Comp.Diode (genctx, null, "D1." + name, true, true);
            orres = new Comp.Resis (genctx, null, "R1." + name, Comp.Resis.R_OR, true, true);
            trans = new Comp.Trans (genctx, null, "Q1." + name);
            bycap = new Comp.ByCap (genctx, null, "BC1." + name);
            rbase = new Comp.Resis (genctx, null, "RB." + name, Comp.Resis.R_BAS, false, true);
            rcoll = new Comp.Resis (genctx, null, "R2." + name, Comp.Resis.R_RPC, false, true);

            robas = new Comp.Resis (genctx, null, "R3." + name, Comp.Resis.R_RPB, true, true);
            rocol = new Comp.Resis (genctx, null, "R4." + name, Comp.Resis.R_COL, false, true);
            roshn = new Comp.Resis (genctx, null, "R5." + name, "1.8K", true, true);
            otran = new Comp.Trans (genctx, null, "Q2." + name);
            obycp = new Comp.ByCap (genctx, null, "BC2." + name);

            columns = new Comp[][] {
                new Comp[] { wddio, mddio },
                new Comp[] { orres, ordio },
                new Comp[] { rcoll, dbase },
                new Comp[] { trans, rbase },
                new Comp[] { rocol, robas },
                new Comp[] { otran, roshn }
            };

            andnet  = new Network (genctx, "A." + name);
            ornet   = new Network (genctx, "O." + name);
            basenet = new Network (genctx, "B." + name);
            _pinet  = new Network (genctx,  "_" + name);
            rcvnet  = new Network (genctx, "R." + name);
            Network gndnet = genctx.nets.get ("GND");
            Network vccnet = genctx.nets.get ("VCC");

            denacpunet.addConn (wddio, Comp.Diode.PIN_C, 1);
            mdcpunet.addConn   (mddio, Comp.Diode.PIN_C, 1);
            vccnet.addConn     (orres, Comp.Resis.PIN_C);

            andnet.addConn  (mddio, Comp.Diode.PIN_A);
            andnet.addConn  (wddio, Comp.Diode.PIN_A);
            andnet.addConn  (orres, Comp.Resis.PIN_A);
            andnet.addConn  (ordio, Comp.Diode.PIN_A);

            ornet.addConn   (ordio, Comp.Diode.PIN_C);
            ornet.addConn   (dbase, Comp.Diode.PIN_A);
            basenet.addConn (dbase, Comp.Diode.PIN_C);
            basenet.addConn (trans, Comp.Trans.PIN_B);
            basenet.addConn (rbase, Comp.Resis.PIN_C);
            _pinet.addConn  (trans, Comp.Trans.PIN_C);
            _pinet.addConn  (rcoll, Comp.Resis.PIN_A);

            v33net.addConn  (rcoll, Comp.Resis.PIN_C);
            v33net.addConn  (bycap, Comp.ByCap.PIN_Y);
            gndnet.addConn  (trans, Comp.Trans.PIN_E);
            gndnet.addConn  (bycap, Comp.ByCap.PIN_Z);
            gndnet.addConn  (rbase, Comp.Resis.PIN_A);

            _pinet.addConn (robas, Comp.Resis.PIN_A);
            rcvnet.addConn (robas, Comp.Resis.PIN_C);
            rcvnet.addConn (otran, Comp.Trans.PIN_B);
            rcvnet.addConn (roshn, Comp.Resis.PIN_A);
            mqcpunet.addConn (rocol, Comp.Resis.PIN_A);
            mqcpunet.addConn (otran, Comp.Trans.PIN_C);
            qenacpunet.addConn (otran, Comp.Trans.PIN_E);
            qenacpunet.addConn (roshn, Comp.Resis.PIN_C);

            vccnet.addConn (rocol, Comp.Resis.PIN_C);
            vccnet.addConn (obycp, Comp.ByCap.PIN_Y);
            gndnet.addConn (obycp, Comp.ByCap.PIN_Z);

            genctx.predrawables.add (this);
        }

        @Override  // PreDrawable
        public void preDraw (PrintStream ps)
        {
            double orresax = orres.pcbPosXPin (Comp.Resis.PIN_A);
            double orresay = orres.pcbPosYPin (Comp.Resis.PIN_A);
            double ordioax = ordio.pcbPosXPin (Comp.Diode.PIN_A);
            double ordioay = ordio.pcbPosYPin (Comp.Diode.PIN_A);
            double mddioax = mddio.pcbPosXPin (Comp.Diode.PIN_A);
            double mddioay = mddio.pcbPosYPin (Comp.Diode.PIN_A);
            double wddioax = wddio.pcbPosXPin (Comp.Diode.PIN_A);
            double wddioay = wddio.pcbPosYPin (Comp.Diode.PIN_A);
            andnet.putseg (ps, wddioax, wddioay, mddioax, mddioay, "B.Cu");
            andnet.putseg (ps, mddioax, mddioay, ordioax, ordioay, "F.Cu");
            andnet.putseg (ps, ordioax, ordioay, orresax, orresay, "B.Cu");

            double ordiocx = ordio.pcbPosXPin (Comp.Diode.PIN_C);
            double ordiocy = ordio.pcbPosYPin (Comp.Diode.PIN_C);
            double dbaseax = dbase.pcbPosXPin (Comp.Diode.PIN_A);
            double dbaseay = dbase.pcbPosYPin (Comp.Diode.PIN_A);
            ornet.putseg (ps, ordiocx, ordiocy, dbaseax, dbaseay, "F.Cu");

            double dbasecx = dbase.pcbPosXPin (Comp.Diode.PIN_C);
            double dbasecy = dbase.pcbPosYPin (Comp.Diode.PIN_C);
            double rbasecx = rbase.pcbPosXPin (Comp.Resis.PIN_C);
            double rbasecy = rbase.pcbPosYPin (Comp.Resis.PIN_C);
            double transbx = trans.pcbPosXPin (Comp.Trans.PIN_B);
            double transby = trans.pcbPosYPin (Comp.Trans.PIN_B);
            basenet.putseg (ps, dbasecx, dbasecy, rbasecx, rbasecy, "F.Cu");
            basenet.putseg (ps, rbasecx, rbasecy, transbx, transby, "B.Cu");

            double rcollax = rcoll.pcbPosXPin (Comp.Resis.PIN_A);
            double rcollay = rcoll.pcbPosYPin (Comp.Resis.PIN_A);
            double transcx = trans.pcbPosXPin (Comp.Trans.PIN_C);
            double transcy = trans.pcbPosYPin (Comp.Trans.PIN_C);
            double robasax = robas.pcbPosXPin (Comp.Resis.PIN_A);
            double robasay = robas.pcbPosYPin (Comp.Resis.PIN_A);
            _pinet.putseg (ps, rcollax,       rcollay,       transcx,       transcy,       "F.Cu");
            _pinet.putseg (ps, transcx,       transcy,       transcx + 0.5, transcy + 0.5, "B.Cu");
            _pinet.putseg (ps, transcx + 0.5, transcy + 0.5, transcx + 0.5, robasay - 0.5, "B.Cu");
            _pinet.putvia (ps, transcx + 0.5, robasay - 0.5, "B.Cu", "F.Cu");
            _pinet.putseg (ps, transcx + 0.5, robasay - 0.5, robasax - 0.5, robasay - 0.5, "F.Cu");
            _pinet.putseg (ps, robasax - 0.5, robasay - 0.5, robasax,       robasay,       "F.Cu");

            double robascx = robas.pcbPosXPin (Comp.Resis.PIN_C);
            double robascy = robas.pcbPosYPin (Comp.Resis.PIN_C);
            double otranbx = otran.pcbPosXPin (Comp.Trans.PIN_B);
            double otranby = otran.pcbPosYPin (Comp.Trans.PIN_B);
            rcvnet.putseg (ps, robascx, robascy, otranbx, robascy, "F.Cu");
            rcvnet.putseg (ps, otranbx, robascy, otranbx, otranby, "B.Cu");

            double rocolax = rocol.pcbPosXPin (Comp.Resis.PIN_A);
            double rocolay = rocol.pcbPosYPin (Comp.Resis.PIN_A);
            double otrancx = otran.pcbPosXPin (Comp.Trans.PIN_C);
            double otrancy = otran.pcbPosYPin (Comp.Trans.PIN_C);
            mqcpunet.putseg (ps, rocolax, rocolay, otrancx, otrancy, "F.Cu");
        }
    }
}
