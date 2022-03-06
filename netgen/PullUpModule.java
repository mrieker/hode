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
 * Built-in pull-up resistor module.
 */

import java.io.PrintStream;

public class PullUpModule extends Module {
    private int width;              // number of resistors
    private String instvarsuf;

    private OpndLhs q;

    private Network[] outnets;

    public PullUpModule (String instvarsuf, int width)
    {
        name = "PullUp" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.width = width;

        q = new QOutput ( "Q" + instvarsuf, 0);

        q.hidim = this.width - 1;

        params = new OpndLhs[] { q };

        variables.put (q.name, q);
    }

    // this gets the width token after the module name and before the (
    // eg, inst : PullUp 16 ( ... );
    //                   ^ token is here
    //                      ^ return token here
    // by default, we make a scalar pullup
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.width = 1;   // put 1 for scalar
        if (token instanceof Token.Int) {
            Token.Int tokint = (Token.Int) token;
            long width = tokint.value;
            if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
            config.width = (int) width;
            token = token.next;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public int width;
    }

    // make a new PullUp module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        PullUpModule puinst = new PullUpModule (instvarsuf, config.width);
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
                Comp.Resis resistor = new Comp.Resis (genctx, null, rbit + "." + name, Comp.Resis.R_COL, false, true);
                genctx.addPlaceable (resistor);

                outnets[rbit] = outnet = new Network (genctx, rbit + "." + name);
                outnet.addConn (resistor, Comp.Resis.PIN_A);

                Network vccnet = genctx.nets.get ("VCC");
                vccnet.addConn (resistor, Comp.Resis.PIN_C);
            }
            return outnet;
        }

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
