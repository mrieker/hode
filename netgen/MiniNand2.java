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

import java.io.PrintStream;

// miniature 2-input NAND gate, everything crammed together
//  d1  ri  ro
//  d2  d0  db  q
//          rb
public class MiniNand2 extends Placeable implements PreDrawable {
    public Comp.Trans q;
    public Comp.ByCap bc;
    public Comp.Diode d1;   // connect input to cathode
    public Comp.Diode d2;   // connect input to cathode
    public Comp.Diode d0;
    public Comp.Diode db;
    public Comp.Resis rb;
    public Comp.Resis ri;
    public Comp.Resis ro;

    public Network outnet;  // get output from here
    public Network andnet;  // connect more inputs here

    private boolean flat;
    private Network basnet;
    private Network ornet;

    //  input:
    //   flat = false: diodes,resistors standing up
    //           true: diodes,resistors down flat
    public MiniNand2 (GenCtx genctx, String name, boolean flat)
    {
        this.flat = flat;

        q  = new Comp.Trans (genctx, null, "q."  + name);
        bc = new Comp.ByCap (genctx, null, "bc." + name);
        d1 = new Comp.Diode (genctx, null, "d1." + name, false, flat);
        d2 = new Comp.Diode (genctx, null, "d2." + name, false, flat);
        d0 = new Comp.Diode (genctx, null, "d0." + name, true,  flat);
        db = new Comp.Diode (genctx, null, "db." + name, true,  flat);
        rb = new Comp.Resis (genctx, null, "rb." + name, Comp.Resis.R_BAS, true,  flat);
        ri = new Comp.Resis (genctx, null, "ri." + name, Comp.Resis.R_OR,  true,  flat);
        ro = new Comp.Resis (genctx, null, "ro." + name, Comp.Resis.R_COL, false, flat);

        Network gnd = genctx.nets.get ("GND");
        Network vcc = genctx.nets.get ("VCC");

        gnd.addConn (q,  Comp.Trans.PIN_E);
        gnd.addConn (bc, Comp.ByCap.PIN_Z);
        gnd.addConn (rb, Comp.Resis.PIN_A);
        vcc.addConn (bc, Comp.ByCap.PIN_Y);
        vcc.addConn (ri, Comp.Resis.PIN_C);
        vcc.addConn (ro, Comp.Resis.PIN_C);

        outnet = new Network (genctx, "q." + name);
        outnet.addConn (q, Comp.Trans.PIN_C);
        outnet.addConn (ro, Comp.Resis.PIN_A);

        basnet = new Network (genctx, "base." + name);
        basnet.addConn (q, Comp.Trans.PIN_B);
        basnet.addConn (db, Comp.Diode.PIN_C);
        basnet.addConn (rb, Comp.Diode.PIN_C);

        ornet = new Network (genctx, "or." + name);
        ornet.addConn (db, Comp.Diode.PIN_A);
        ornet.addConn (d0, Comp.Diode.PIN_C);

        andnet = new Network (genctx, "and." + name);
        andnet.addConn (ri, Comp.Resis.PIN_A);
        andnet.addConn (d0, Comp.Diode.PIN_A);
        andnet.addConn (d1, Comp.Diode.PIN_A);
        andnet.addConn (d2, Comp.Diode.PIN_A);

        genctx.predrawables.add (this);
    }

    @Override  // Placeable
    public String getName ()
    {
        assert q.getName ().startsWith ("q.");
        return q.getName ().substring (2);
    }

    @Override  // Placeable
    public void flipIt ()
    {
        throw new RuntimeException ("MiniNand2.flipIt() not implemented");
    }

    @Override  // Placeable
    public int getHeight ()
    {
        return 3;
    }

    @Override  // Placeable
    public int getWidth ()
    {
        return flat ? 13 : 10;
    }

    @Override  // Placeable
    public void setPosXY (double leftx, double topy)
    {
        int dx = flat ? 3 : 2;
        d1.setPosXY (leftx + 0 * dx, topy + 0);
        ri.setPosXY (leftx + 1 * dx, topy + 0);
        ro.setPosXY (leftx + 2 * dx, topy + 0);
        q .setPosXY (leftx + 3 * dx, topy + 0);
        bc.setPosXY (leftx + 3 * dx, topy + 0);
        d2.setPosXY (leftx + 0 * dx, topy + 1);
        d0.setPosXY (leftx + 1 * dx, topy + 1);
        db.setPosXY (leftx + 2 * dx, topy + 1);
        rb.setPosXY (leftx + 2 * dx, topy + 2);
    }

    @Override  // Placeable
    public double getPosX ()
    {
        return d1.getPosX ();
    }

    @Override  // Placeable
    public double getPosY ()
    {
        return d1.getPosY ();
    }

    @Override  // Placeable
    public Network[] getExtNets ()
    {
        //TODO include inputs
        return new Network[] { outnet };
    }

    @Override  // PreDrawable
    public void preDraw (PrintStream ps)
    {
        double d0ax = d0.pcbPosXPin (Comp.Diode.PIN_A);
        double d0ay = d0.pcbPosYPin (Comp.Diode.PIN_A);
        double d1ax = d1.pcbPosXPin (Comp.Diode.PIN_A);
        double d1ay = d1.pcbPosYPin (Comp.Diode.PIN_A);
        double d2ax = d2.pcbPosXPin (Comp.Diode.PIN_A);
        double d2ay = d2.pcbPosYPin (Comp.Diode.PIN_A);
        double ricx = ri.pcbPosXPin (Comp.Resis.PIN_A);
        double ricy = ri.pcbPosYPin (Comp.Resis.PIN_A);
        andnet.putseg (ps, d1ax, d1ay, d2ax, d2ay, "B.Cu");
        andnet.putseg (ps, d2ax, d2ay, d0ax, d0ay, "F.Cu");
        andnet.putseg (ps, d0ax, d0ay, ricx, ricy, "B.Cu");

        double d0cx = d0.pcbPosXPin (Comp.Diode.PIN_C);
        double d0cy = d0.pcbPosYPin (Comp.Diode.PIN_C);
        double dbax = db.pcbPosXPin (Comp.Diode.PIN_A);
        double dbay = db.pcbPosYPin (Comp.Diode.PIN_A);
        ornet.putseg (ps, d0cx, d0cy, dbax, dbay, "F.Cu");

        double dbcx = db.pcbPosXPin (Comp.Diode.PIN_C);
        double dbcy = db.pcbPosYPin (Comp.Diode.PIN_C);
        double qbx  = q.pcbPosXPin  (Comp.Trans.PIN_B);
        double qby  = q.pcbPosYPin  (Comp.Trans.PIN_B);
        double rbcx = rb.pcbPosXPin (Comp.Resis.PIN_C);
        double rbcy = rb.pcbPosYPin (Comp.Resis.PIN_C);
        basnet.putseg (ps, dbcx, dbcy, qbx,  qby,  "F.Cu");
        basnet.putseg (ps, dbcx, dbcy, rbcx, rbcy, "B.Cu");

        double rocx = ro.pcbPosXPin (Comp.Resis.PIN_A);
        double rocy = ro.pcbPosYPin (Comp.Resis.PIN_A);
        double qcx  = q.pcbPosXPin  (Comp.Trans.PIN_C);
        double qcy  = q.pcbPosYPin  (Comp.Trans.PIN_C);
        outnet.putseg (ps, rocx, rocy, qcx, qcy, "F.Cu");
    }
}
