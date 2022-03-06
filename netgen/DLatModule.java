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
 * Built-in latch module.
 */

import java.io.PrintStream;

public class DLatModule extends Module {
    private int width;              // number of latches
    private String instvarsuf;

    private OpndLhs  q;
    private OpndLhs _q;
    private OpndLhs  d;
    private OpndLhs  g;

    private DLatCell[] dlatcells;

    public DLatModule (String instvarsuf, int width)
    {
        name = "DLat" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.width = width;

         q = new  QOutput ( "Q" + instvarsuf, 0);
        _q = new _QOutput ("_Q" + instvarsuf, 1);

        if (instvarsuf.equals ("")) {
            d = new IParam ( "D", 2);
            g = new IParam ( "G", 3);
        } else {
            d = new OpndLhs ( "D" + instvarsuf);
            g = new OpndLhs ( "G" + instvarsuf);
        }

        q.hidim = _q.hidim = d.hidim = this.width - 1;

        params = new OpndLhs[] { q, _q, d, g };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // this gets the width token after the module name and before the (
    // eg, inst : DLat 16 ( ... );
    //                 ^ token is here
    //                    ^ return token here
    // by default, we make a scalar flipflop
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        int biwidth = 1;   // put 1 for scalar
        if (token instanceof Token.Int) {
            Token.Int tokint = (Token.Int) token;
            long width = tokint.value;
            if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
            biwidth = (int) width;
            token = token.next;
        }
        instconfig_r[0] = biwidth;
        return token;
    }

    // make a new DLat module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        DLatModule dlatinst = new DLatModule (instvarsuf, (Integer) instconfig);
        target.variables.putAll (dlatinst.variables);
        return dlatinst.params;
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
            // generate circuit for latch
            getCircuit (genctx, rbit);

            // Q output comes from the C gate
            return dlatcells[rbit].cnand.outnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DLat", this, rbit);
            pp.inclevel ();
            d.printProp (pp, rbit);
            pp.declevel ();
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
            // generate circuit for latch
            getCircuit (genctx, rbit);

            // _Q output comes from the D gate
            return dlatcells[rbit].dnand.outnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DLat", this, rbit);
            pp.inclevel ();
            d.printProp (pp, rbit);
            pp.declevel ();
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
        if (dlatcells == null) {
            dlatcells = new DLatCell[width];
        }

        // see if we have already generated circuitry for this bitslice
        if (dlatcells[rbit] == null) {

            // make 4 cells for this bitslice of the latch
            DLatCell dlatcell = new DLatCell ();
            MiniNand2 anand, bnand, cnand, dnand;
            dlatcell.rbit  = rbit;
            dlatcell.anand = new MiniNand2 (genctx, "a." + rbit + "." + name, true);
            dlatcell.bnand = new MiniNand2 (genctx, "b." + rbit + "." + name, true);
            dlatcell.cnand = new MiniNand2 (genctx, "c." + rbit + "." + name, true);
            dlatcell.dnand = new MiniNand2 (genctx, "d." + rbit + "." + name, true);
            dlatcells[rbit] = dlatcell;

            // set up to place flipflop on circuit board
            // and it can also pre-draw gate interconnections
            genctx.addPlaceable (dlatcell);
            genctx.predrawables.add (dlatcell);

            // get networks all our inputs are connected to
            dlatcell.dnet = d.generate (genctx, rbit % d.busWidth ());
            dlatcell.gnet = g.generate (genctx, rbit % g.busWidth ());

            // connect the cells and inputs into a latch arrangement
            //   data -> a.d1   a -> c.d1   c -> q
            //   gate -> a.d2   d -> c.d2
            //      a -> b.d1   c -> d.d1   d -> _q
            //   gate -> b.d2   b -> d.d2
            dlatcell.dnet.addConn (dlatcell.anand.d1, Comp.Diode.PIN_C);
            dlatcell.gnet.addConn (dlatcell.anand.d2, Comp.Diode.PIN_C);
            dlatcell.gnet.addConn (dlatcell.bnand.d2, Comp.Diode.PIN_C);

            dlatcell.anand.outnet.addConn (dlatcell.cnand.d1, Comp.Diode.PIN_C);
            dlatcell.anand.outnet.addConn (dlatcell.bnand.d1, Comp.Diode.PIN_C);
            dlatcell.bnand.outnet.addConn (dlatcell.dnand.d2, Comp.Diode.PIN_C);
            dlatcell.cnand.outnet.addConn (dlatcell.dnand.d1, Comp.Diode.PIN_C);
            dlatcell.dnand.outnet.addConn (dlatcell.cnand.d2, Comp.Diode.PIN_C);
        }
    }

    //// SIMULATION ////

    private void stepSimulator (int t)
            throws HaltedException
    {
        // add extra gate delay for 4-transistor configuration
        // ie, the input gates see inputs 1 more gate delay ago
        int t1 = t - OpndRhs.SIM_PROP;

        // get state all up front in case of recursion
        long dval = d.getSmearedInput (t1, width);
        long gval = g.getSmearedInput (t1, width);
        long qval = q.getSimOutput (t - 1);

        // process flipflops one at a time
        for (int bitn = 0; bitn < width; bitn ++) {
            long longm = 0x100000001L << bitn;
            long onem  = 0x100000000L << bitn;

            // if G is one, Q follows D
            // else, Q keeps old value
            if ((gval & onem) != 0) {
                qval = (qval & ~longm) | (dval & longm);
            }
        }

        // set outputs for this timestep
         q.setSimOutput (t, qval);
        _q.setSimOutput (t, (qval << 32) | (qval >>> 32));
    }

    // holds the 4 mininand2s that make up a single-bit D latch
    private class DLatCell extends Placeable implements PreDrawable {
        public int rbit;
        public MiniNand2 anand;     // D,G input
        public MiniNand2 bnand;     //  G input
        public MiniNand2 cnand;     //  Q output
        public MiniNand2 dnand;     // _Q output

        public Network dnet;        // what drives the latch data input
        public Network gnet;        // what drives the latch gate input

        private int abwidth;
        private int cdwidth;

        private int acheight;
        private int bdheight;

        @Override  // Placeable
        public String getName ()
        {
            return rbit + "." + name;
        }

        @Override  // Placeable
        public void flipIt ()
        { }

        @Override  // Placeable
        public int getHeight ()
        {
            arrange ();
            return acheight + bdheight;
        }

        @Override  // Placeable
        public int getWidth ()
        {
            arrange ();
            return abwidth + cdwidth;
        }

        @Override  // Placeable
        public void setPosXY (double leftx, double topy)
        {
            arrange ();

            anand.setPosXY (leftx, topy);
            bnand.setPosXY (leftx, topy + acheight);

            cnand.setPosXY (leftx + abwidth, topy);
            dnand.setPosXY (leftx + abwidth, topy + acheight);
        }

        @Override  // Placeable
        public double getPosX ()
        {
            return cnand.getPosX () - abwidth;
        }

        @Override  // Placeable
        public double getPosY ()
        {
            return cnand.getPosY ();
        }

        private void arrange ()
        {
            if (abwidth == 0) {
                abwidth = Math.max (anand.getWidth (), bnand.getWidth ());
                cdwidth = Math.max (cnand.getWidth (), dnand.getWidth ());

                acheight = Math.max (anand.getHeight (), cnand.getHeight ());
                bdheight = Math.max (bnand.getHeight (), dnand.getHeight ());
            }
        }

        @Override  // Placeable
        public Network[] getExtNets ()
        {
            return new Network[] { cnand.outnet, dnand.outnet, gnet, dnet };
        }

        @Override  // PreDrawable
        public void preDraw (PrintStream ps)
        {
            double leftx = anand.getPosX ();
            double topy  = anand.getPosY ();

            Network aoutnet = anand.outnet;
            Network boutnet = bnand.outnet;
            Network coutnet = cnand.outnet;
            Network doutnet = dnand.outnet;

            // a out -> c in
            aoutnet.putseg (ps, leftx + 11.0, topy + 1.0, leftx + 11.5, topy + 1.5, "F.Cu");
            aoutnet.putseg (ps, leftx + 11.5, topy + 1.5, leftx + 13.5, topy + 1.5, "F.Cu");
            aoutnet.putseg (ps, leftx + 13.5, topy + 1.5, leftx + 14.0, topy + 1.0, "F.Cu");

            // a out -> b in
            aoutnet.putseg (ps, leftx +  9.0, topy + 1.0, leftx +  8.5, topy + 1.5, "B.Cu");
            aoutnet.putseg (ps, leftx +  8.5, topy + 1.5, leftx +  8.5, topy + 3.5, "B.Cu");
            aoutnet.putvia (ps, leftx +  8.5, topy + 3.5, "F.Cu", "B.Cu");
            aoutnet.putseg (ps, leftx +  8.5, topy + 3.5, leftx +  1.5, topy + 3.5, "F.Cu");
            aoutnet.putseg (ps, leftx +  1.5, topy + 3.5, leftx +  1.0, topy + 4.0, "F.Cu");

            // b out -> d in
            boutnet.putseg (ps, leftx + 11.0, topy + 4.0, leftx + 11.5, topy + 4.5, "F.Cu");
            boutnet.putseg (ps, leftx + 11.5, topy + 4.5, leftx + 13.5, topy + 4.5, "F.Cu");
            boutnet.putseg (ps, leftx + 13.5, topy + 4.5, leftx + 14.0, topy + 5.0, "F.Cu");

            // c out -> d in
            coutnet.putseg (ps, leftx + 22.0, topy + 1.0, leftx + 21.5, topy + 1.5, "F.Cu");
            coutnet.putseg (ps, leftx + 21.5, topy + 1.5, leftx + 14.5, topy + 1.5, "F.Cu");
            coutnet.putvia (ps, leftx + 14.5, topy + 1.5, "F.Cu", "B.Cu");
            coutnet.putseg (ps, leftx + 14.5, topy + 1.5, leftx + 14.5, topy + 3.5, "B.Cu");
            coutnet.putseg (ps, leftx + 14.5, topy + 3.5, leftx + 14.0, topy + 4.0, "B.Cu");

            // d out -> c in
            doutnet.putseg (ps, leftx + 22.0, topy + 4.0, leftx + 21.5, topy + 3.5, "B.Cu");
            doutnet.putseg (ps, leftx + 21.5, topy + 3.5, leftx + 21.5, topy + 2.5, "B.Cu");
            doutnet.putvia (ps, leftx + 21.5, topy + 2.5, "B.Cu", "F.Cu");
            doutnet.putseg (ps, leftx + 21.5, topy + 2.5, leftx + 14.5, topy + 2.5, "F.Cu");
            doutnet.putseg (ps, leftx + 14.5, topy + 2.5, leftx + 14.0, topy + 2.0, "F.Cu");

            // tie two gate inputs (a & b) together
            gnet.putseg (ps, leftx + 1.0, topy + 2.0, leftx + 1.5, topy + 2.5, "B.Cu");
            gnet.putseg (ps, leftx + 1.5, topy + 2.5, leftx + 1.5, topy + 4.5, "B.Cu");
            gnet.putseg (ps, leftx + 1.5, topy + 4.5, leftx + 1.0, topy + 5.0, "B.Cu");
        }
    }
}
