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
 * Built-in module to access the physical GPIO pins this NetGen program is runing on.
 * Used for testing the physical rasboard circuit board.
 */

import java.io.PrintStream;
import java.util.LinkedList;

public class GPIOModule extends Module {
    private String instvarsuf;      // instantiation name "/instname"
    private IPinInput clk, irq, res, mq, dena, qena;
    private OPinOutput md, mword, mread, mwrite, halt;

    private static boolean opened;
    private static GPIOModule singleton;
    private static int lastsimtime;

    private native static void opengpio ();
    private native static int readgpio ();
    private native static void writegpio (boolean wdata, int data);

    public GPIOModule (String instvarsuf)
    {
        name = "GPIO" + instvarsuf;
        this.instvarsuf = instvarsuf;

        if (instvarsuf.equals ("")) {
            params = new OpndLhs[0];
        } else {
                md = new OPinOutput (    "MD" + instvarsuf, 0);
             mword = new OPinOutput ( "MWORD" + instvarsuf, 1);
             mread = new OPinOutput ( "MREAD" + instvarsuf, 2);
            mwrite = new OPinOutput ("MWRITE" + instvarsuf, 3);
              halt = new OPinOutput (  "HALT" + instvarsuf, 4);

            // isInstParamIn() assumes there are exactly 5 output parameters

               clk = new IPinInput  (   "CLK" + instvarsuf);
               irq = new IPinInput  (   "IRQ" + instvarsuf);
               res = new IPinInput  (   "RES" + instvarsuf);
                mq = new IPinInput  (    "MQ" + instvarsuf);
              dena = new IPinInput  (  "DENA" + instvarsuf);
              qena = new IPinInput  (  "QENA" + instvarsuf);

            mq.hidim = md.hidim = 15;

            params = new OpndLhs[] { md, mword, mread, mwrite, halt, clk, irq, res, mq, dena, qena };
        }

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // make a new gpio module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        if (singleton != null) {
            throw new Exception ("only one instance of GPIO module allowed");
        }
        singleton = new GPIOModule (instvarsuf);

        // make gpio pin variables available to target module
        target.variables.putAll (singleton.variables);

        return singleton.params;
    }

    // get whether module parameter is an 'input' or an 'output' parameter
    //  input:
    //   prmn = parameter number
    //  output:
    //   returns true: IN
    //          false: OUT
    @Override
    public boolean isInstParamIn (Object instconfig, int prmn)
    {
        return prmn >= 5;
    }

    // INPUT PIN
    // - these route signals from software to the physical circuit board
    //   ie, it is being INPUT to the physical circuit board

    public static class IPinInput extends OpndLhs {
        public IPinInput (String name)
        {
            super (name);
        }
    }

    // OUTPUT PIN
    // - these route signals from the physical circuit board to software
    //   ie, it is being OUTPUT from the physical circuit board

    public static class OPinOutput extends OParam {
        public OPinOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// SIMULATOR ////

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulation (t);
        }
    }

    //// GENERATION ////

    public void getCircuit (GenCtx genctx)
    {
        throw new RuntimeException ("cannot generate circuit board with GPIO module");
    }

    //// SIMULATION ////

    private static void stepSimulation (int t)
            throws HaltedException
    {
        // if first time here, open portal to gpio registers
        if (! opened) {
            opened = true;
            System.out.println ("GPIOModule.stepSimulation*: java.library.path=" + System.getProperty("java.library.path"));
            System.loadLibrary ("GPIOModule");
            opengpio ();
        }

        // step simulation to given time
        GPIOModule inst = singleton;
        while (lastsimtime < t) {
            lastsimtime ++;

            // write values to GPIO pins that go on to physical circuit board
            boolean qenaval = inst.qena.getSimOutput (lastsimtime) == 0x100000000L;
            boolean denaval = inst.dena.getSimOutput (lastsimtime) == 0x100000000L;

            boolean clkval = inst.clk.getSimOutput (lastsimtime - 1) == 0x100000000L;
            boolean irqval = inst.irq.getSimOutput (lastsimtime - 1) == 0x100000000L;
            boolean resval = inst.res.getSimOutput (lastsimtime - 1) == 0x100000000L;
            int      mqval = (int) (inst.mq.getSimOutput (lastsimtime - 1) >> 32) & 0xFFFF;

            int data = (irqval ? 0x100000 : 0) | (mqval << 4) | (resval ? 8 : 0) | (clkval ? 4 : 0);

            writegpio (qenaval, data);

            // ?? always wait at least 1ms
            long started = System.currentTimeMillis ();
            while (System.currentTimeMillis () - started < 2) { }

            // read current GPIO pin values from physical circuit board
            data = readgpio () & 0x7FFFFC;

            // distribute the values to the various parameters
            inst.    md.setSimOutput (lastsimtime, denaval ? ((((long) data) << 28) | (~ data >>  4)) & 0x0000FFFF0000FFFFL : 0);
            inst. mword.setSimOutput (lastsimtime, ((((long) data) <<  9) | (~ data >> 23)) & 0x0000000100000001L);
            inst. mread.setSimOutput (lastsimtime, ((((long) data) <<  8) | (~ data >> 24)) & 0x0000000100000001L);
            inst.mwrite.setSimOutput (lastsimtime, ((((long) data) <<  7) | (~ data >> 25)) & 0x0000000100000001L);
            inst.  halt.setSimOutput (lastsimtime, ((((long) data) <<  6) | (~ data >> 26)) & 0x0000000100000001L);
        }
    }
}
