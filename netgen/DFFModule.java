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
 * Built-in D flipflop module.
 */

import java.io.PrintStream;

public class DFFModule extends Module {
    private boolean miniature;
    private boolean miniflat;
    private int lighted;            // =0: none; >0: lit when SET; <0: lit when CLEAR
    private int width;              // number of flipflops
    private int lastgoodclocks;     // bitmask of previous good clock values
    private String instvarsuf;

    private OpndLhs   q;
    private OpndLhs  _q;
    private OpndLhs   d;
    private OpndLhs clk;
    private OpndLhs _pc;
    private OpndLhs _ps;

    private DFFCell[] dffnands;

    public DFFModule (String instvarsuf, int width)
    {
        name = "DFF" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.width = width;

          q = new  QOutput ( "Q" + instvarsuf, 0);
         _q = new _QOutput ("_Q" + instvarsuf, 1);

        if (instvarsuf.equals ("")) {
              d = new IParam (  "D", 2);
            clk = new IParam (  "T", 3);
            _pc = new IParam ("_PC", 4);
            _ps = new IParam ("_PS", 5);
        } else {
              d = new OpndLhs (  "D" + instvarsuf);
            clk = new OpndLhs (  "T" + instvarsuf);
            _pc = new OpndLhs ("_PC" + instvarsuf);
            _ps = new OpndLhs ("_PS" + instvarsuf);
        }

        q.hidim = _q.hidim = d.hidim = this.width - 1;

        params = new OpndLhs[] { q, _q, d, clk, _pc, _ps };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // this gets the width token after the module name and before the (
    // eg, inst : DFF 16 ( ... );
    //                ^ token is here
    //                   ^ return token here
    // by default, we make a scalar flipflop
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.width = 1;   // put 1 for scalar
        while (! token.isDel (Token.DEL.OPRN)) {
            if (token instanceof Token.Int) {
                Token.Int tokint = (Token.Int) token;
                long width = tokint.value;
                if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
                config.width = (int) width;
                token = token.next;
                continue;
            }
            if ("led1".equalsIgnoreCase (token.getSym ())) {
                // led is lit when Q=1
                config.light = 1;
                token = token.next;
                continue;
            }
            if ("led0".equalsIgnoreCase (token.getSym ())) {
                // led is lit when Q=0
                config.light = -1;
                token = token.next;
                continue;
            }
            if ("mini".equalsIgnoreCase (token.getSym ())) {
                config.miniature = true;
                config.miniflat  = false;
                token = token.next;
                continue;
            }
            if ("miniflat".equalsIgnoreCase (token.getSym ())) {
                config.miniature = true;
                config.miniflat  = true;
                token = token.next;
                continue;
            }
            break;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public boolean miniature;
        public boolean miniflat;
        public int width;
        public int light;
    }

    // make a new DFF module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        DFFModule dffinst = new DFFModule (instvarsuf, config.width);
        dffinst.lighted = config.light;
        dffinst.miniature = config.miniature;
        dffinst.miniflat  = config.miniflat;
        target.variables.putAll (dffinst.variables);
        return dffinst.params;
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
            // generate circuit for flipflop
            getCircuit (genctx, rbit);

            // Q output comes from the F gate
            return dffnands[rbit].foutnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DFF", this, rbit);
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
            stepSimulator (t);
        }
    }

    private class _QOutput extends OParam {
        public _QOutput (String name, int index)
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
            // generate circuit for flipflop
            getCircuit (genctx, rbit);

            // _Q output comes from the E gate
            return dffnands[rbit].eoutnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DFF", this, rbit);
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
            stepSimulator (t);
        }
    }

    //// GENERATION ////

    // generate circuit for the given bitslice of the flipflop array
    //  input:
    //   rbit = relative bitnumber in the array
    //  output:
    //   returns network for the flipflop's Q output
    private void getCircuit (GenCtx genctx, int rbit)
    {
        if (dffnands == null) {
            dffnands = new DFFCell[width];
        }

        // see if we have already generated circuitry for this bitslice
        if (dffnands[rbit] != null) return;

        // generate circuitry
        DFFCell dffnand = miniature ? new DFFMini (genctx, miniflat, rbit) : new DFFStand (genctx, rbit);
        dffnands[rbit]  = dffnand;

        // set up to place flipflop on circuit board
        genctx.addPlaceable (dffnand);

        // get networks all our inputs are connected to
        // also the D input is start of propagation chain
        genctx.addPropRoot (d, rbit);
        Network   dnet =   d.generate (genctx, rbit %   d.busWidth ());
        Network clknet = clk.generate (genctx, rbit % clk.busWidth ());
        Network _pcnet = _pc.generate (genctx, rbit % _pc.busWidth ());
        Network _psnet = _ps.generate (genctx, rbit % _ps.busWidth ());

        // wire up flipflop inputs
        dffnand.setInputs (genctx, rbit, dnet, clknet, _pcnet, _psnet);
    }

    //// SIMULATION ////

    private void stepSimulator (int t)
            throws HaltedException
    {
        // add extra 2 gate delays for 6-transistor configuration
        // ie, the input gates see inputs 2 more gate delays ago
        int t2 = t - 2 * OpndRhs.SIM_PROP;

        // get state all up front in case of recursion
        long   dval =   d.getSmearedInput (t2, width);
        long   tval = clk.getSmearedInput (t2, width);
        long _pcval = _pc.getSmearedInput (t2, width);
        long _psval = _ps.getSmearedInput (t2, width);
        long   qval =   q.getSimOutput (t - 1);

        // process flipflops one at a time
        for (int bitn = 0; bitn < width; bitn ++) {
            long longm = 0x100000001L << bitn;
            long zerom = 1 << bitn;
            long onem  = 0x100000000L << bitn;

            switch ((int) (((_pcval & zerom) << 1) | (_psval & zerom))) {

                // neither preset nor preclear
                case 0: {

                    // clock transitioned from low to high
                    if (((lastgoodclocks & zerom) == 0) & ((tval & onem) != 0)) {

                        // remember we have seen a high clock
                        lastgoodclocks |= zerom;

                        // forward data to the q/_q outputs
                        qval = (qval & ~longm) | (dval & longm);
                        break;
                    }

                    // clock transitioned from high to low
                    if (((lastgoodclocks & zerom) != 0) & ((tval & zerom) != 0)) {

                        // remember we have seen a low clock
                        lastgoodclocks &= ~zerom;
                    }

                    // maintain data output from last timestep
                    break;
                }

                // preset flipflop
                case 1: {
                    qval = (qval & ~longm) | onem;
                    break;
                }

                // preclear flipflop
                case 2: {
                    qval = (qval & ~longm) | zerom;
                    break;
                }

                // both preset and preclear, undefined outputs
                case 3: {
                    qval &= ~longm;
                    break;
                }
            }
        }

        // set outputs for this timestep
         q.setSimOutput (t, qval);
        _q.setSimOutput (t, (qval << 32) | (qval >>> 32));
    }

    // holds circuitry that makes a single bit D flipflop
    private abstract class DFFCell extends Placeable {
        public int rbit;
        public Network eoutnet;     // _Q output
        public Network foutnet;     //  Q output

        public abstract void setInputs (GenCtx genctx, int rbit, Network dnet, Network clknet, Network _pcnet, Network _psnet);

        @Override  // Placeable
        public String getName ()
        {
            return rbit + "." + name;
        }

        @Override  // Placeable
        public void flipIt ()
        {
            throw new RuntimeException ("DFFCell.flipIt() not implemented");
        }
    }

    // holds the 6 standard cells that make up a D flipflop
    private class DFFStand extends DFFCell implements PreDrawable {
        public StandCell anand;     // _PC input, D input
        public StandCell bnand;     // clock
        public StandCell cnand;     // clock
        public StandCell dnand;     // _PS input
        public StandCell enand;     // _Q output
        public StandCell fnand;     //  Q output

        private Network clknet;
        private Network datnet;
        private Network _pcnet;
        private Network _psnet;

        private int abwidth;
        private int cdwidth;
        private int efwidth;

        private int aceheight;
        private int bdfheight;

        private int anand_in__pc = -1;
        private int anand_in_dat = -1;
        private int anand_in_B   = -1;

        private int bnand_in_A   = -1;
        private int bnand_in_clk = -1;
        private int bnand_in_C   = -1;

        private int cnand_in__pc = -1;
        private int cnand_in_clk = -1;
        private int cnand_in_D   = -1;

        private int dnand_in_C   = -1;
        private int dnand_in_A   = -1;
        private int dnand_in__ps = -1;

        private int enand_in__pc = -1;
        private int enand_in_B   = -1;
        private int enand_in_F   = -1;

        private int fnand_in_E   = -1;
        private int fnand_in_C   = -1;
        private int fnand_in__ps = -1;

        public DFFStand (GenCtx genctx, int rbit)
        {
            // always create output cells before reading inputs to prevent infinite recursion
            String label = (lighted != 0) ? name.split ("/") [1] : null;
            enand = new StandCell (genctx, "e." + rbit + "." + name, (lighted > 0) ? label : null, false);
            fnand = new StandCell (genctx, "f." + rbit + "." + name, (lighted < 0) ? label : null, false);

            eoutnet = enand.getOutput ();
            foutnet = fnand.getOutput ();

            // maybe draw some traces between cells
            genctx.predrawables.add (this);
        }

        @Override  // DFFCell
        public void setInputs (GenCtx genctx, int rbit, Network dnet, Network clknet, Network _pcnet, Network _psnet)
        {
            this.clknet = clknet;
            this.datnet =   dnet;
            this._pcnet = _pcnet;
            this._psnet = _psnet;

            // grounded clock means do an SR flipflop with just enand and fnand
            if (clknet.name.equals ("GND")) {
                enand_in__pc = enand.addInput (0, _pcnet);
                enand_in_F   = enand.addInput (0, fnand.getOutput ());
                fnand_in_E   = fnand.addInput (0, enand.getOutput ());
                fnand_in__ps = fnand.addInput (0, _psnet);
                enand.close ();
                fnand.close ();
                return;
            }

            // grounded D means eliminate anand
            if (dnet.name.equals ("GND")) {
                bnand = new StandCell (genctx, "b." + rbit + "." + name);
                cnand = new StandCell (genctx, "c." + rbit + "." + name);
                dnand = new StandCell (genctx, "d." + rbit + "." + name);

                bnand_in_clk = bnand.addInput (0, clknet);
                bnand_in_C   = bnand.addInput (0, cnand.getOutput ());

                cnand_in__pc = cnand.addInput (0, _pcnet);
                cnand_in_clk = cnand.addInput (0, clknet);
                cnand_in_D   = cnand.addInput (0, dnand.getOutput ());

                dnand_in_C   = dnand.addInput (0, cnand.getOutput ());
                dnand_in__ps = dnand.addInput (0, _psnet);

                enand_in__pc = enand.addInput (0, _pcnet);
                enand_in_B   = enand.addInput (0, bnand.getOutput ());
                enand_in_F   = enand.addInput (0, fnand.getOutput ());

                fnand_in_E   = fnand.addInput (0, enand.getOutput ());
                fnand_in_C   = fnand.addInput (0, cnand.getOutput ());
                fnand_in__ps = fnand.addInput (0, _psnet);

                bnand.close ();
                cnand.close ();
                dnand.close ();
                enand.close ();
                fnand.close ();
                return;
            }

            // full D flipflop
            anand = new StandCell (genctx, "a." + rbit + "." + name);
            bnand = new StandCell (genctx, "b." + rbit + "." + name);
            cnand = new StandCell (genctx, "c." + rbit + "." + name);
            dnand = new StandCell (genctx, "d." + rbit + "." + name);

            anand_in__pc = anand.addInput (0, _pcnet);
            anand_in_dat = anand.addInput (0,   dnet);
            anand_in_B   = anand.addInput (0, bnand.getOutput ());

            bnand_in_A   = bnand.addInput (0, anand.getOutput ());
            bnand_in_clk = bnand.addInput (0, clknet);
            bnand_in_C   = bnand.addInput (0, cnand.getOutput ());

            cnand_in__pc = cnand.addInput (0, _pcnet);
            cnand_in_clk = cnand.addInput (0, clknet);
            cnand_in_D   = cnand.addInput (0, dnand.getOutput ());

            dnand_in_C   = dnand.addInput (0, cnand.getOutput ());
            dnand_in_A   = dnand.addInput (0, anand.getOutput ());
            dnand_in__ps = dnand.addInput (0, _psnet);

            enand_in__pc = enand.addInput (0, _pcnet);
            enand_in_B   = enand.addInput (0, bnand.getOutput ());
            enand_in_F   = enand.addInput (0, fnand.getOutput ());

            fnand_in_E   = fnand.addInput (0, enand.getOutput ());
            fnand_in_C   = fnand.addInput (0, cnand.getOutput ());
            fnand_in__ps = fnand.addInput (0, _psnet);

            anand.close ();
            bnand.close ();
            cnand.close ();
            dnand.close ();
            enand.close ();
            fnand.close ();
        }

        @Override  // Placeable
        public int getHeight ()
        {
            arrange ();
            return aceheight + bdfheight;
        }

        @Override  // Placeable
        public int getWidth ()
        {
            arrange ();
            return abwidth + cdwidth + efwidth;
        }

        @Override  // Placeable
        public void setPosXY (double leftx, double topy)
        {
            arrange ();

            if (anand != null) anand.setPosXY (leftx, topy);
            if (bnand != null) bnand.setPosXY (leftx, topy + aceheight);

            if (cnand != null) cnand.setPosXY (leftx + abwidth, topy);
            if (dnand != null) dnand.setPosXY (leftx + abwidth, topy + aceheight);

            enand.setPosXY (leftx + abwidth + cdwidth, topy);
            fnand.setPosXY (leftx + abwidth + cdwidth, topy + aceheight);
        }

        @Override  // Placeable
        public double getPosX ()
        {
            return enand.getPosX () - abwidth - cdwidth;
        }

        @Override  // Placeable
        public double getPosY ()
        {
            return enand.getPosY ();
        }

        private void arrange ()
        {
            if (efwidth == 0) {
                int awidth = (anand == null) ? 0 : anand.getWidth ();
                int bwidth = (bnand == null) ? 0 : bnand.getWidth ();
                int cwidth = (cnand == null) ? 0 : cnand.getWidth ();
                int dwidth = (dnand == null) ? 0 : dnand.getWidth ();
                int ewidth = enand.getWidth ();
                int fwidth = fnand.getWidth ();

                abwidth = Math.max (awidth, bwidth);
                cdwidth = Math.max (cwidth, dwidth);
                efwidth = Math.max (ewidth, fwidth);

                int aheight = (anand == null) ? 0 : anand.getHeight ();
                int bheight = (bnand == null) ? 0 : bnand.getHeight ();
                int cheight = (cnand == null) ? 0 : cnand.getHeight ();
                int dheight = (dnand == null) ? 0 : dnand.getHeight ();
                int eheight = enand.getHeight ();
                int fheight = fnand.getHeight ();

                aceheight = Math.max (aheight, Math.max (cheight, eheight));
                bdfheight = Math.max (bheight, Math.max (dheight, fheight));
            }
        }

        @Override  // Placeable
        public Network[] getExtNets ()
        {
            return new Network[] { eoutnet, foutnet, datnet, clknet, _pcnet, _psnet };
        }

        @Override  // PreDrawable
        public void preDraw (PrintStream ps)
        {
            // e out -> f in 1
            double eoutx = enand.hasSmLed () ? enand.colPcbX () : enand.resPcbX ();
            double eouty = enand.hasSmLed () ? enand.colPcbY () : enand.resPcbY ();
            assert fnand_in_E == 0;
            double finEx = fnand.inPcbX (0);    // top diode
            double finEy = fnand.inPcbY (0);
            Network enet = enand.getOutput ();
            enet.putseg (ps, eoutx,       eouty,       eoutx - 0.5, eouty + 0.5, "F.Cu");
            enet.putseg (ps, eoutx - 0.5, eouty + 0.5, finEx + 0.5, eouty + 0.5, "F.Cu");
            enet.putvia (ps, finEx + 0.5, eouty + 0.5, "F.Cu", "B.Cu");
            enet.putseg (ps, finEx + 0.5, eouty + 0.5, finEx + 0.5, finEy - 0.5, "B.Cu");
            enet.putseg (ps, finEx + 0.5, finEy - 0.5, finEx,       finEy,       "B.Cu");

            // f out -> e in 2 or 3
            double foutx = fnand.hasSmLed () ? fnand.ledPcbX () : fnand.resPcbX ();
            double fouty = fnand.hasSmLed () ? fnand.ledPcbY () : fnand.resPcbY ();
            double einFx = enand.inPcbX (enand_in_F);
            double einFy = enand.inPcbY (enand_in_F);
            Network fnet = fnand.getOutput ();
            if ((enand_in_F >= 2) || fnand.hasSmLed ()) {
                if (fnand.hasSmLed () && (enand_in_F < 2)) {
                    fnet.putseg (ps, foutx,       fouty,       foutx,       einFy + 1.0, "B.Cu");
                    fnet.putseg (ps, foutx,       einFy + 1.0, foutx - 0.5, einFy + 0.5, "B.Cu");
                    fnet.putvia (ps, foutx - 0.5, einFy + 0.5, "B.Cu", "F.Cu");
                    fnet.putseg (ps, foutx - 0.5, einFy + 0.5, einFx + 0.5, einFy + 0.5, "F.Cu");
                } else {
                    fnet.putseg (ps, foutx,       fouty,       foutx,       einFy + 0.5, "B.Cu");
                    fnet.putvia (ps, foutx,       einFy + 0.5, "B.Cu", "F.Cu");
                    fnet.putseg (ps, foutx,       einFy + 0.5, einFx + 0.5, einFy + 0.5, "F.Cu");
                }
                fnet.putseg (ps, einFx + 0.5, einFy + 0.5, einFx,       einFy,       "F.Cu");
            } else {
                fnet.putseg (ps, foutx,       fouty,       foutx,       einFy + 1.0, "B.Cu");
                fnet.putseg (ps, foutx,       einFy + 1.0, foutx - 0.5, einFy + 0.5, "B.Cu");
                fnet.putvia (ps, foutx - 0.5, einFy + 0.5, "B.Cu", "F.Cu");
                fnet.putseg (ps, foutx - 0.5, einFy + 0.5, einFx + 0.5, einFy + 0.5, "F.Cu");
                fnet.putseg (ps, einFx + 0.5, einFy + 0.5, einFx,       einFy,       "F.Cu");
            }

            // maybe just have enand,fnand (T tied to ground)
            if (dnand == null) return;

            // c out -> d in 1
            double coutx = cnand.colPcbX ();    // collector
            double couty = cnand.colPcbY ();
            assert dnand_in_C == 0;
            double dinCx = dnand.inPcbX (0);    // top diode
            double dinCy = dnand.inPcbY (0);
            Network cnet = cnand.getOutput ();
            cnet.putseg (ps, coutx,       couty,       coutx + 0.5, couty + 0.5, "B.Cu");
            cnet.putseg (ps, coutx + 0.5, couty + 0.5, coutx + 0.5, dinCy - 0.5, "B.Cu");
            cnet.putvia (ps, coutx + 0.5, dinCy - 0.5, "B.Cu", "F.Cu");
            cnet.putseg (ps, coutx + 0.5, dinCy - 0.5, dinCx + 0.5, dinCy - 0.5, "F.Cu");
            cnet.putseg (ps, dinCx + 0.5, dinCy - 0.5, dinCx,       dinCy,       "F.Cu");

            // c out -> f in 2
            assert fnand_in_C == 1;
            double finCx = fnand.inPcbX (1);    // middle or bottom diode
            double finCy = fnand.inPcbY (1);
            cnet.putseg (ps, coutx + 0.5, dinCy - 0.5, coutx + 0.5, finCy - 0.5, "B.Cu");
            cnet.putvia (ps, coutx + 0.5, finCy - 0.5, "B.Cu", "F.Cu");
            cnet.putseg (ps, coutx + 0.5, finCy - 0.5, finCx - 0.5, finCy - 0.5, "F.Cu");
            cnet.putseg (ps, finCx - 0.5, finCy - 0.5, finCx,       finCy,       "F.Cu");

            // d in 1 (c out) -> b in 2 or 3
            assert (bnand_in_C == 1) || (bnand_in_C == 2);
            double binCx = bnand.inPcbX (bnand_in_C);
            double binCy = bnand.inPcbY (bnand_in_C);
            cnet.putseg (ps, dinCx, dinCy, dinCx - 0.5, dinCy + 0.5, "B.Cu");
            cnet.putseg (ps, dinCx - 0.5, dinCy + 0.5, dinCx - 0.5, binCy - 0.5, "B.Cu");
            cnet.putvia (ps, dinCx - 0.5, binCy - 0.5, "B.Cu", "F.Cu");
            cnet.putseg (ps, dinCx - 0.5, binCy - 0.5, binCx + 0.5, binCy - 0.5, "F.Cu");
            cnet.putseg (ps, binCx + 0.5, binCy - 0.5, binCx, binCy, "F.Cu");

            // d out -> c in 2 or 3
            double doutx = dnand.resPcbX ();        // resistor
            double douty = dnand.resPcbY ();
            Network dnet = dnand.getOutput ();
            assert (cnand_in_D == 1) || (cnand_in_D == 2);
            double cinDx = cnand.inPcbX (cnand_in_D);
            double cinDy = cnand.inPcbY (cnand_in_D);
            if (cnand_in_D >= 2) {
                dnet.putseg (ps, doutx,       douty,       doutx,       cinDy + 0.5, "B.Cu");
                dnet.putvia (ps, doutx,       cinDy + 0.5, "B.Cu", "F.Cu");
                dnet.putseg (ps, doutx,       cinDy + 0.5, cinDx + 0.5, cinDy + 0.5, "F.Cu");
                dnet.putseg (ps, cinDx + 0.5, cinDy + 0.5, cinDx,       cinDy,       "F.Cu");
            } else {
                dnet.putseg (ps, doutx,       douty,       doutx,       cinDy + 1.0, "B.Cu");
                dnet.putseg (ps, doutx,       cinDy + 1.0, doutx - 0.5, cinDy + 0.5, "B.Cu");
                dnet.putvia (ps, doutx - 0.5, cinDy + 0.5, "B.Cu", "F.Cu");
                dnet.putseg (ps, doutx - 0.5, cinDy + 0.5, cinDx + 0.5, cinDy + 0.5, "F.Cu");
                dnet.putseg (ps, cinDx + 0.5, cinDy + 0.5, cinDx, cinDy, "F.Cu");
            }

            if (anand != null) {

                // a out -> b in 1
                double aoutx = anand.colPcbX ();    // collector
                double aouty = anand.colPcbY ();
                assert bnand_in_A == 0;
                double binAx = bnand.inPcbX (0);
                double binAy = bnand.inPcbY (0);
                Network anet = anand.getOutput ();
                anet.putseg (ps, aoutx, aouty, aoutx + 0.5, aouty + 0.5, "B.Cu");
                anet.putseg (ps, aoutx + 0.5, aouty + 0.5, aoutx + 0.5, binAy - 0.5, "B.Cu");
                anet.putvia (ps, aoutx + 0.5, binAy - 0.5, "B.Cu", "F.Cu");
                anet.putseg (ps, aoutx + 0.5, binAy - 0.5, binAx + 0.5, binAy - 0.5, "F.Cu");
                anet.putseg (ps, binAx + 0.5, binAy - 0.5, binAx, binAy, "F.Cu");

                // a out -> d in 2
                assert dnand_in_A == 1;
                double dinAx = dnand.inPcbX (1);
                double dinAy = dnand.inPcbY (1);
                anet.putseg (ps, aoutx + 0.5, binAy - 0.5, aoutx + 0.5, dinAy - 0.5, "B.Cu");
                anet.putvia (ps, aoutx + 0.5, dinAy - 0.5, "B.Cu", "F.Cu");
                anet.putseg (ps, aoutx + 0.5, dinAy - 0.5, dinAx - 0.5, dinAy - 0.5, "F.Cu");
                anet.putseg (ps, dinAx - 0.5, dinAy - 0.5, dinAx,       dinAy,       "F.Cu");

                // b out -> a in 2 or 3
                double boutx = bnand.colPcbX ();            // collector
                double bouty = bnand.colPcbY ();
                double ainBx = anand.inPcbX (anand_in_B);   // bottom diode
                double ainBy = anand.inPcbY (anand_in_B);
                Network bnet = bnand.getOutput ();
                bnet.putseg (ps, boutx, bouty, boutx - 0.5, bouty - 0.5, "B.Cu");
                bnet.putseg (ps, boutx - 0.5, bouty - 0.5, boutx - 0.5, ainBy + 0.5, "B.Cu");
                bnet.putvia (ps, boutx - 0.5, ainBy + 0.5, "B.Cu", "F.Cu");
                bnet.putseg (ps, boutx - 0.5, ainBy + 0.5, ainBx + 0.5, ainBy + 0.5, "F.Cu");
                bnet.putseg (ps, ainBx + 0.5, ainBy + 0.5, ainBx, ainBy, "F.Cu");

                // b out -> e in 1 or 2
                double einBx = enand.inPcbX (enand_in_B);
                double einBy = enand.inPcbY (enand_in_B);
                bnet.putseg (ps, boutx - 0.5, ainBy + 0.5, boutx - 0.5, einBy + 0.5, "B.Cu");
                bnet.putvia (ps, boutx - 0.5, einBy + 0.5, "B.Cu", "F.Cu");
                bnet.putseg (ps, boutx - 0.5, einBy + 0.5, einBx - 0.5, einBy + 0.5, "F.Cu");
                bnet.putseg (ps, einBx - 0.5, einBy + 0.5, einBx,       einBy,       "F.Cu");
            } else {

                // b out -> e in 1 or 2
                double boutx = bnand.colPcbX ();            // collector
                double bouty = bnand.colPcbY ();
                double einBx = enand.inPcbX (enand_in_B);
                double einBy = enand.inPcbY (enand_in_B);
                Network bnet = bnand.getOutput ();
                bnet.putseg (ps, boutx,       bouty,       boutx,       einBy + 0.5, "B.Cu");
                bnet.putvia (ps, boutx,       einBy + 0.5, "B.Cu", "F.Cu");
                bnet.putseg (ps, boutx,       einBy + 0.5, einBx - 0.5, einBy + 0.5, "F.Cu");
                bnet.putseg (ps, einBx - 0.5, einBy + 0.5, einBx,       einBy,       "F.Cu");
            }

            // common clock inputs
            double bnandclkx = bnand.inPcbX (bnand_in_clk);
            double bnandclky = bnand.inPcbY (bnand_in_clk);
            double cnandclkx = cnand.inPcbX (cnand_in_clk);
            double cnandclky = cnand.inPcbY (cnand_in_clk);
            if (anand != null) {
                clknet.putseg (ps, bnandclkx,       bnandclky,       bnandclkx + 0.5, bnandclky - 0.5, "B.Cu");
                clknet.putseg (ps, bnandclkx + 0.5, bnandclky - 0.5, bnandclkx + 0.5, cnandclky - 0.5, "B.Cu");
                clknet.putvia (ps, bnandclkx + 0.5, cnandclky - 0.5, "B.Cu", "F.Cu");
                clknet.putseg (ps, bnandclkx + 0.5, cnandclky - 0.5, cnandclkx - 0.5, cnandclky - 0.5, "F.Cu");
                clknet.putseg (ps, cnandclkx - 0.5, cnandclky - 0.5, cnandclkx,       cnandclky,       "F.Cu");
            } else {
                clknet.putseg (ps, bnandclkx, bnandclky, bnandclkx, cnandclky, "B.Cu");
                clknet.putvia (ps, bnandclkx, cnandclky, "B.Cu", "F.Cu");
                clknet.putseg (ps, bnandclkx, cnandclky, cnandclkx, cnandclky, "F.Cu");
            }

            // common preclear inputs
            if (enand_in__pc >= 0) {
                assert enand_in__pc == 0;       // assume both diodes are on very top row
                assert cnand_in__pc == 0;       // ...so our wiring will be out of the way
                double cnand_pcx = cnand.inPcbX (cnand_in__pc);
                double cnand_pcy = cnand.inPcbY (cnand_in__pc);
                double enand_pcx = enand.inPcbX (enand_in__pc);
                double enand_pcy = enand.inPcbY (enand_in__pc);
                assert cnand_pcy == enand_pcy;  // horizontal connection links the two
                _pcnet.putseg (ps, cnand_pcx,       enand_pcy,       cnand_pcx + 0.5, enand_pcy + 0.5, "F.Cu");
                _pcnet.putseg (ps, cnand_pcx + 0.5, enand_pcy + 0.5, enand_pcx - 0.5, enand_pcy + 0.5, "F.Cu");
                _pcnet.putseg (ps, enand_pcx - 0.5, enand_pcy + 0.5, enand_pcx,       enand_pcy,       "F.Cu");

                if (anand_in__pc >= 0) {
                    assert anand_in__pc == 0;   // assume all 3 diodes are on very top row
                    double anand_pcx = anand.inPcbX (anand_in__pc);
                    double anand_pcy = anand.inPcbY (anand_in__pc);
                    assert (anand_pcy == cnand_pcy);
                    _pcnet.putseg (ps, anand_pcx,       enand_pcy,       anand_pcx + 0.5, enand_pcy + 0.5, "F.Cu");
                    _pcnet.putseg (ps, anand_pcx + 0.5, enand_pcy + 0.5, cnand_pcx - 0.5, enand_pcy + 0.5, "F.Cu");
                    _pcnet.putseg (ps, cnand_pcx - 0.5, enand_pcy + 0.5, cnand_pcx,       enand_pcy,       "F.Cu");
                }
            }

            // common preset inputs
            if ((fnand_in__ps == 2) && (dnand_in__ps == 2)) {
                double dnand_psx = dnand.inPcbX (dnand_in__ps);
                double dnand_psy = dnand.inPcbY (dnand_in__ps);
                double fnand_psx = fnand.inPcbX (fnand_in__ps);
                double fnand_psy = fnand.inPcbY (fnand_in__ps);
                assert dnand_psy == fnand_psy;  // horizontal connection links the two
                _psnet.putseg (ps, dnand_psx,       fnand_psy,       dnand_psx + 0.5, fnand_psy - 0.5, "F.Cu");
                _psnet.putseg (ps, dnand_psx + 0.5, fnand_psy - 0.5, fnand_psx - 0.5, fnand_psy - 0.5, "F.Cu");
                _psnet.putseg (ps, fnand_psx - 0.5, fnand_psy - 0.5, fnand_psx,       fnand_psy,       "F.Cu");
            }
        }
    }

    // holds the 6 miniature cells that make up a D flipflop
    //  Da1 Ra1 Ra2     Dc1 Rc1 Rc2     De1 Re1 Re2
    //  Da2 Da2 Da3 Qa  Dc2 Dc3 Dc4 Qc  De2 De3 De4 Qd
    //
    //  Db1 Rb1 Rb2 Qb  Dd1 Rd1 Rd2 Qd  Df1 Rf1 Rf2 Qf
    //  Db2 Db3 Db4     Dd2 Dd3 Dd4     Df2 Df3 Df4
    //          bd3
    private class DFFMini extends DFFCell implements PreDrawable {
        public MiniNand2 amini;     // D input -> d1 cathode
        public MiniNand2 bmini;     // clock   -> d2 cathode
        public MiniNand2 cmini;     // clock   -> d1 cathode
        public MiniNand2 dmini;     //
        public MiniNand2 emini;     // _Q output
        public MiniNand2 fmini;     //  Q output

        public Comp.Diode bd3;

        private boolean flat;
        private Network clknet;
        private Network datnet;

        public DFFMini (GenCtx genctx, boolean flat, int rbit)
        {
            this.flat = flat;

            amini = new MiniNand2 (genctx, "a." + rbit + "." + name, flat);
            bmini = new MiniNand2 (genctx, "b." + rbit + "." + name, flat);
            cmini = new MiniNand2 (genctx, "c." + rbit + "." + name, flat);
            dmini = new MiniNand2 (genctx, "d." + rbit + "." + name, flat);
            emini = new MiniNand2 (genctx, "e." + rbit + "." + name, flat);
            fmini = new MiniNand2 (genctx, "f." + rbit + "." + name, flat);

            bd3   = new Comp.Diode (genctx, null, "d3.b." + rbit + "." + name, true, flat);
            bmini.andnet.addConn (bd3, Comp.Diode.PIN_A);

            bmini.outnet.addConn (amini.d2, Comp.Diode.PIN_C);

            amini.outnet.addConn (bmini.d1, Comp.Diode.PIN_C);
            cmini.outnet.addConn (bd3,      Comp.Diode.PIN_C);

            dmini.outnet.addConn (cmini.d2, Comp.Diode.PIN_C);

            cmini.outnet.addConn (dmini.d1, Comp.Diode.PIN_C);
            amini.outnet.addConn (dmini.d2, Comp.Diode.PIN_C);

            bmini.outnet.addConn (emini.d1, Comp.Diode.PIN_C);
            fmini.outnet.addConn (emini.d2, Comp.Diode.PIN_C);

            emini.outnet.addConn (fmini.d1, Comp.Diode.PIN_C);
            cmini.outnet.addConn (fmini.d2, Comp.Diode.PIN_C);

            eoutnet = emini.outnet;
            foutnet = fmini.outnet;

            // we pre-draw all the internal connections
            genctx.predrawables.add (this);
        }

        @Override  // DFFCell
        public void setInputs (GenCtx genctx, int rbit, Network dnet, Network clknet, Network _pcnet, Network _psnet)
        {
            if (! _pcnet.name.equals ("VCC") || ! _psnet.name.equals ("VCC")) {
                throw new IllegalArgumentException ("mini DFF " + name + " must have _PC and _PS inputs tied to VCC");
            }

            dnet.addConn   (amini.d1, Comp.Diode.PIN_C);
            clknet.addConn (bmini.d2, Comp.Diode.PIN_C);
            clknet.addConn (cmini.d1, Comp.Diode.PIN_C);

            this.clknet = clknet;
            this.datnet = dnet;
        }

        @Override  // Placeable
        public int getHeight ()
        {
            return  7;
        }

        @Override  // Placeable
        public int getWidth ()
        {
            return flat ? 39 : 30;
        }

        @Override  // Placeable
        public void setPosXY (double leftx, double topy)
        {
            int dx = flat ? 13 : 10;
            int x3 = flat ?  3 :  2;
            amini.setPosXY (leftx +  0, topy + 0);
            bmini.setPosXY (leftx +  0, topy + 3);
            bd3  .setPosXY (leftx + x3, topy + 5);
            cmini.setPosXY (leftx + dx, topy + 0);
            dmini.setPosXY (leftx + dx, topy + 3);
            emini.setPosXY (leftx + 2 * dx, topy + 0);
            fmini.setPosXY (leftx + 2 * dx, topy + 3);
        }

        @Override  // Placeable
        public double getPosX ()
        {
            return amini.getPosX ();
        }

        @Override  // Placeable
        public double getPosY ()
        {
            return amini.getPosY ();
        }

        @Override  // Placeable
        public Network[] getExtNets ()
        {
            return new Network[] { emini.outnet, fmini.outnet, clknet, datnet };
        }

        @Override  // PreDrawable
        public void preDraw (PrintStream ps)
        {
            double leftx = amini.getPosX ();
            double topy  = amini.getPosY ();

            if (flat) {

                // a out -> b in
                amini.outnet.putseg (ps, leftx +  9.0, topy + 1.0, leftx +  8.5, topy + 1.5, "B.Cu");
                amini.outnet.putseg (ps, leftx +  8.5, topy + 1.5, leftx +  8.5, topy + 4.5, "B.Cu");
                amini.outnet.putvia (ps, leftx +  8.5, topy + 4.5, "B.Cu", "F.Cu");
                amini.outnet.putseg (ps, leftx +  8.5, topy + 4.5, leftx +  1.5, topy + 4.5, "F.Cu");
                amini.outnet.putseg (ps, leftx +  1.5, topy + 4.5, leftx +  1.0, topy + 4.0, "F.Cu");

                // a out -> d in
                amini.outnet.putseg (ps, leftx +  8.5, topy + 4.5, leftx + 13.5, topy + 4.5, "F.Cu");
                amini.outnet.putseg (ps, leftx + 13.5, topy + 4.5, leftx + 14.0, topy + 5.0, "F.Cu");

                // b out -> a in
                bmini.outnet.putseg (ps, leftx +  9.0, topy + 4.0, leftx +  9.5, topy + 3.5, "B.Cu");
                bmini.outnet.putseg (ps, leftx +  9.5, topy + 3.5, leftx +  9.5, topy + 1.5, "B.Cu");
                bmini.outnet.putvia (ps, leftx +  9.5, topy + 1.5, "B.Cu", "F.Cu");
                bmini.outnet.putseg (ps, leftx +  9.5, topy + 1.5, leftx +  1.5, topy + 1.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx +  1.5, topy + 1.5, leftx +  1.0, topy + 2.0, "F.Cu");

                // b out -> e in
                bmini.outnet.putseg (ps, leftx +  9.5, topy + 1.5, leftx + 26.5, topy + 1.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx + 26.5, topy + 1.5, leftx + 27.0, topy + 1.0, "F.Cu");

                // c out -> d in
                cmini.outnet.putseg (ps, leftx + 22.0, topy + 1.0, leftx + 21.5, topy + 1.5, "B.Cu");
                cmini.outnet.putseg (ps, leftx + 21.5, topy + 1.5, leftx + 21.5, topy + 5.5, "B.Cu");
                cmini.outnet.putvia (ps, leftx + 21.5, topy + 5.5, "B.Cu", "F.Cu");
                cmini.outnet.putseg (ps, leftx + 21.5, topy + 5.5, leftx + 14.5, topy + 5.5, "F.Cu");
                cmini.outnet.putvia (ps, leftx + 14.5, topy + 5.5, "F.Cu", "B.Cu");
                cmini.outnet.putseg (ps, leftx + 14.5, topy + 5.5, leftx + 14.5, topy + 4.5, "B.Cu");
                cmini.outnet.putseg (ps, leftx + 14.5, topy + 4.5, leftx + 14.0, topy + 4.0, "B.Cu");

                // c out -> b in (3rd diode)
                cmini.outnet.putseg (ps, leftx + 14.5, topy + 5.5, leftx +  6.5, topy + 5.5, "F.Cu");
                cmini.outnet.putseg (ps, leftx +  6.5, topy + 5.5, leftx +  6.0, topy + 6.0, "F.Cu");

                // c out -> f in
                cmini.outnet.putseg (ps, leftx + 21.5, topy + 5.5, leftx + 26.5, topy + 5.5, "F.Cu");
                cmini.outnet.putseg (ps, leftx + 26.5, topy + 5.5, leftx + 27.0, topy + 5.0, "F.Cu");

                // d out -> c in
                dmini.outnet.putseg (ps, leftx + 22.0, topy + 4.0, leftx + 21.5, topy + 3.5, "F.Cu");
                dmini.outnet.putseg (ps, leftx + 21.5, topy + 3.5, leftx + 14.5, topy + 3.5, "F.Cu");
                dmini.outnet.putvia (ps, leftx + 14.5, topy + 3.5, "F.Cu", "B.Cu");
                dmini.outnet.putseg (ps, leftx + 14.5, topy + 3.5, leftx + 14.5, topy + 2.5, "B.Cu");
                dmini.outnet.putseg (ps, leftx + 14.5, topy + 2.5, leftx + 14.0, topy + 2.0, "B.Cu");

                // e out -> f in
                emini.outnet.putseg (ps, leftx + 35.0, topy + 1.0, leftx + 34.5, topy + 1.5, "B.Cu");
                emini.outnet.putseg (ps, leftx + 34.5, topy + 1.5, leftx + 34.5, topy + 4.5, "B.Cu");
                emini.outnet.putvia (ps, leftx + 34.5, topy + 4.5, "B.Cu", "F.Cu");
                emini.outnet.putseg (ps, leftx + 34.5, topy + 4.5, leftx + 27.5, topy + 4.5, "F.Cu");
                emini.outnet.putseg (ps, leftx + 27.5, topy + 4.5, leftx + 27.0, topy + 4.0, "F.Cu");

                // f out -> e in
                fmini.outnet.putseg (ps, leftx + 35.0, topy + 4.0, leftx + 34.5, topy + 3.5, "F.Cu");
                fmini.outnet.putseg (ps, leftx + 34.5, topy + 3.5, leftx + 27.5, topy + 3.5, "F.Cu");
                fmini.outnet.putvia (ps, leftx + 27.5, topy + 3.5, "F.Cu", "B.Cu");
                fmini.outnet.putseg (ps, leftx + 27.5, topy + 3.5, leftx + 27.5, topy + 2.5, "B.Cu");
                fmini.outnet.putseg (ps, leftx + 27.5, topy + 2.5, leftx + 27.0, topy + 2.0, "B.Cu");

                // 3rd diode
                bmini.andnet.putseg (ps, leftx + 4.0, topy + 5.0, leftx + 4.0, topy + 6.0, "B.Cu");

                // common clock input to b and c
                clknet.putseg (ps, leftx +  1.0, topy + 5.0, leftx +  1.5, topy + 4.5, "B.Cu");
                clknet.putseg (ps, leftx +  1.5, topy + 4.5, leftx +  1.5, topy + 3.5, "B.Cu");
                clknet.putvia (ps, leftx +  1.5, topy + 3.5, "B.Cu", "F.Cu");
                clknet.putseg (ps, leftx +  1.5, topy + 3.5, leftx + 13.5, topy + 3.5, "F.Cu");
                clknet.putvia (ps, leftx + 13.5, topy + 3.5, "F.Cu", "B.Cu");
                clknet.putseg (ps, leftx + 13.5, topy + 3.5, leftx + 13.5, topy + 1.5, "B.Cu");
                clknet.putseg (ps, leftx + 13.5, topy + 1.5, leftx + 14.0, topy + 1.0, "B.Cu");
            } else {

                // a out -> b in
                amini.outnet.putseg (ps, leftx +  8.0, topy + 1.0, leftx +  8.0, topy + 2.0, "B.Cu");
                amini.outnet.putseg (ps, leftx +  8.0, topy + 2.0, leftx +  7.5, topy + 2.5, "B.Cu");
                amini.outnet.putseg (ps, leftx +  7.5, topy + 2.5, leftx +  7.5, topy + 4.5, "B.Cu");
                amini.outnet.putvia (ps, leftx +  7.5, topy + 4.5, "B.Cu", "F.Cu");
                amini.outnet.putseg (ps, leftx +  7.5, topy + 4.5, leftx +  1.5, topy + 4.5, "F.Cu");
                amini.outnet.putseg (ps, leftx +  1.5, topy + 4.5, leftx +  1.0, topy + 4.0, "F.Cu");

                // a out -> d in
                amini.outnet.putseg (ps, leftx +  7.5, topy + 4.5, leftx +  8.0, topy + 5.0, "F.Cu");
                amini.outnet.putseg (ps, leftx +  8.0, topy + 5.0, leftx + 11.0, topy + 5.0, "F.Cu");

                // b out -> a in
                bmini.outnet.putseg (ps, leftx +  6.0, topy + 4.0, leftx +  5.5, topy + 3.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx +  5.5, topy + 3.5, leftx +  2.5, topy + 3.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx +  2.5, topy + 3.5, leftx +  2.0, topy + 3.0, "F.Cu");
                bmini.outnet.putseg (ps, leftx +  2.0, topy + 3.0, leftx +  1.5, topy + 2.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx +  1.5, topy + 2.5, leftx +  1.0, topy + 2.0, "F.Cu");

                // b out -> e in
                bmini.outnet.putseg (ps, leftx +  8.0, topy + 4.0, leftx +  8.5, topy + 3.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx +  8.5, topy + 3.5, leftx + 20.5, topy + 3.5, "F.Cu");
                bmini.outnet.putseg (ps, leftx + 20.5, topy + 3.5, leftx + 21.0, topy + 3.0, "F.Cu");
                bmini.outnet.putvia (ps, leftx + 21.0, topy + 3.0, "F.Cu", "B.Cu");
                bmini.outnet.putseg (ps, leftx + 21.0, topy + 3.0, leftx + 21.5, topy + 2.5, "B.Cu");
                bmini.outnet.putseg (ps, leftx + 21.5, topy + 2.5, leftx + 21.5, topy + 1.5, "B.Cu");
                bmini.outnet.putseg (ps, leftx + 21.5, topy + 1.5, leftx + 21.0, topy + 1.0, "B.Cu");

                // c out -> d in
                cmini.outnet.putseg (ps, leftx + 18.0, topy + 1.0, leftx + 18.0, topy + 2.0, "B.Cu");
                cmini.outnet.putseg (ps, leftx + 18.0, topy + 2.0, leftx + 17.5, topy + 2.5, "B.Cu");
                cmini.outnet.putvia (ps, leftx + 17.5, topy + 2.5, "B.Cu", "F.Cu");
                cmini.outnet.putseg (ps, leftx + 17.5, topy + 2.5, leftx + 11.5, topy + 2.5, "F.Cu");
                cmini.outnet.putvia (ps, leftx + 11.5, topy + 2.5, "F.Cu", "B.Cu");
                cmini.outnet.putseg (ps, leftx + 11.5, topy + 2.5, leftx + 11.5, topy + 3.5, "B.Cu");
                cmini.outnet.putseg (ps, leftx + 11.5, topy + 3.5, leftx + 11.0, topy + 4.0, "B.Cu");

                // c out -> b in (3rd diode)
                cmini.outnet.putseg (ps, leftx + 11.0, topy + 4.0, leftx + 11.5, topy + 4.5, "B.Cu");
                cmini.outnet.putseg (ps, leftx + 11.5, topy + 4.5, leftx + 11.5, topy + 5.5, "B.Cu");
                cmini.outnet.putvia (ps, leftx + 11.5, topy + 5.5, "B.Cu", "F.Cu");
                cmini.outnet.putseg (ps, leftx + 11.5, topy + 5.5, leftx +  4.5, topy + 5.5, "F.Cu");
                cmini.outnet.putseg (ps, leftx +  4.5, topy + 5.5, leftx +  4.0, topy + 6.0, "F.Cu");

                // c out -> f in
                cmini.outnet.putseg (ps, leftx + 18.0, topy + 2.0, leftx + 18.5, topy + 2.5, "B.Cu");
                cmini.outnet.putseg (ps, leftx + 18.5, topy + 2.5, leftx + 18.5, topy + 5.0, "B.Cu");
                cmini.outnet.putvia (ps, leftx + 18.5, topy + 5.0, "B.Cu", "F.Cu");
                cmini.outnet.putseg (ps, leftx + 18.5, topy + 5.0, leftx + 21.0, topy + 5.0, "F.Cu");

                // d out -> c in
                dmini.outnet.putseg (ps, leftx + 16.0, topy + 4.0, leftx + 15.5, topy + 3.5, "B.Cu");
                dmini.outnet.putseg (ps, leftx + 15.5, topy + 3.5, leftx + 15.5, topy + 1.5, "B.Cu");
                dmini.outnet.putvia (ps, leftx + 15.5, topy + 1.5, "B.Cu", "F.Cu");
                dmini.outnet.putseg (ps, leftx + 15.5, topy + 1.5, leftx + 11.5, topy + 1.5, "F.Cu");
                dmini.outnet.putseg (ps, leftx + 11.5, topy + 1.5, leftx + 11.0, topy + 2.0, "F.Cu");

                // e out -> f in
                emini.outnet.putseg (ps, leftx + 28.0, topy + 1.0, leftx + 28.0, topy + 2.0, "B.Cu");
                emini.outnet.putseg (ps, leftx + 28.0, topy + 2.0, leftx + 27.5, topy + 2.5, "B.Cu");
                emini.outnet.putseg (ps, leftx + 27.5, topy + 2.5, leftx + 27.5, topy + 3.5, "B.Cu");
                emini.outnet.putvia (ps, leftx + 27.5, topy + 3.5, "B.Cu", "F.Cu");
                emini.outnet.putseg (ps, leftx + 27.5, topy + 3.5, leftx + 21.5, topy + 3.5, "F.Cu");
                emini.outnet.putseg (ps, leftx + 21.5, topy + 3.5, leftx + 21.0, topy + 4.0, "F.Cu");

                // f out -> e in
                fmini.outnet.putseg (ps, leftx + 26.0, topy + 4.0, leftx + 25.5, topy + 3.5, "B.Cu");
                fmini.outnet.putseg (ps, leftx + 25.5, topy + 3.5, leftx + 25.5, topy + 2.5, "B.Cu");
                fmini.outnet.putvia (ps, leftx + 25.5, topy + 2.5, "B.Cu", "F.Cu");
                fmini.outnet.putseg (ps, leftx + 25.5, topy + 2.5, leftx + 21.5, topy + 2.5, "F.Cu");
                fmini.outnet.putseg (ps, leftx + 21.5, topy + 2.5, leftx + 21.0, topy + 2.0, "F.Cu");

                // 3rd diode
                bmini.andnet.putseg (ps, leftx + 3.0, topy + 5.0, leftx + 3.0, topy + 6.0, "B.Cu");

                // common clock input to b and c
                clknet.putseg (ps, leftx +  1.0, topy + 5.0, leftx +  1.5, topy + 4.5, "B.Cu");
                clknet.putseg (ps, leftx +  1.5, topy + 4.5, leftx +  1.5, topy + 1.5, "B.Cu");
                clknet.putvia (ps, leftx +  1.5, topy + 1.5, "B.Cu", "F.Cu");
                clknet.putseg (ps, leftx +  1.5, topy + 1.5, leftx + 10.5, topy + 1.5, "F.Cu");
                clknet.putseg (ps, leftx + 10.5, topy + 1.5, leftx + 11.0, topy + 1.0, "F.Cu");
            }
        }
    }
}
