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
 * Built-in buffered LED module.
 */

import java.io.PrintStream;

public class BufLEDModule extends Module {
    private boolean inverted;       // light when zero
    private int width;              // number of LEDs
    private String instvarsuf;

    private OpndLhs q;

    private Network[] outnets;

    public BufLEDModule (String instvarsuf, int width, boolean inverted)
    {
        name = "BufLED" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.inverted   = inverted;
        this.width = width;

        q = new QOutput ("Q" + instvarsuf, 0);

        q.hidim = this.width - 1;

        params = new OpndLhs[] { q };

        variables.put (q.name, q);
    }

    // this gets the width token after the module name and before the (
    // eg, inst : BufLED 16 [inv] ( ... );
    //                   ^ token is here
    //                            ^ return token here
    // by default, we make a scalar buffered LED
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.width = 1;   // put 1 for scalar
        while (! "(".equals (token.getDel ())) {
            if (token instanceof Token.Int) {
                Token.Int tokint = (Token.Int) token;
                long width = tokint.value;
                if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
                config.width = (int) width;
                token = token.next;
                continue;
            }
            if ("inv".equals (token.getSym ())) {
                config.inv = true;
                token = token.next;
                continue;
            }
            break;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public boolean inv;
        public int width;
    }

    // make a new BufLED module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        BufLEDModule puinst = new BufLEDModule (instvarsuf, config.width, config.inv);
        target.variables.putAll (puinst.variables);
        return puinst.params;
    }

    // the internal circuitry that writes to the output wires

    private class QOutput extends OParam {
        public QOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            if (outnets == null) {
                outnets = new Network[width];
            }

            Network outnet = outnets[rbit];
            if (outnet == null) {
                String label = instvarsuf;
                if (label.startsWith ("/")) label = label.substring (1);
                if (width > 10) label += String.format ("%02d", rbit);
                else if (width > 1) label += String.format ("%d", rbit);

                Comp.Resis rc = new Comp.Resis (genctx, null, "rc." + rbit + "." + name, "390", false, true);
                Comp.Resis rb = new Comp.Resis (genctx, null, "rb." + rbit + "." + name, "10K", false, true);
                Comp.SmLed sl = new Comp.SmLed (genctx, null, "sl." + rbit + "." + name, label, false);
                Comp.Trans qt = new Comp.Trans (genctx, null, "q."  + rbit + "." + name, inverted ? "2N3906" : "2N3904");

                genctx.addPlaceable (rc);
                genctx.addPlaceable (rb);
                genctx.addPlaceable (sl);
                genctx.addPlaceable (qt);

                Network gndnet = genctx.nets.get ("GND");
                Network vccnet = genctx.nets.get ("VCC");

                outnets[rbit] = outnet = new Network (genctx, rbit + "." + name);
                Network basenet = new Network (genctx, "b." + rbit + "." + name);
                Network collnet = new Network (genctx, "c." + rbit + "." + name);
                Network litenet = new Network (genctx, "l." + rbit + "." + name);

                if (inverted) {
                    gndnet.addConn  (sl, Comp.SmLed.PIN_C);
                    litenet.addConn (sl, Comp.SmLed.PIN_A);

                    litenet.addConn (rc, Comp.Resis.PIN_C);
                    collnet.addConn (rc, Comp.Resis.PIN_A);

                    collnet.addConn (qt, Comp.Trans.PIN_C);
                    basenet.addConn (qt, Comp.Trans.PIN_B);
                    vccnet.addConn  (qt, Comp.Trans.PIN_E);

                    outnet.addConn  (rb, Comp.Resis.PIN_C);
                    basenet.addConn (rb, Comp.Resis.PIN_A);
                } else {
                    vccnet.addConn  (sl, Comp.SmLed.PIN_A);
                    litenet.addConn (sl, Comp.SmLed.PIN_C);

                    litenet.addConn (rc, Comp.Resis.PIN_A);
                    collnet.addConn (rc, Comp.Resis.PIN_C);

                    collnet.addConn (qt, Comp.Trans.PIN_C);
                    basenet.addConn (qt, Comp.Trans.PIN_B);
                    gndnet.addConn  (qt, Comp.Trans.PIN_E);

                    outnet.addConn  (rb, Comp.Resis.PIN_A);
                    basenet.addConn (rb, Comp.Resis.PIN_C);
                }
            }
            return outnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        { }

        @Override
        protected int busWidthWork ()
        {
            return width;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            setSimOutput (t, ((1L << width) - 1) << 32);
        }
    }
}
