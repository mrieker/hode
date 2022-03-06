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
 * Standard cell circuit
 *
                         (VCC)                                             (VCC)
                           |                                                 |
                           _                                                 _
                         orres                                             rcoll
                           _                                                 _
                           |                                                 |
                           |                                              (outnet)
                           |                                                 |
                           |                                                /
    (innet)----[|<]----(andnet)----[>|]----(ornet)----[>|]----(basenet)----|
              andiode             ordiode            basedio      |         \
                                                                  _          |
                                                                rbase      (GND)
                                                                  _
                                                                  |
                                                                (GND)
*/

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.LinkedList;

public class StandCell extends Placeable {
    private LinkedList<Comp> col1, col2, col3, col4;
    private ArrayList<ArrayList<Network>> inputss;
    private boolean closed;
    private Comp.ByCap bycap;
    private Comp.Diode basedio;
    private Comp.Diode[] ordiodes;
    private Comp.Diode[][] andiodess;
    private Comp.Resis rbase;
    private Comp.Resis rcoll;
    private Comp.Resis[] orresiss;
    private Comp.SmLed smled;
    private Comp.Trans trans;
    private GenCtx genctx;
    private Network ornet;
    private Network outnet;
    private Network[] andnets;
    public  String name;

    public StandCell (GenCtx genctx, String name)
    {
        this (genctx, name, null, false);
    }

    public StandCell (GenCtx genctx, String name, String lighted, boolean opencoll)
    {
        this.genctx = genctx;
        this.name   = name;

        inputss = new ArrayList<> ();

        // reserve some room on circuit board in the form of a small 4 column table
        //  col1: 'and' diodes
        //  col2: first 'or' resistor, 'or' diodes
        //  col3: collector resistor, base diode, second and rest of base resistors
        //  col4: optional led, transistor, base resistor
        col1 = new LinkedList<Comp> ();
        col2 = new LinkedList<Comp> ();
        col3 = new LinkedList<Comp> ();
        col4 = new LinkedList<Comp> ();

        // allocate and wire everything up to point of 'ornet' in above diagram
        ornet  = new Network (genctx, "O." + name);
        outnet = new Network (genctx, "Q." + name);

        Network lednet = null;
        smled = null;
        if (lighted != null) {
            lednet = new Network (genctx, "L." + name);
            smled  = new Comp.SmLed (genctx, col4, "L." + name, lighted, true);
        }

        trans = new Comp.Trans (genctx, col4, "Q." + name);
        bycap = new Comp.ByCap (genctx, null, "BC." + name);
        rbase = new Comp.Resis (genctx, col4, "RB." + name, Comp.Resis.R_BAS, false, true);
        rcoll = opencoll ? null : new Comp.Resis (genctx, null, "RC." + name, (lighted != null) ? Comp.Resis.R_LED : Comp.Resis.R_COL, false, true);
        col3.add (rcoll);
        basedio          = new Comp.Diode (genctx, col3, "D." + name, true, true);

        Network basenet  = new Network (genctx, "B." + name);
        Network gndnet   = genctx.nets.get ("GND");
        Network vccnet   = genctx.nets.get ("VCC");

        gndnet.addConn (trans, Comp.Trans.PIN_E);
        gndnet.addConn (bycap, Comp.ByCap.PIN_Z);
        gndnet.addConn (rbase, Comp.Resis.PIN_A);
        vccnet.addConn (bycap, Comp.ByCap.PIN_Y);
        if (! opencoll) vccnet.addConn (rcoll, Comp.Resis.PIN_C);

        if (lighted != null) {
            basenet.preWire (trans, Comp.Trans.PIN_B, rbase,   Comp.Resis.PIN_C);
            basenet.preWire (trans, Comp.Trans.PIN_B, basedio, Comp.Diode.PIN_C);
        } else {
            basenet.preWire (trans, Comp.Trans.PIN_B, rbase,   Comp.Resis.PIN_C);
            basenet.preWire (rbase, Comp.Resis.PIN_C, basedio, Comp.Diode.PIN_C);
        }

        if (lighted != null) {
            lednet.preWire (rcoll, Comp.Resis.PIN_A, smled, Comp.SmLed.PIN_A);
            outnet.preWire (smled, Comp.SmLed.PIN_C, trans, Comp.Trans.PIN_C);
        } else if (opencoll) {
            outnet.addConn (trans, Comp.Trans.PIN_C);
        } else {
            outnet.preWire (rcoll, Comp.Resis.PIN_A, trans, Comp.Trans.PIN_C);
        }
    }

    // get the output of this cell
    public Network getOutput ()
    {
        return outnet;
    }

    // add an input to this cell
    //  input:
    //   orindex = which input to the NOR gate this is for (starting with 0)
    //   innet = input to the AND gate connected to the NOR input
    //  output:
    //   innet added to cell
    //   returns innet's andindex
    public int addInput (int orindex, Network innet)
    {
        assert ! closed;

        // ignore any inputs hardwired to VCC
        if (innet.name.equals ("VCC")) return -1;

        // make sure there is an array to capture inputs for this branch of the NOR gate
        while (inputss.size () <= orindex) {
            inputss.add (new ArrayList<> ());
        }

        // if an input is hardwired to GND, disable collecting inputs to the AND gate
        if (innet.name.equals ("GND")) {
            inputss.set (orindex, null);
            return -1;
        }

        // get collection of inputs to this AND gate
        // if null, there is an hardwired GND input so don't bother with this new one
        ArrayList<Network> inputs = inputss.get (orindex);
        if (inputs == null) return -1;

        // AND gate not grounded, add new input
        inputs.add (innet);

        return inputs.size () - 1;
    }

    // no more inputs to add, create rest of components and wire them up
    public void close ()
    {
        assert ! closed;
        closed = true;

        // strip out grounded ANDs
        int totalinputs = 0;
        for (int i = inputss.size (); -- i >= 0;) {
            ArrayList<Network> inputs = inputss.get (i);
            if (inputs == null) inputss.remove (i);
            else totalinputs += inputs.size ();
        }

        // save all the input diodes
        andiodess = new Comp.Diode[inputss.size()][];

        // if no inputs, all inputs to OR gate are zero, so tie base diode to ground
        //TODO someday optimize whole transistor out
        if (inputss.size () == 0) {
            Network gndnet = genctx.nets.get ("GND");
            gndnet.addConn (basedio, Comp.Diode.PIN_A);
            return;
        }

        // one end of resistors gets tied to VCC
        Network vccnet = genctx.nets.get ("VCC");

        // special layout for simple NAND gate in case it has lots of inputs

        if (inputss.size () == 1) {

            // get list of inputs to the NAND gate
            ArrayList<Network> inputs = inputss.get (0);

            // split the diodes between 1st and 3rd columns
            int ninputs = inputs.size ();
            int col3ins = ninputs / 2 - 1;
            if (col3ins >= 0) {
                andiodess[0] = new Comp.Diode[ninputs];

                // this wires all the anodes together and one end of the resistor
                Network andnet = new Network (genctx, "A1." + name);

                // put the resistor and diode in 2nd column
                Comp.Resis orres = new Comp.Resis (genctx, col2, "R1." + name, Comp.Resis.R_OR, true, true);
                Comp.Diode ordio = new Comp.Diode (genctx, col2, "D1." + name, true, true);

                // other end of resistor goes to VCC
                vccnet.addConn (orres, Comp.Resis.PIN_C);

                // connect diode and resistor
                ornet.preWire (basedio, Comp.Diode.PIN_A, ordio, Comp.Diode.PIN_C);
                andnet.preWire (orres, Comp.Resis.PIN_A, ordio, Comp.Diode.PIN_A);

                // set up first two input diodes and connect to their driving circuits
                Comp.Diode andio1 = new Comp.Diode (genctx, col1, "D1_1." + name, false, true);
                Comp.Diode andio2 = new Comp.Diode (genctx, col1, "D1_2." + name, false, true);
                inputs.get (0).addConn (andio1, Comp.Diode.PIN_C, 1);
                inputs.get (1).addConn (andio2, Comp.Diode.PIN_C, 1);

                andiodess[0][0] = andio1;
                andiodess[0][1] = andio2;

                // connect their anodes to the ordio's anode
                andnet.preWire (andio1, Comp.Diode.PIN_A, andio2, Comp.Diode.PIN_A);
                andnet.preWire (andio2, Comp.Diode.PIN_A, ordio, Comp.Diode.PIN_A);

                // set up all the other diodes that go in 1st column
                Comp.Diode lastandio = andio2;
                Comp.Diode andio3 = null;
                for (int j = 2; j < ninputs - col3ins;) {
                    Network input = inputs.get (j ++);
                    Comp.Diode andio = new Comp.Diode (genctx, col1, "D1_" + j + "." + name, false, true);
                    andiodess[0][j-1] = andio;
                    input.addConn (andio, Comp.Diode.PIN_C, 1);
                    andnet.preWire (andio, Comp.Diode.PIN_A, lastandio, Comp.Diode.PIN_A);
                    lastandio = andio;
                    if (andio3 == null) andio3 = andio;
                }

                // set up all the diodes that go in 3rd column
                lastandio = andio3;
                for (int j = ninputs - col3ins; j < ninputs;) {
                    Network input = inputs.get (j ++);
                    Comp.Diode andio = new Comp.Diode (genctx, col3, "D1_" + j + "." + name, true, true);
                    andiodess[0][j-1] = andio;
                    input.addConn (andio, Comp.Diode.PIN_C, 1);
                    andnet.preWire (andio, Comp.Diode.PIN_A, lastandio, Comp.Diode.PIN_A);
                    lastandio = andio;
                }
                return;
            }
        }

        // general AND-OR-INVERT layout

        //   col1        col2       col3      col4
        //   andiode1a   orres1     collres   led
        //   andiode1b   ordiode1   basedio   trans
        //   andiode1c                        baseres
        //   andiode1d
        //   andiode2a   orres2     ordiode2
        //   andiode2b
        //   andiode3a   orres3     ordiode3

        ordiodes = new Comp.Diode[inputss.size()];
        orresiss = new Comp.Resis[inputss.size()];
        andnets  = new Network[inputss.size()];

        for (int i = 0; i < inputss.size ();) {

            // even up column heights for second and later ordiodes
            if (i > 0) {
                int colht = Math.max (col1.size (), Math.max (col2.size (), col3.size ()));
                while (col1.size () < colht) col1.add (null);
                while (col2.size () < colht) col2.add (null);
                while (col3.size () < colht) col3.add (null);
            }

            // get list of inputs for this AND partition
            ArrayList<Network> inputs = inputss.get (i ++);

            // set up a network for all the anodes and the resistor
            Network andnet = new Network (genctx, "A" + i + "." + name);
            Comp.Resis orres = new Comp.Resis (genctx, col2, "R" + i + "." + name, Comp.Resis.R_OR, true, true);
            vccnet.addConn (orres, Comp.Resis.PIN_C);

            Comp.Diode ordio;
            if (i == 1) {

                // first AND partition gets laid out one way
                ordio = new Comp.Diode (genctx, col2, "D" + i + "." + name, true, true);
                ornet.preWire (basedio, Comp.Diode.PIN_A, ordio, Comp.Diode.PIN_C);
                andnet.preWire (orres, Comp.Resis.PIN_A, ordio, Comp.Diode.PIN_A);
            } else {

                // the rest of the AND partions get laid out another way
                ordio = new Comp.Diode (genctx, col3, "D" + i + "." + name, true, true);
                ornet.addConn (ordio, Comp.Diode.PIN_C);
                andnet.addConn (ordio, Comp.Diode.PIN_A);
            }

            andnets[i-1]  = andnet;
            ordiodes[i-1] = ordio;
            orresiss[i-1] = orres;

            // now create, place and connect all the input diodes
            // place them all in 1st column
            andiodess[i-1] = new Comp.Diode[inputs.size()];
            Comp.Diode lastandio = null;
            for (int j = 0; j < inputs.size ();) {
                Network input = inputs.get (j ++);
                Comp.Diode andio = new Comp.Diode (genctx, col1, "D" + i + "_" + j + "." + name, false, true);
                andiodess[i-1][j-1] = andio;
                input.addConn (andio, Comp.Diode.PIN_C, 1);
                if (lastandio == null) {
                    andnet.preWire (andio, Comp.Diode.PIN_A, orres, Comp.Resis.PIN_A);
                } else {
                    andnet.preWire (andio, Comp.Diode.PIN_A, lastandio, Comp.Diode.PIN_A);
                }
                lastandio = andio;
            }
        }

        if (inputss.size () > 1) {
            genctx.predrawables.add (new PreDrawAOI ());
        }
    }

    // predraw AOI wires that weren't done with preWire() calls
    private class PreDrawAOI implements PreDrawable {
        public void preDraw (PrintStream ps)
        {
            double ordio0cx = basedio.pcbPosXPin (Comp.Diode.PIN_A);
            double ordio0cy = basedio.pcbPosYPin (Comp.Diode.PIN_A);
            for (int i = 1; i < ordiodes.length; i ++) {
                Comp.Diode ordiode1 = ordiodes[i];

                // draw trace for ordiode's cathode
                // - either to basedio's anode with a little crook at each end
                //   or previous ordiode's cathode with straight wire
                double ordio1cx = ordiode1.pcbPosXPin (Comp.Diode.PIN_C);
                double ordio1cy = ordiode1.pcbPosYPin (Comp.Diode.PIN_C);
                if (i > 1) {
                    ornet.putseg (ps, ordio0cx, ordio0cy, ordio1cx, ordio1cy, "B.Cu");
                } else if (flip) {
                    ornet.putseg (ps, ordio0cx,       ordio0cy,       ordio0cx - 0.5, ordio0cy - 0.5, "B.Cu");
                    ornet.putseg (ps, ordio0cx - 0.5, ordio0cy - 0.5, ordio0cx - 0.5, ordio1cy + 0.5, "B.Cu");
                    ornet.putvia (ps, ordio0cx - 0.5, ordio1cy + 0.5, "B.Cu", "F.Cu");
                    ornet.putseg (ps, ordio0cx - 0.5, ordio1cy + 0.5, ordio1cx + 0.5, ordio1cy + 0.5, "F.Cu");
                    ornet.putseg (ps, ordio1cx + 0.5, ordio1cy + 0.5, ordio1cx,       ordio1cy,       "F.Cu");
                } else {
                    ornet.putseg (ps, ordio0cx,       ordio0cy,       ordio0cx + 0.5, ordio0cy + 0.5, "B.Cu");
                    ornet.putseg (ps, ordio0cx + 0.5, ordio0cy + 0.5, ordio0cx + 0.5, ordio1cy - 0.5, "B.Cu");
                    ornet.putvia (ps, ordio0cx + 0.5, ordio1cy - 0.5, "B.Cu", "F.Cu");
                    ornet.putseg (ps, ordio0cx + 0.5, ordio1cy - 0.5, ordio1cx - 0.5, ordio1cy - 0.5, "F.Cu");
                    ornet.putseg (ps, ordio1cx - 0.5, ordio1cy - 0.5, ordio1cx,       ordio1cy,       "F.Cu");
                }
                ordio0cx = ordio1cx;
                ordio0cy = ordio1cy;

                // draw trace for ordiode's anode
                // - to orresis's anode with a little crook at each end
                Network andnet1 = andnets[i];
                double ordio1ax = ordiode1.pcbPosXPin (Comp.Diode.PIN_A);
                double ordio1ay = ordiode1.pcbPosYPin (Comp.Diode.PIN_A);
                double orres1ax = orresiss[i].pcbPosXPin (Comp.Resis.PIN_A);
                double orres1ay = orresiss[i].pcbPosYPin (Comp.Resis.PIN_A);
                if (flip) {
                    andnet1.putseg (ps, ordio1ax, ordio1ay, ordio1ax + 0.5, ordio1ay + 0.5, "F.Cu");
                    andnet1.putseg (ps, ordio1ax + 0.5, ordio1ay + 0.5, orres1ax - 0.5, orres1ay + 0.5, "F.Cu");
                    andnet1.putseg (ps, orres1ax - 0.5, orres1ay + 0.5, orres1ax, orres1ay, "F.Cu");
                } else {
                    andnet1.putseg (ps, ordio1ax, ordio1ay, ordio1ax - 0.5, ordio1ay - 0.5, "F.Cu");
                    andnet1.putseg (ps, ordio1ax - 0.5, ordio1ay - 0.5, orres1ax + 0.5, orres1ay - 0.5, "F.Cu");
                    andnet1.putseg (ps, orres1ax + 0.5, orres1ay - 0.5, orres1ax, orres1ay, "F.Cu");
                }
            }
        }
    }

    private boolean flip;
    private Comp[][] compcols;
    private int cellwidth;
    private int cellheight;
    private int[] colwidths;

    @Override  // Placeable
    public String getName ()
    {
        return name;
    }

    @Override  // Placeable
    public void flipIt ()
    {
        flip = ! flip;

        sizeUpCell ();

        for (Comp[] compcol : compcols) {
            for (Comp comp : compcol) {
                if (comp != null) comp.flipIt ();
            }
        }
        bycap.flipIt ();
    }

    @Override  // Placeable
    public int getWidth ()
    {
        sizeUpCell ();
        return cellwidth;
    }

    @Override  // Placeable
    public int getHeight ()
    {
        sizeUpCell ();
        return cellheight;
    }

    private double saveleftx, savetopy;

    // place cell components on circuit board
    @Override  // Placeable
    public void setPosXY (double leftx, double topy)
    {
        saveleftx = leftx;
        savetopy  = topy;

        sizeUpCell ();

        if (flip) {
            for (int i = compcols.length; -- i >= 0;) {
                Comp[] compcol = compcols[i];
                double y = topy + cellheight;
                for (Comp comp : compcol) {
                    if (comp != null) {
                        y -= comp.getHeight ();
                        comp.setPosXY (leftx, y);
                    } else {
                        y -= Comp.Resis.HEIGHT;
                    }
                }
                leftx += colwidths[i];
            }
        } else {
            for (int i = 0; i < compcols.length; i ++) {
                Comp[] compcol = compcols[i];
                double y = topy;
                for (Comp comp : compcol) {
                    if (comp != null) {
                        comp.setPosXY (leftx, y);
                        y += comp.getHeight ();
                    } else {
                        y += Comp.Resis.HEIGHT;
                    }
                }
                leftx += colwidths[i];
            }
        }

        bycap.setPosXY (trans.getPosX (), trans.getPosY ());
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

    private void sizeUpCell ()
    {
        assert closed;  // col1,2,3,4 all filled in and won't change

        if (compcols == null) {
            compcols = new Comp[][] {
                col1.toArray (new Comp[col1.size()]),
                col2.toArray (new Comp[col2.size()]),
                col3.toArray (new Comp[col3.size()]),
                col4.toArray (new Comp[col4.size()]) };

            // find widths of each column and highest column height
            colwidths = new int[compcols.length];
            for (int i = 0; i < compcols.length; i ++) {
                Comp[] compcol = compcols[i];
                int width  = 0;
                int height = 0;
                for (int j = compcol.length; -- j >= 0;) {
                    int w, h;
                    Comp comp = compcol[j];
                    if (comp != null) {
                        w = comp.getWidth ();
                        h = comp.getHeight ();
                    } else {
                        w = Comp.Resis.WIDTH;
                        h = Comp.Resis.HEIGHT;
                    }
                    if (width < w) width = w;
                    height += h;
                }
                colwidths[i] = width;
                cellwidth += width;
                if (cellheight < height) cellheight = height;
            }
        }
    }

    @Override  // Placeable
    public Network[] getExtNets ()
    {
        int i = 1;
        for (ArrayList<Network> inputs : inputss) i += inputs.size ();
        Network[] extnets = new Network[i];
        i = 0;
        extnets[i++] = outnet;
        for (ArrayList<Network> inputs : inputss) {
            for (Network input : inputs) {
                extnets[i++] = input;
            }
        }
        return extnets;
    }

    // various things about the cell

    public boolean has3Inputs ()
    {
        return (andiodess.length == 1) && (andiodess[0].length == 3);
    }

    public boolean hasSmLed ()
    {
        return smled != null;
    }

    public double colPcbX () { return trans.pcbPosXPin (Comp.Trans.PIN_C); }
    public double colPcbY () { return trans.pcbPosYPin (Comp.Trans.PIN_C); }

    public double ledPcbX () { return smled.pcbPosXPin (Comp.SmLed.PIN_C); }
    public double ledPcbY () { return smled.pcbPosYPin (Comp.SmLed.PIN_C); }

    public double resPcbX () { return rcoll.pcbPosXPin (Comp.Resis.PIN_A); }
    public double resPcbY () { return rcoll.pcbPosYPin (Comp.Resis.PIN_A); }

    public double inPcbX (int i) { return andiodess[0][i].pcbPosXPin (Comp.Diode.PIN_C); }
    public double inPcbY (int i) { return andiodess[0][i].pcbPosYPin (Comp.Diode.PIN_C); }
}
