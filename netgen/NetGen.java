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

// rm -f *.class ; javac NetGen.java

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.TreeMap;
import java.util.TreeSet;
import javax.imageio.ImageIO;

public class NetGen {
    public final static double CORNER10THS = 7.5;

    private static int boardwidth;
    private static int boardheight;
    private static long starttime;

    public static void main (String[] args)
            throws Exception
    {
        boolean dumptokens  = false;
        boolean useoldnet   = false;
        boardwidth  = 195;  // 10th inch
        boardheight = 195;
        int topgap = 0;
        LinkedList<String> modfns = new LinkedList<> ();
        String genname = null;
        String mapname = null;
        String netname = null;
        String pcbname = null;
        String propname = null;
        String repname = null;
        String resname = null;
        String simfile = null;

        for (int i = 0; i < args.length; i ++) {
            String arg = args[i];
            if (arg.equals ("-boardsize")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing x,y after -boardsize");
                }
                int j = args[i].indexOf (",");
                boardwidth  = Integer.parseInt (args[i].substring (0, j));
                boardheight = Integer.parseInt (args[i].substring (++ j));
                continue;
            }
            if (arg.equals ("-dumptokens")) {
                dumptokens = true;
                continue;
            }
            if (arg.equals ("-gen")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing module name after -gen");
                }
                genname = args[i];
                continue;
            }
            if (arg.equals ("-net")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing file name after -net");
                }
                netname = args[i];
                continue;
            }
            if (arg.equals ("-pcb")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing file name after -pcb");
                }
                pcbname = args[i];
                continue;
            }
            if (arg.equals ("-pcbmap")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing file name after -pcbmap");
                }
                mapname = args[i];
                continue;
            }
            if (arg.equals ("-printprop")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing file name after -printprop");
                }
                propname = args[i];
                continue;
            }
            if (arg.equals ("-report")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing file name after -report");
                }
                repname = args[i];
                continue;
            }
            if (arg.equals ("-resist")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing file name after -resist");
                }
                resname = args[i];
                continue;
            }
            if (arg.equals ("-sim")) {
                if ((++ i >= args.length) || ((args[i].length () > 1) && args[i].startsWith ("-"))) {
                    throw new Exception ("missing file name after -sim");
                }
                simfile = args[i];
                continue;
            }
            if (arg.equals ("-topgap")) {
                if ((++ i >= args.length) || args[i].startsWith ("-")) {
                    throw new Exception ("missing gap after -topgap");
                }
                topgap = Integer.parseInt (args[i]);
                continue;
            }
            if (arg.equals ("-useoldnet")) {
                useoldnet = true;
                continue;
            }
            if (arg.startsWith ("-")) {
                throw new Exception ("bad option " + arg);
            }
            modfns.add (arg);
        }
        if (modfns.isEmpty ()) {
            throw new Exception ("no module files specified");
        }
        if (((netname != null) || (pcbname != null) || (repname != null) || (resname != null)) && (genname == null)) {
            throw new Exception ("must specify -gen module when giving -net, -pcb, -report, -resist");
        }

        starttime = System.currentTimeMillis ();

        // parse source files into flat tokens
        Token tokbeg = new Token.Beg ();
        Token tokend = tokbeg;
        for (String modfn : modfns) {
            tokend = Token.parse (tokend, modfn, new BufferedReader (new FileReader (modfn)));
            if (tokend == null) System.exit (1);
        }
        tokend.next = new Token.End ();
        tokend.next.next = tokend.next;

        // print tokens for debugging
        if (dumptokens) {
            String lastfile = "";
            int lastline = 0;
            for (Token tok = tokbeg.next; !(tok instanceof Token.End); tok = tok.next) {
                if (! lastfile.equals (tok.fnm) || (lastline != tok.lno)) {
                    if (lastline > 0) System.out.print ("\n");
                    lastfile = tok.fnm;
                    lastline = tok.lno;
                    System.out.print (lastfile + ":" + lastline + ":");
                }
                System.out.print (" " + tok);
            }
            System.out.print ("\n");
        }

        // reduce flat tokens to textured module trees
        Reduce reduce = new Reduce ();
        HashMap<String,Module> modules = reduce.reduce (tokbeg.next);
        if (modules == null) System.exit (1);

        // generate circuitry
        GenCtx genctx = null;
        Module genmod = null;
        if (genname != null) {
            genmod = modules.get (genname);
            if (genmod == null) throw new Exception ("unknown -gen module " + genname);

            genctx = new GenCtx (boardwidth, boardheight);
            genctx.topgap = topgap;
            new Network (genctx, "GND");
            new Network (genctx, "VCC");

            // get previous component and network assignments
            // ...so we can try to use same ones again
            if (useoldnet && (netname != null)) {
                try {
                    genctx.oldnetfile = new OldNetFile (netname);
                } catch (FileNotFoundException fnfe) {
                    genctx.oldnetfile = null;
                }
            }

            // go through selected module's output pins to see what circuitry it needs to generate those outputs
            for (OpndLhs param : genmod.params) {
                if (!(param instanceof IParam)) {
                    int bw = param.busWidth ();
                    for (int rbit = 0; rbit < bw; rbit ++) {
                        param.generate (genctx, rbit);
                    }
                }
            }

            // any connector output pins also get scanned for circuitry
            for (OpndLhs genner : genmod.generators) {
                int bw = genner.busWidth ();
                for (int rbit = 0; rbit < bw; rbit ++) {
                    genner.generate (genctx, rbit);
                }
            }

            // any other connectors (like power-only) get placed and wired
            for (ConnModule connector : genmod.connectors) {
                connector.getCircuit (genctx);
            }

            // place components on board
            genctx.placeComponents (mapname);

            // assign network numbers
            genctx.assignNetNums ();
        }

        // output netlist file
        if (netname != null) {
            PrintStream ps = new PrintStream (netname);
            printNetFile (ps, genctx);
            ps.close ();
        }

        // output circuit board file
        if (pcbname != null) {
            PrintStream ps = new PrintStream (pcbname);
            printPcbFile (ps, genctx, genmod);
            ps.close ();
        }

        // output report file
        if (repname != null) {
            PrintStream ps = new PrintStream (repname);
            printReport (ps, genctx, genmod);
            ps.close ();
        }

        // output resistor file
        if (resname != null) {
            printResist (resname, genctx, genmod);
        }

        // output propagation file
        if (propname != null) {
            PrintStream ps = new PrintStream (propname);
            printPropFile (ps, genctx);
            ps.close ();
        }

        // do simulation
        if (simfile != null) {
            Simulator sim = new Simulator ();
            sim.modules = modules;
            sim.simulate (simfile);
        }
    }

    private static long valong (int x)
    {
        return (((long) x) << 32) | ((~x) & 0xFFFFFFFFL);
    }

    private static char valchr (long x, int mask)
    {
        if ((int) ((x >>> 32) | x) != mask) return '-';
        return (char) ('0' + (x >>> 32));
    }

    /**
     * Print summary file.
     */
    private static void printReport (PrintStream ps, GenCtx genctx, Module genmod)
    {
        HashMap<String,Comp> colldet = new HashMap<> ();
        TreeMap<String,Integer> compcounts = new TreeMap<> ();
        for (Comp comp : genctx.comps.values ()) {
            String key = comp.getClass ().getName () + " " + comp.value;
            Integer oldcount = compcounts.get (key);
            if (oldcount == null) oldcount = 0;
            compcounts.put (key, oldcount + 1);
            if (comp.getWidth () > 0 || comp.getHeight () > 0) {
                String poskey = comp.posx10th + "," + comp.posy10th;
                if (colldet.containsKey (poskey)) {
                    System.err.println ("position " + poskey + " collision " + comp.name + " and " + colldet.get (poskey).name);
                }
                colldet.put (poskey, comp);
            }
        }
        ps.println ("Component counts:");
        for (String key : compcounts.keySet ()) {
            ps.println (String.format ("%8d  %s", compcounts.get (key), key));
        }

        // compute total line lengths coming out of all placeables
        for (Placeable p : genctx.placeables.values ()) {
            Network[] extnets = p.getExtNets ();
            for (Network n : extnets) {
                if (n.placeables == null) {
                    n.placeables = new HashSet<> ();
                }
                n.placeables.add (p);
            }
        }
        Placeable[] placeablesbylinelen = genctx.placeables.values ().toArray (new Placeable[0]);
        for (Placeable p : placeablesbylinelen) {
            p.linelen = 0.0;
            double px = p.getPosX () + p.getWidth ()  / 2.0;
            double py = p.getPosY () + p.getHeight () / 2.0;
            Network[] extnets = p.getExtNets ();
            for (Network n : extnets) {
                double nearestdist = 99999.0;
                for (Placeable nearbyplaceable : n.placeables) {
                    double nbpx = nearbyplaceable.getPosX () + nearbyplaceable.getWidth  () / 2.0;
                    double nbpy = nearbyplaceable.getPosY () + nearbyplaceable.getHeight () / 2.0;
                    double dist = Math.abs (nbpx - px) + Math.abs (nbpy - py);
                    if ((dist > 0.0) && (nearestdist > dist)) nearestdist = dist;
                }
                if (nearestdist < 99999.0) p.linelen += nearestdist;
            }
        }
        Arrays.sort (placeablesbylinelen, new Comparator<Placeable> () {
            @Override
            public int compare (Placeable a, Placeable b)
            {
                return Double.compare (b.linelen, a.linelen);
            }
        });

        ps.println ("");
        ps.println ("Placeables:");
        double totarea = 0.0;
        double totalen = 0.0;
        for (Placeable p : placeablesbylinelen) {
            double w = p.getWidth  () / 10.0;
            double h = p.getHeight () / 10.0;
            ps.println (String.format ("  %4.1f x %4.1f  at  %5.2f,%5.2f  %6.1f  %s",
                w, h,
                p.getPosX () / 10.0, p.getPosY () / 10.0,
                p.linelen,
                p.getName ()));
            totarea += w * h;
            totalen += p.linelen;
        }
        ps.println (String.format ("  %6.2f :area  total  length: %7.1f", totarea, totalen));

        // output a report showing all the components in a hierarchy of modules and source lines
        ps.println ("");
        CompTree comptree = new CompTree ("", genctx.comps);
        comptree.print (ps, "", genmod.name);

        // print wire-anded networks
        ps.println ("");
        ps.println ("Wired Ands:");
        for (WiredAnd wiredand : genctx.wiredands.values ()) {
            ps.println ("  " + wiredand.name);
            for (Network subnet : wiredand.subnets) {
                ps.println ("    " + subnet.name);
            }
        }

        // print network with load counts
        String[] netnames = genctx.nets.keySet ().toArray (new String[0]);
        Arrays.sort (netnames);
        for (String name : netnames) {
            Network net = genctx.nets.get (name);
            int loads = net.getLoads ();
            if (loads > 0) {
                ps.println ("");
                ps.println ("Network: " + name + " (load " + loads + ((loads > 10) ? " EXCEEDS LIMIT OF 10" : "") + ")");
                for (Network.Conn conn : net.connIterable ()) {
                    ps.println ("  " + conn.comp.name + " <" + conn.pino + ">");
                }
            }
        }

        // print component references and values
        ps.println ("");
        ps.println ("Component values:");
        TreeMap<String,ArrayList<Comp>> compsbyref = new TreeMap<> ();
        for (Comp comp : genctx.comps.values ()) {
            String ref = comp.ref;
            int j;
            for (j = ref.length (); j > 0; -- j) {
                char c = ref.charAt (j - 1);
                if ((c < '0') || (c > '9')) break;
            }
            String refpfx = ref.substring (0, j);
            ArrayList<Comp> comps = compsbyref.get (refpfx);
            if (comps == null) {
                comps = new ArrayList<> (genctx.comps.size ());
                compsbyref.put (refpfx, comps);
            }
            comps.add (comp);
        }

        for (String refpfx : compsbyref.keySet ()) {
            Comp[] comps = compsbyref.get (refpfx).toArray (new Comp[0]);

            // sort components such that R100 comes after R99
            int refpfxlen = refpfx.length ();
            Arrays.sort (comps, new Comparator<Comp> () {
                @Override
                public int compare (Comp compa, Comp compb)
                {
                    int refa = Integer.parseInt (compa.ref.substring (refpfxlen));
                    int refb = Integer.parseInt (compb.ref.substring (refpfxlen));
                    return refa - refb;
                }
            });

            // get longest ref and val lengths
            int reflen = 0;
            int vallen = 0;
            for (Comp comp : comps) {
                if (reflen < comp.ref.length   ()) reflen = comp.ref.length   ();
                if (vallen < comp.value.length ()) vallen = comp.value.length ();
            }

            // print components for this prefix
            ps.println ("");
            int colwidth = reflen + vallen + 6;
            int nperline = 132 / colwidth;
            if (nperline < 1) nperline = 1;
            int numlines = (comps.length + nperline - 1) / nperline;
            for (int i = 0; i < numlines; i ++) {
                for (int j = 0; j < nperline; j ++) {
                    int k = j * numlines + i;
                    if (k < comps.length) {
                        Comp comp = comps[k];
                        String ref = comp.ref;
                        String val = comp.value;
                        while (ref.length () < reflen) ref += ' ';
                        while (val.length () < vallen) val += ' ';
                        ps.print ("  " + ref + "  " + val + "  ");
                    }
                }
                ps.println ("");
            }
        }
    }

    /**
     * Print resistor placement file.
     */
    private static void printResist (String resname, GenCtx genctx, Module genmod)
            throws Exception
    {
        int wf = 40;
        int hf = 25;
        BufferedImage bi = new BufferedImage (boardwidth * wf, boardheight * hf, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = bi.createGraphics ();
        g2d.setColor (Color.WHITE);
        g2d.fillRect (0, 0, boardwidth * wf, boardheight * hf);
        g2d.setFont (Font.decode ("Arial-PLAIN-18"));
        g2d.setColor (Color.BLACK);
        for (Comp comp : genctx.comps.values ()) {
            comp.printres (g2d, wf, hf);
        }
        if (! ImageIO.write (bi, "png", new File (resname))) {
            throw new Exception ("error writing " + resname);
        }
    }

    /**
     * Print out NetList file.
     */
    private static void printNetFile (PrintStream ps, GenCtx genctx)
    {
        ps.println (
            "(export (version D)\n" +
            "  (design\n" +
            "    (source /home/mrieker/nfs/tubecpu/kicads/ProtoBoard/ProtoBoard.sch)\n" +
            "    (date \"Fri 26 Feb 2021 03:41:24 PM EST\")\n" +
            "    (tool \"Eeschema 4.0.6\")\n" +
            "    (sheet (number 1) (name /) (tstamps /)\n" +
            "      (title_block\n" +
            "        (title)\n" +
            "        (company)\n" +
            "        (rev)\n" +
            "        (date)\n" +
            "        (source ProtoBoard.sch)\n" +
            "        (comment (number 1) (value \"\"))\n" +
            "        (comment (number 2) (value \"\"))\n" +
            "        (comment (number 3) (value \"\"))\n" +
            "        (comment (number 4) (value \"\")))))");

        // print out list of components
        ps.println ("  (components");
        String[] compnames = genctx.comps.keySet ().toArray (new String[0]);
        Arrays.sort (compnames);
        for (String name : compnames) {
            Comp comp = genctx.comps.get (name);
            comp.printnet (ps);
        }
        ps.println ("  )");

        ps.println ("  (libparts");
        for (String libpart : genctx.libparts.values ()) {
            ps.print (libpart);
        }
        ps.println (
            "  )\n" +
            "  (libraries\n" +
            "   (library (logical conn)\n" +
            "     (uri /usr/share/kicad/library/conn.lib))\n" +
            "   (library (logical device)\n" +
            "     (uri /usr/share/kicad/library/device.lib)))");

        // print out list of networks (connections among components)
        TreeSet<String> netnames = genctx.getMergedNetNames ();
        for (String name : netnames) {
            Network net = genctx.nets.get (name);
            ps.println ("    (net (code " + net.code + ") (name \"" + name + "\")");
            for (Network.Conn conn : net.connIterable ()) {
                ps.println ("      (node (ref " + conn.comp.ref + ") (pin " + conn.pino + "))");
            }
            ps.println ("    )");
        }
        ps.println ("  ))");
    }

    /**
     * Print out PCB file.
     */
    private static void printPcbFile (PrintStream ps, GenCtx genctx, Module genmod)
    {
        TreeSet<String> netnames = genctx.getMergedNetNames ();
        String[] compnames = genctx.comps.keySet ().toArray (new String[0]);
        Arrays.sort (compnames);
        int ncomps = compnames.length;

        String sizemm = String.format ("%4.2f %4.2f", boardwidth * 2.54, boardheight * 2.54);

        ps.print (
            "(kicad_pcb (version 4) (host pcbnew 4.0.6)\n" +
            "  (general\n" +
        //TODO  "    (links 548)\n" +
        //TODO  "    (no_connects 103)\n" +
            "    (area 0 0 0 0)\n" +
            "    (thickness 1.6)\n" +
            "    (drawings 0)\n" +
            "    (tracks 0)\n" +
            "    (zones 0)\n" +
            "    (modules " + ncomps + ")\n" +
            "    (nets " + netnames.size () + ")\n" +
            "  )\n" +
            "  (page User " + sizemm + ")\n" +
            "  (layers\n" +
            "    (0 F.Cu signal)\n" +
            "    (1 In1.Cu power hide)\n" +
            "    (2 In2.Cu power hide)\n" +
            "    (31 B.Cu signal)\n" +
            "    (32 B.Adhes user)\n" +
            "    (33 F.Adhes user)\n" +
            "    (34 B.Paste user)\n" +
            "    (35 F.Paste user)\n" +
            "    (36 B.SilkS user)\n" +
            "    (37 F.SilkS user)\n" +
            "    (38 B.Mask user)\n" +
            "    (39 F.Mask user)\n" +
            "    (40 Dwgs.User user)\n" +
            "    (41 Cmts.User user)\n" +
            "    (42 Eco1.User user)\n" +
            "    (43 Eco2.User user)\n" +
            "    (44 Edge.Cuts user)\n" +
            "    (45 Margin user)\n" +
            "    (46 B.CrtYd user)\n" +
            "    (47 F.CrtYd user)\n" +
            "    (48 B.Fab user)\n" +
            "    (49 F.Fab user)\n" +
            "  )\n" +
            "  (setup\n" +
            "    (last_trace_width 0.25)\n" +
            "    (trace_clearance 0.2)\n" +
            "    (zone_clearance 0.508)\n" +
            "    (zone_45_only no)\n" +
            "    (trace_min 0.2)\n" +
            "    (segment_width 0.2)\n" +
            "    (edge_width 0.1)\n" +
            "    (via_size 0.6)\n" +
            "    (via_drill 0.4)\n" +
            "    (via_min_size 0.4)\n" +
            "    (via_min_drill 0.3)\n" +
            "    (uvia_size 0.3)\n" +
            "    (uvia_drill 0.1)\n" +
            "    (uvias_allowed no)\n" +
            "    (uvia_min_size 0.2)\n" +
            "    (uvia_min_drill 0.1)\n" +
            "    (pcb_text_width 0.3)\n" +
            "    (pcb_text_size 1.5 1.5)\n" +
            "    (mod_edge_width 0.15)\n" +
            "    (mod_text_size 1 1)\n" +
            "    (mod_text_width 0.15)\n" +
            "    (pad_size 1.5 1.5)\n" +
            "    (pad_drill 0.6)\n" +
            "    (pad_to_mask_clearance 0)\n" +
            "    (aux_axis_origin 0 0)\n" +
            "    (visible_elements FFFFF77F)\n" +
            "    (pcbplotparams\n" +
            "      (layerselection 0x00030_80000007)\n" +
            "      (usegerberextensions false)\n" +
            "      (excludeedgelayer true)\n" +
            "      (linewidth 0.100000)\n" +
            "      (plotframeref false)\n" +
            "      (viasonmask false)\n" +
            "      (mode 1)\n" +
            "      (useauxorigin false)\n" +
            "      (hpglpennumber 1)\n" +
            "      (hpglpenspeed 20)\n" +
            "      (hpglpendiameter 15)\n" +
            "      (hpglpenoverlay 2)\n" +
            "      (psnegative false)\n" +
            "      (psa4output false)\n" +
            "      (plotreference true)\n" +
            "      (plotvalue true)\n" +
            "      (plotinvisibletext false)\n" +
            "      (padsonsilk false)\n" +
            "      (subtractmaskfromsilk false)\n" +
            "      (outputformat 1)\n" +
            "      (mirror false)\n" +
            "      (drillshape 0)\n" +
            "      (scaleselection 1)\n" +
            "      (outputdirectory gerber/))\n" +
            "  )\n" +
            "  (net 0 \"\")\n");

        // output network names and assigned number
        for (String name : netnames) {
            Network net = genctx.nets.get (name);
            ps.println ("  (net " + net.code + " \"" + name + "\")");
        }

        // define default net class and put all the nets in that
        for (NetClass netclass : genctx.netclasses.values ()) {
            ps.println ("  (net_class " + netclass.name + " " + netclass.attr);
            for (Network net : netclass.networks) {
                if (netnames.contains (net.name)) {
                    ps.println ("    (add_net \"" + net.name + "\")");
                }
            }
            ps.println ("  )");
        }

        // four corners
        int corners = 0;
        for (double cy = CORNER10THS; cy < boardheight; cy += boardheight - 2 * CORNER10THS) {
            for (double cx = CORNER10THS; cx < boardwidth; cx += boardwidth - 2 * CORNER10THS) {
                int tedit  = ++ corners;
                int tstamp = ++ corners;
                int path   = ++ corners;
                ps.println (String.format ("  (module Connectors:1pin (layer F.Cu) (tedit %08X) (tstamp %08X)", tedit, tstamp));
                ps.println (String.format ("    (at %4.2f %4.2f)", cx * 2.54, cy * 2.54));
                ps.println ("    (descr \"module 1 pin (ou trou mecanique de percage)\")");
                ps.println ("    (tags DEV)");
                ps.println ("    (fp_text reference H" + (++ corners) + " (at 0 0 0) (layer F.SilkS)");
                ps.println ("      (effects (font (size 0 0) (thickness 0)))");
                ps.println ("    )");
                ps.println (String.format ("    (path /%08X)", path));
                ps.println ("    (fp_circle (center 0 0) (end 2 0.8) (layer F.Fab) (width 0.1))");
                ps.println ("    (fp_circle (center 0 0) (end 2.6 0) (layer F.CrtYd) (width 0.05))");
                ps.println ("    (fp_circle (center 0 0) (end 0 -2.286) (layer F.SilkS) (width 0.12))");
                ps.println ("    (pad 1 thru_hole circle (at 0 0) (size 4.064 4.064) (drill 3.048) (layers *.Cu *.Mask)))");
            }
        }

        // puque out a lot for each component "(module ...)"
        // - mostly boilerplate over and over and over
        for (String name : compnames) {
            Comp comp = genctx.comps.get (name);
            try {
                comp.printpcb (ps, genctx);
            } catch (Exception e) {
                System.err.println ("exception placing component <" + name + "> on circuit board");
                throw e;
            }
        }

        // print prewired traces
        for (Network net : genctx.nets.values ()) {
            net.printPreWires (ps);
        }

        for (PreDrawable pd : genctx.predrawables) {
            pd.preDraw (ps);
        }

        // print edge line around whole perimeter
        String xedgemm = String.format ("%4.2f", boardwidth  * 2.54);
        String yedgemm = String.format ("%4.2f", boardheight * 2.54);
        ps.println ("   (gr_line (start 0 " + yedgemm + ") (end 0 0) (angle 90) (layer Edge.Cuts) (width 0.1))");
        ps.println ("   (gr_line (start " + xedgemm + " " + yedgemm + ") (end 0 " + yedgemm + ") (angle 90) (layer Edge.Cuts) (width 0.1))");
        ps.println ("   (gr_line (start " + xedgemm + " 0) (end " + xedgemm + " " + yedgemm + ") (angle 90) (layer Edge.Cuts) (width 0.1))");
        ps.println ("   (gr_line (start 0 0) (end " + xedgemm + " 0) (angle 90) (layer Edge.Cuts) (width 0.1))");

        // draw zone outlines for ground and power planes
        Network gndnet = genctx.nets.get ("GND");
        ps.println (String.format ("  (zone (net %d) (net_name \"%s\") (layer In2.Cu) (tstamp 606A325F) (hatch edge 0.508)", gndnet.code, gndnet.name));
        ps.println ("    (connect_pads (clearance 0.508))");
        ps.println ("    (min_thickness 0.254)");
        ps.println ("    (fill (arc_segments 16) (thermal_gap 0.508) (thermal_bridge_width 0.508))");
        ps.println ("    (polygon");
        ps.println ("      (pts");
        ps.println (String.format ("        (xy 2.54 2.54) (xy 2.54 %4.2f) (xy %4.2f %4.2f) (xy %4.2f 2.54)",
                (boardheight - 1) * 2.54, (boardwidth - 1) * 2.54, (boardheight - 1) * 2.54, (boardwidth - 1) * 2.54));
        ps.println ("      )");
        ps.println ("    )");
        ps.println ("  )");

        Network vccnet = genctx.nets.get ("VCC");
        ps.println (String.format ("  (zone (net %d) (net_name \"%s\") (layer In1.Cu) (tstamp 606A3260) (hatch edge 0.508)", vccnet.code, vccnet.name));
        ps.println ("    (connect_pads (clearance 0.508))");
        ps.println ("    (min_thickness 0.254)");
        ps.println ("    (fill (arc_segments 16) (thermal_gap 0.508) (thermal_bridge_width 0.508))");
        ps.println ("    (polygon");
        ps.println ("      (pts");
        ps.println (String.format ("        (xy 2.54 2.54) (xy 2.54 %4.2f) (xy %4.2f %4.2f) (xy %4.2f 2.54)",
                (boardheight - 1) * 2.54, (boardwidth - 1) * 2.54, (boardheight - 1) * 2.54, (boardwidth - 1) * 2.54));
        ps.println ("      )");
        ps.println ("    )");
        ps.println ("  )");

        // done
        ps.println (")");
    }

    // print propagation delays
    private static void printPropFile (PrintStream ps, GenCtx genctx)
    {
        // generate in-memory propagation tree
        System.out.println ((System.currentTimeMillis () - starttime) + " generating propagation tree");
        PropNode topnode = new PropNode ();
        topnode.type = "Root";

        PropPrinter pp = new PropPrinter ();
        pp.currnode = topnode;

        for (PropRoot root : genctx.proproots) {
            pp.println (root.propopnd.getClass ().getName (), root.propopnd, root.proprbit);
            pp.inclevel ();
            root.propopnd.printProp (pp, root.proprbit);
            pp.declevel ();
            assert pp.currnode == topnode;
        }

        ////// print out all the propagation branches
        ////printPropChilds (ps, topnode, "");

        ps.println ("roots named D/halt1:");
        if (topnode.childs != null) {
            for (PropNode node : topnode.childs) {
                if (node.opnd.name.startsWith ("D/halt1")) {
                    ps.println (node.opnd.name);
                    printPropChilds (ps, node, "  ");
                }
            }
        }

        /*
        // get list of all the leaves
        // they should all be DFF Q/_Q outputs and RasPi outputs
        System.out.println ((System.currentTimeMillis () - starttime) + " getting all leaves");
        LinkedList<PropNode> leaves = new LinkedList<> ();
        findPropLeaves (leaves, topnode);

        // print all leaves
        System.out.println ((System.currentTimeMillis () - starttime) + " printing leaves");
        ps.println ("all leaves:");
        for (PropNode leaf : leaves) {
            ps.println (leaf.toString ());
        }

        // find the deepest level leaf
        System.out.println ((System.currentTimeMillis () - starttime) + " printing deepest branches");
        int deepest = 0;
        for (PropNode leaf : leaves) {
            if (deepest < leaf.level) {
                deepest = leaf.level;
            }
        }
        for (int depth = deepest; depth > 0; -- depth) {
            int n = 0;
            for (PropNode leaf : leaves) {
                if (leaf.level == depth) {
                    n ++;
                }
            }
            ps.println ("depth " + depth + " has " + n + " branches");
        }
        ps.println ("deepest branch:");
        for (PropNode leaf : leaves) {
            if (leaf.level == deepest) {
                printLeafBranch (ps, leaf);
            }
        }
        */
    }

    private static void printPropChilds (PrintStream ps, PropNode parent, String indent)
    {
        if (parent.childs != null) {
            String indent2 = indent + "  ";
            for (PropNode node : parent.childs) {
                ps.println (String.format ("%05d ", node.level) + indent + node.type + " " + node.opnd.name + "[" + node.rbit + "]");
                printPropChilds (ps, node, indent2);
            }
        }
    }

    private static void findPropLeaves (LinkedList<PropNode> leaves, PropNode node)
    {
        if (node.childs == null) {
            leaves.add (node);
        } else {
            for (PropNode child : node.childs) {
                findPropLeaves (leaves, child);
            }
        }
    }

    private static void printLeafBranch (PrintStream ps, PropNode leaf)
    {
        if (leaf.parent != null) printLeafBranch (ps, leaf.parent);
        ps.println (leaf.toString ());
    }
}
