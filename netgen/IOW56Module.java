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
 * Built-in module to access an IO-Warrior-56 paddle plugged into one of the A,C,I,D connectors of a physical circuit board.
 * In essence, they make a physical circuit board look like a built-in module.
 *
 * An input pin of the circuit board is an input pin in this module so this module is a transparent wrapper of the circuit board.
 * Likewise, an output pin of the circuit board is an output pin of this module.
 */

import java.io.PrintStream;
import java.util.LinkedList;

import com.codemercs.iow.IowKit;

public class IOW56Module extends Module {
    private Config config;          // pin configuration
    private long iowhandle;
    private long inputvalu;
    private long outputvalu;
    private String instvarsuf;      // instantiation name "/instname"

    private final static int IOW_PIPE_IO_PINS = 0;
    private final static int IOW_PIPE_SPECIAL_MODE = 1;

    public IOW56Module (String instvarsuf, Config config)
    {
        name = "IOW56" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.config = config;

        if (config == null) {
            params = new OpndLhs[0];
        } else {
            params = config.params;
        }

        // parameters are named {I,O}hwpinname
        //  I params are for 'i' config letters
        //  O params are for 'o' config letters
        // they are indexed by order given in config string,
        //  just including the 'i's and 'o's
        // see getInstConfig()
        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // this gets the module configuration
    // eg, inst : IOW56 iiiooioiiiongg ... (...);
    //                       ^input ^ground
    //                     ^output ^not-connected
    // one string of letters per row on the connector
    // length of strings gives number of columns in connector
    // all strings must be same length
    //  input pin : signal coming from software going to physical circuit board
    // output pin : signal coming from physical circuit board going to software
    // note:
    //  order of arguments is same as i's and o's in config strings
    //  but parameters are named by the physical pin number, eg
    //  a 2x4 connector config'd with giov viio will be defined:
    //  module IOW56 giov viio (out I2, in O3, out I6, out I7, in O8) { }
    //         pins: 1234 5678
    // also supply id_<letter> to get serial number from envar iow56sn_<letter>
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        // configuration string is mandatory
        // also return it as the instantiation parameter
        // one string per row and they must all be of same length
        Config config = new Config ();
        int ncols = 0;
        int nrows = 0;
        String cfgstr = "";
        while (true) {
            String sym = token.getSym ();
            if (sym == null) break;
            sym = sym.toLowerCase ();
            if (sym.startsWith ("id_")) {
                String sn = System.getenv ("iow56sn_" + sym.substring (3));
                if (sn == null) throw new Exception ("no envar iow56sn_" + sym.substring (3) + " defined");
                config.iow56sn = sn;
            } else {
                if (ncols == 0) ncols = sym.length ();
                else if (sym.length () != ncols) {
                    throw new Exception ("IOW56 rows must be of equal lengths");
                }
                cfgstr += sym;
                nrows ++;
            }
            token = token.next;
        }
        if (cfgstr.equals ("")) throw new Exception ("IOW56 module instantiation must have configuration");
        if ((ncols != 2 && ncols != 20) || (nrows != 2 && nrows != 20) || (ncols * nrows != 40)) {
            throw new Exception ("Conn ncols=" + ncols + ", nrows=" + nrows + " not 2x20 or 20x2");
        }
        assert cfgstr.length () == 40;

        config.cfgstr = cfgstr;
        config.nrows  = nrows;
        instconfig_r[0] = config;

        // parse the string and create a wire for each 'i' or 'o'
        LinkedList<OpndLhs> params = new LinkedList<> ();
        config.iparams = new IPinInput[40];
        config.oparams = new OPinOutput[40];
        for (int cfgidx = 0; cfgidx < 40; cfgidx ++) {
            String pino = config.pinNumberFromCfgIdx (cfgidx);
            char c = cfgstr.charAt (cfgidx);
            switch (c) {

                // input pin indicating the physical circuit board has an input pin at this position
                // ...so the io-warrior-56 module has this programmed as an output pin
                case 'i': {
                    config.inputmask |= 1L << cfgidx;
                    config.iparams[cfgidx] = new IPinInput ("I" + pino + instvarsuf);
                    params.add (config.iparams[cfgidx]);
                    break;
                }

                // output pin indicating the physical circuit board has an output pin at this position
                // ...so the io-warrior-56 module has this programmed as an input pin
                case 'o': {
                    config.outputmask |= 1L << cfgidx;
                    config.oparams[cfgidx] = new OPinOutput ("O" + pino + instvarsuf, params.size ());
                    params.add (config.oparams[cfgidx]);
                    break;
                }

                // ground, not-connected do not have parameters
                case 'g':
                case 'n': break;

                // garbazhe
                default: {
                    throw new Exception ("bad character " + c + " in configuration " + cfgstr);
                }
            }
        }

        config.params = params.toArray (new OpndLhs[0]);

        return token;
    }

    // make a new io-warrior-56 module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the pin configuration is in instconfig as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        IOW56Module inst = new IOW56Module (instvarsuf, (Config) instconfig);

        // make io-warrior-56's pin variables available to target module
        target.variables.putAll (inst.variables);

        allinstances.add (inst);

        return inst.params;
    }

    // get whether module parameter is an 'input' or an 'output' parameter
    //  input:
    //   instconfig = as returned by getInstConfig()
    //   prmn = parameter number
    //  output:
    //   returns true: IN
    //          false: OUT
    @Override
    public boolean isInstParamIn (Object instconfig, int prmn)
    {
        Config config = (Config) instconfig;
        OpndLhs param = config.params[prmn];

        assert param.name.startsWith ("I") || param.name.startsWith ("O");

        return param.name.startsWith ("I");
    }

    // INPUT PIN
    // - these route signals from software to the physical circuit board
    // for example, if testing an ALU board, these provide operands to the ALU

    public static class IPinInput extends OpndLhs {
        public IPinInput (String name)
        {
            super (name);
        }
    }

    // OUTPUT PIN
    // - these route signals from the physical circuit board to software
    // for example, if testing an ALU board, these retrieve results from the ALU

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
        throw new RuntimeException ("cannot generate circuit board with IOW56 modules");
    }

    //// SIMULATION ////

    private static boolean iowopened = false;
    private static int lastsimtime = 0;
    private static LinkedList<IOW56Module> allinstances = new LinkedList<> ();

    private final static int[] reqallpins = new int[] {
                      255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    private final static byte[] pinmapping = new byte[] {
                        4 * 8 + 6,  // pin 01 -> P4.6
                        2 * 8 + 2,  // pin 02 -> P2.2
                        4 * 8 + 2,  // pin 03 -> P4.2
                        2 * 8 + 6,  // pin 04 -> P2.6
                        -1,         // pin 05 -> GND
                        0 * 8 + 0,  // pin 06 -> P0.0
                        3 * 8 + 6,  // pin 07 -> P3.6
                        2 * 8 + 4,  // pin 08 -> P2.4
                        -1,         // pin 09 -> GND
                        2 * 8 + 0,  // pin 10 -> P2.0
                        3 * 8 + 2,  // pin 11 -> P3.2
                        4 * 8 + 4,  // pin 12 -> P4.4
                        -1,         // pin 13 -> GND
                        4 * 8 + 0,  // pin 14 -> P4.0
                        5 * 8 + 6,  // pin 15 -> P5.6
                        3 * 8 + 4,  // pin 16 -> P3.4
                        -1,         // pin 17 -> GND
                        3 * 8 + 0,  // pin 18 -> P3.0
                        5 * 8 + 2,  // pin 19 -> P5.2
                        5 * 8 + 4,  // pin 20 -> P5.4

                        3 * 8 + 3,  // pin 21 -> P3.3
                        3 * 8 + 5,  // pin 22 -> P3.5
                        -1,         // pin 23 -> GND
                        4 * 8 + 1,  // pin 24 -> P4.1
                        3 * 8 + 7,  // pin 25 -> P3.7
                        4 * 8 + 5,  // pin 26 -> P4.5
                        -1,         // pin 27 -> GND
                        2 * 8 + 1,  // pin 28 -> P2.1
                        4 * 8 + 3,  // pin 29 -> P4.3
                        2 * 8 + 5,  // pin 30 -> P2.5
                        -1,         // pin 31 -> GND
                        0 * 8 + 1,  // pin 32 -> P0.1
                        4 * 8 + 7,  // pin 33 -> P4.7
                        0 * 8 + 5,  // pin 34 -> P0.5
                        -1,         // pin 35 -> GND
                        0 * 8 + 6,  // pin 36 -> P0.6
                        2 * 8 + 3,  // pin 37 -> P2.3
                        0 * 8 + 7,  // pin 38 -> P0.7
                        2 * 8 + 7,  // pin 39 -> P2.7
                        0 * 8 + 3   // pin 40 -> P0.3
    };

    private static void stepSimulation (int t)
            throws HaltedException
    {
        // if first time here, open handles to all the io-warrior-56 USB devices
        if (! iowopened) {
            iowopened = true;
            IowKit.openDevice ();
            long ndevs = IowKit.getNumDevs ();
            for (IOW56Module inst : allinstances) {
                for (int i = 0; ++ i <= ndevs;) {
                    long iowhandle = IowKit.getDeviceHandle (i);
                    String iowserial = IowKit.getSerialNumber (iowhandle);
                    if (inst.config.iow56sn.equals (iowserial)) {
                        inst.iowhandle = iowhandle;
                        break;
                    }
                }
                if (inst.iowhandle == 0) {
                    throw new RuntimeException ("io-warrior-56 " + inst.name + " serial number " + inst.config.iow56sn + " not found");
                }

                // all pins with inputmask bit set is an output pin from io-warrior-56's point of view
                // all pins with outputmask bit set is an input pin from io-warrior-56's point of view
                inst.inputvalu = -1L;

                int[] allones = new int[] { 0, 255, 255, 255, 255, 255, 255, 255 };
                IowKit.write (inst.iowhandle, IOW_PIPE_IO_PINS, allones);

                while (IowKit.readNonBlocking (inst.iowhandle, IOW_PIPE_IO_PINS, 8).length > 0) { }
                while (IowKit.readNonBlocking (inst.iowhandle, IOW_PIPE_SPECIAL_MODE, 64).length > 0) { }
            }
        }

        // step simulation to given time
        while (lastsimtime < t) {
            lastsimtime ++;

            // step through each of the 4 paddles to see if we need to send them something
            boolean sentsomething = false;
            for (IOW56Module inst : allinstances) {

                // 'input' pins are for signals going in to the physical board
                if (inst.config.inputmask != 0) {

                    // step through each of the 40 possible input pins per paddle
                    // ...so we can figure out what the paddle should be sending to the physical circuit board
                    long newval = inst.inputvalu;
                    for (int i = 0; i < 40; i ++) {
                        long m = 1L << i;
                        IPinInput iparam = inst.config.iparams[i];
                        if (iparam != null) {
                            // get value from, for example, the test script using a force command
                            // ...or from some boolean equation or some such thing
                            // and save it to be sent to the physical circuit board
                            long val = iparam.getSimOutput (lastsimtime - 1);
                            if ((val &          1) != 0) newval &= ~ m;
                            if ((val & 0x100000000L) != 0) newval |= m;
                        }
                    }

                    // if not already sending correct value to physical circuit board, send correct value
                    if (inst.inputvalu != newval) {
                        inst.inputvalu = newval;

                        int[] update = new int[] { 0, 255, 255, 255, 255, 255, 255, 255 };
                        for (int i = 0; i < 40; i ++) {
                            if ((newval & (1L << i)) == 0) {
                                byte pm = pinmapping[i];
                                update[(pm>>3)+1] -= 1 << (pm & 7);
                            }
                        }
                        IowKit.write (inst.iowhandle, IOW_PIPE_IO_PINS, update);

                        sentsomething = true;
                    }
                }
            }

            if (sentsomething) {

                // always wait at least 1ms for circuit to settle
                long started = System.currentTimeMillis ();
                while (System.currentTimeMillis () - started < 3) { }

                // step through each of the 4 paddles again to see if any of the physical circuit board outputs changed
                for (IOW56Module inst : allinstances) {

                    // 'output' pins are for signals coming out of the physical board
                    if (inst.config.outputmask != 0) {

                        // read current physical circuit board output pin values from the paddle
                        IowKit.write (inst.iowhandle, IOW_PIPE_SPECIAL_MODE, reqallpins);
                        int[] report = IowKit.read (inst.iowhandle, IOW_PIPE_SPECIAL_MODE, 64);
                        if (report.length < 7) throw new RuntimeException ("bad report length " + report.length);
                        if (report[0] != 255) throw new RuntimeException ("bad report type " + report[0]);

                        // set them as the IOW56Module output pin values for this simulation timestep
                        for (int i = 0; i < 40; i ++) {
                            long m = 1L << i;
                            byte pm = pinmapping[i];
                            inst.outputvalu &= ~ m;
                            if ((report[(pm>>3)+1] & (1 << (pm & 7))) != 0) inst.outputvalu |= m;
                        }
                    }
                }
            }

            // update sim output pin values
            for (IOW56Module inst : allinstances) {

                // set them as the IOW56Module output pin values for this simulation timestep
                for (int i = 0; i < 40; i ++) {
                    OPinOutput oparam = inst.config.oparams[i];
                    if (oparam != null) {
                        oparam.setSimOutput (lastsimtime, (((inst.outputvalu >> i) & 1) != 0) ? 0x100000000L : 1);
                    }
                }
            }
        }
    }

    // holds per-connector configuration information
    private static class Config {
        public int nrows;               // 2 or 20
        public int inputmask;           // one bit set per element in iparams array
        public int outputmask;          // one bit set per element in oparams array
        public IPinInput[]  iparams;    // always len 40 with embedded nulls
        public OPinOutput[] oparams;    // always len 40 with embedded nulls
        public OpndLhs[] params;        // len <= 32, no embedded nulls
        public String cfgstr;           // always len 40
        public String iow56sn;

        // cfgstr for a 2x4 connector has 4 chars for top row indexed 0..3
        // and 4 chars for bottom row indexed 4..7:
        //   *1 2 3 4
        //    5 6 7 8
        // so convert the index to pin
        //  input:
        //   cfgidx = which letter (zero based) from all config strings tacked together
        //  output:
        //   returns hardware pin number understood by kicad
        public String pinNumberFromCfgIdx (int cfgidx)
        {
            assert (cfgidx >= 0) && (cfgidx < cfgstr.length ());
            return Integer.toString (cfgidx + 1);
        }
    }
}
