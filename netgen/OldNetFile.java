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
 * Parse an old .net file to
 *   get component timestamp -> reference
 *   and network name -> code
 */

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.FileReader;
import java.util.HashMap;

public class OldNetFile {
    public HashMap<String,String> comp_tstamp_to_ref;
    public HashMap<String,Integer> net_name_to_code;

    private BufferedReader br;
    private int c;

    public OldNetFile (String netfilename)
            throws Exception
    {
        comp_tstamp_to_ref = new HashMap<> ();
        net_name_to_code = new HashMap<> ();

        br = new BufferedReader (new FileReader (netfilename));

        String blocktype = "";
        String comp_ref = null;
        String comp_tstamp = null;
        int net_code = -1;
        String net_name = null;

        while (true) {
            if (c <= ' ') {
                c = br.read ();
                if (c < 0) break;
                if (c <= ' ') continue;
            }

            // beginning of block
            if (c == '(') {
                c = 0;
                String bt = readValue ();
                blocktype += " " + bt;

                switch (blocktype) {
                    case " export components comp": {
                        comp_ref = null;
                        comp_tstamp = null;
                        break;
                    }
                    case " export net": {
                        net_code = -1;
                        net_name = null;
                        break;
                    }
                }
            }

            // value in block
            else if (c != ')') {
                String val = readValue ();
                switch (blocktype) {
                    case " export components comp ref": {
                        comp_ref = val;
                        break;
                    }
                    case " export components comp tstamp": {
                        comp_tstamp = val;
                        break;
                    }
                    case " export net code": {
                        net_code = Integer.parseInt (val);
                        break;
                    }
                    case " export net name": {
                        net_name = val;
                        break;
                    }
                }
            }

            // end of block
            else {
                if (blocktype.equals ("")) break;
                switch (blocktype) {
                    case " export components comp": {
                        comp_tstamp_to_ref.put (comp_tstamp, comp_ref);
                        break;
                    }
                    case " export net": {
                        net_name_to_code.put (net_name, net_code);
                        break;
                    }
                }
                int i = blocktype.lastIndexOf (' ');
                blocktype = blocktype.substring (0, i);
                c = 0;
            }
        }
    }

    private String readValue ()
            throws Exception
    {
        while (c <= ' ') {
            c = br.read ();
            if (c < 0) throw new EOFException ("eof reading old net file");
        }

        StringBuilder sb = new StringBuilder ();
        boolean quoted = false;
        while (true) {
            if (c < 0) throw new EOFException ("eof reading old net file");
            if (c == '"') {
                quoted = ! quoted;
                c = br.read ();
                continue;
            }
            if (c == '\\') {
                c = br.read ();
                sb.append ((char) c);
                c = br.read ();
                continue;
            }
            if (! quoted) {
                if (c <= ' ') break;
                if (c == '(') break;
                if (c == ')') break;
            }
            sb.append ((char) c);
            c = br.read ();
        }

        return sb.toString ();
    }
}
