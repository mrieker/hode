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
 * Delay built-in module.
 * Only works for -printprop reports,
 * does not provide any connectivity.
 */

import java.io.PrintStream;

public class DelayModule extends Module {
    private boolean[] dproproots;
    private int gates;              // delay in gates
    private int width;              // number of flipflops
    private int lastgoodclocks;     // bitmask of previous good clock values
    private String instvarsuf;

    private OpndLhs q;
    private OpndLhs d;
    private OpndLhs t;

    public DelayModule (String instvarsuf, int width)
    {
        name = "Delay" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.gates = 1;
        this.width = width;
        this.dproproots = new boolean[width];

        q = new QOutput ("Q" + instvarsuf, 0);

        if (instvarsuf.equals ("")) {
            d = new IParam ("D", 1);
            t = new IParam ("T", 2);
        } else {
            d = new OpndLhs ("D" + instvarsuf);
            t = new OpndLhs ("T" + instvarsuf);
        }

        q.hidim = d.hidim = t.hidim = this.width - 1;

        params = new OpndLhs[] { q, d, t };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // this gets the width token after the module name and before the (
    // eg, inst : Delay gates:50 width:16 (Q:dataout, D:datain, T:clock);
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.gates = 1;   // default 1 gate delay
        config.width = 1;   // put 1 for scalar
        while (! token.isDel (Token.DEL.OPRN)) {
            if ("gates".equalsIgnoreCase (token.getSym ())) {
                token = token.next;
                if (! token.isDel (Token.DEL.COLON)) throw new Exception ("expecting : after 'gates'");
                token = token.next;
                if (! (token instanceof Token.Int)) throw new Exception ("expecting integer after gates:'");
                config.gates = (int) ((Token.Int) token).value;
                token = token.next;
                continue;
            }
            if ("width".equalsIgnoreCase (token.getSym ())) {
                token = token.next;
                if (! token.isDel (Token.DEL.COLON)) throw new Exception ("expecting : after 'width'");
                token = token.next;
                if (! (token instanceof Token.Int)) throw new Exception ("expecting integer after width:'");
                config.width = (int) ((Token.Int) token).value;
                token = token.next;
                continue;
            }
            break;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public int gates;
        public int width;
    }

    // make a new Delay module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        DelayModule delayinst = new DelayModule (instvarsuf, config.width);
        delayinst.gates = config.gates;
        target.variables.putAll (delayinst.variables);
        return delayinst.params;
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
            return d.generate (genctx, rbit % d.busWidth ());
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
            d.stepSim (t);
        }
    }
}
