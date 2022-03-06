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

#include <assert.h>
#include <complex.h>
#include <hode.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define FMT "%g"
typedef ComplexF CVal;
typedef float SVal;
typedef __uint16_t Number;

struct Matrix;
struct Node;
struct Row;

struct Branch {
    Branch *nextbranch;
    char const *branchname;
    Number branchnum;
    Node *plusnode;
    Node *minusnode;

    virtual void addtomatrix (Row *row) = 0;
};

struct Capacitor : Branch {
    SVal farads;

    virtual void addtomatrix (Row *row);
};

struct Current : Branch {
    CVal amps;

    virtual void addtomatrix (Row *row);
};

struct CurDepOnCurrent : Branch {
    CVal ratio;
    Branch depend;

    virtual void addtomatrix (Row *row);
};

struct CurDepOnVoltage : Branch {
    CVal ratio;
    Node posdep;
    Node negdep;

    virtual void addtomatrix (Row *row);
};

struct Inductor : Branch {
    SVal henries;

    virtual void addtomatrix (Row *row);
};

struct Resistor : Branch {
    SVal ohms;

    virtual void addtomatrix (Row *row);
};

struct Voltage : Branch {
    CVal volts;

    virtual void addtomatrix (Row *row);
};

struct Node {
    Node *nextnode;
    char const *nodename;
    Number nodenum;
};

struct Row {
    Number numnodes;
    Number numbranches;
    CVal *columns;

    virtual ~Row ();
    Row *init (Number numnodes, Number numbranches);

    void addtonodecolumn (Number nodenum, CVal value);
    void addtonodecolumnr (Number nodenum, SVal val);
    void addtobranchcolumn (Number branchnum, CVal value);
    void addtobranchcolumnr (Number branchnum, SVal val);
    void addtobranchcolumni (Number branchnum, SVal val);
    void addtoconstantcolumn (CVal value);

    CVal getconstantcolumn ();
};

struct Matrix {
    Number numnodes;
    Number numbranches;
    Row **rowslist;

    Node *nodelist;
    Branch *branchlist;

    Matrix ();
    virtual ~Matrix ();
    void addbranch (Branch *branch);
    void finalize (Node *groundnode);
    Row *getnoderow (Number nodenum);
    Row *getbranchrow (Number branchnum);
    void solve ();
    void print ();
    void reset ();
};

////////////
//  main  //
////////////

static SVal womega;

int main (int argc, char **argv)
{
    Node groundnode, sourcenode, vinputnode, middlenode, voutputnode;
    groundnode.nodename  = "ground";
    sourcenode.nodename  = "source";
    vinputnode.nodename  = "vinput";
    middlenode.nodename  = "middle";
    voutputnode.nodename = "voutput";

    Inductor L1;
    L1.branchname = "L1";
    L1.plusnode = &sourcenode;
    L1.minusnode = &groundnode;
    L1.henries = (SVal) 4.7e-6;

    Current I2;
    I2.branchname = "I2";
    I2.plusnode = &sourcenode;
    I2.minusnode = &groundnode;
    I2.amps.real = (SVal) 0.1;
    I2.amps.imag = 0.0;

    Capacitor C1;
    C1.branchname = "C1";
    C1.plusnode = &sourcenode;
    C1.minusnode = &vinputnode;
    C1.farads = (SVal) 0.01e-6;

    Capacitor C2;
    C2.branchname = "C2";
    C2.plusnode = &vinputnode;
    C2.minusnode = &groundnode;
    C2.farads = (SVal) 470e-12;

    Inductor L2;
    L2.branchname = "L2";
    L2.plusnode = &vinputnode;
    L2.minusnode = &middlenode;
    L2.henries = (SVal) 2e-6;

    Capacitor C3;
    C3.branchname = "C3";
    C3.plusnode = &middlenode;
    C3.minusnode = &groundnode;
    C3.farads = (SVal) 940e-12;

    Inductor L3;
    L3.branchname = "L3";
    L3.plusnode = &middlenode;
    L3.minusnode = &voutputnode;
    L3.henries = (SVal) 2e-6;

    Capacitor C4;
    C4.branchname = "C4";
    C4.plusnode = &voutputnode;
    C4.minusnode = &groundnode;
    C4.farads = (SVal) 470e-12;

    Resistor R1;
    R1.branchname = "R1";
    R1.plusnode = &voutputnode;
    R1.minusnode = &groundnode;
    R1.ohms = 50;

    Matrix matrix;

    //SVal freq = 7000000; {
    for (SVal freq = 6000000; freq <= 9000000; freq += 250000) {
        womega = freq * (SVal) (2.0 * M_PI);

        printf ("\nresults freq "FMT":\n", freq);

        matrix.addbranch (&L1);
        matrix.addbranch (&I2);
        matrix.addbranch (&C1);
        matrix.addbranch (&C2);
        matrix.addbranch (&L2);
        matrix.addbranch (&C3);
        matrix.addbranch (&L3);
        matrix.addbranch (&C4);
        matrix.addbranch (&R1);

        try {
            matrix.finalize (&groundnode);
            //printf ("\nbefore:\n");
            //matrix.print ();
            matrix.solve ();
            //printf ("\nafter:\n");
            //matrix.print ();
        } catch (char const *msg) {
            fprintf (stderr, "error: %s\n", msg);
            matrix.print ();
            return 1;
        }

        // 0x2220 = 0xE2 0x88 0xA0 = angle symbol https://www.compart.com/en/unicode/U+2220

        printf ("\n  voltages:\n\n");
        for (Node *node = matrix.nodelist; node != NULL; node = node->nextnode) {
            if (node->nodenum > 0) {
                CVal v = matrix.rowslist[node->nodenum-1]->getconstantcolumn ();
                SVal a = v.ang () * (SVal) (180.0 / M_PI);
                printf ("    v_%03u  %s = "FMT" + j "FMT" = "FMT" \342\210\240 "FMT"\302\260\n", node->nodenum, node->nodename, v.real, v.imag, v.abs (), a);
            }
        }
        printf ("\n  currents:\n\n");
        for (Branch *branch = matrix.branchlist; branch != NULL; branch = branch->nextbranch) {
            CVal i = matrix.rowslist[matrix.numnodes+branch->branchnum-2]->getconstantcolumn ();
            SVal a = i.ang () * (SVal) (180.0 / M_PI);
            printf ("    i_%03u  %s = "FMT" + j "FMT" = "FMT" \342\210\240 "FMT"\302\260\n", branch->branchnum, branch->branchname, i.real, i.imag, i.abs (), a);
        }
        printf ("\n");

        matrix.reset ();
    }

    return 0;
}

/////////////////////////////
//  Branches (components)  //
/////////////////////////////

Branch::Branch ();
Capacitor::Capacitor ();
Current::Current ();
Inductor::Inductor ();
Resistor::Resistor ();
Voltage::Voltage ();

// add capacitor to matrix
//  plusnode  = resistor takes current from plus node
//  minusnode = resistor gives current to minus node
//  farads    = how many farads
void Capacitor::addtomatrix (Row *row)
{
    // r = 1 / (j * w * c) = - j / (w * c)
    // v = i * r

    // v = vp - vm
    // i = ib

    //  1 * vp - 1 * vm - R * ib = 0
    // -1 * vp + 1 * vm + R * ib = 0
    row->addtonodecolumnr (this->plusnode->nodenum, -1);
    row->addtonodecolumnr (this->minusnode->nodenum, 1);
    row->addtobranchcolumni (this->branchnum, -1.0 / (womega * this->farads));
}

// add current source to matrix
//  plusnode  = positive end of current source
//  minusnode = negative end of current source
//  amps      = how many amps from minusnode to plusnode
void Current::addtomatrix (Row *row)
{
    // current in the branch is this many amps
    //  1 * ib = amps
    row->addtobranchcolumnr (this->branchnum, 1);
    row->addtoconstantcolumn (this->amps);
}

void CurDepOnCurrent::addtomatrix (Row *row)
{
    // ibranch = ratio * idepend
    //  -1 * ibanch + ratio * idepend = 0
    row->addtobranchcolumnr (this->branchnum, -1);
    row->addtobranchcolumn (this->depend->branchnum, this->ratio);
}

void CurDepOnVoltage::addtomatrix (Row *row)
{
    // ibranch = ratio * (iposvolts - inegvolts)
    //  -1 * ibanch + ratio * iposvolts - ratio * inegvolts = 0
    row->addtobranchcolumnr (this->branchnum, -1);
    row->addtonodecolumn (this->posdep->nodenum, this->ratio);
    row->addtonodecolumn (this->negdep->nodenum, this->ratio->neg ());
}

// add inductor to matrix
//  plusnode  = resistor takes current from plus node
//  minusnode = resistor gives current to minus node
//  henries   = how many henries
void Inductor::addtomatrix (Row *row)
{
    // r = j * w * l
    // v = i * r

    // v = vp - vm
    // i = ib

    //  1 * vp - 1 * vm - R * ib = 0
    // -1 * vp + 1 * vm + R * ib = 0
    row->addtonodecolumnr (this->plusnode->nodenum, -1);
    row->addtonodecolumnr (this->minusnode->nodenum, 1);
    row->addtobranchcolumni (this->branchnum, womega * this->henries);
}

// add resistor to matrix
//  plusnode  = resistor takes current from plus node
//  minusnode = resistor gives current to minus node
//  ohms      = how many ohms
void Resistor::addtomatrix (Row *row)
{
    // v = i * r

    // v = vp - vm
    // i = ib

    //  1 * vp - 1 * vm - R * ib = 0
    // -1 * vp + 1 * vm + R * ib = 0
    row->addtonodecolumnr (this->plusnode->nodenum, -1);
    row->addtonodecolumnr (this->minusnode->nodenum, 1);
    row->addtobranchcolumnr (this->branchnum, this->ohms);
}

// add voltage source to matrix
//  plusnode  = positive end of voltage source
//  minusnode = negative end of voltage source
//  volts     = how many volts from minusnode to plusnode
void Voltage::addtomatrix (Row *row)
{
    // vp - vm = V
    row->addtonodecolumnr (this->plusnode->nodenum, 1);
    row->addtonodecolumnr (this->minusnode->nodenum, -1);
    row->addtoconstantcolumn (this->volts);
}

//////////////
//  Matrix  //
//////////////

// matrix:
//  vnode1  vnode2  ...  ibranch1  ibranch2  ibranch3  ...  =  constant
//  ------  ------  ---  --------  --------  --------  ---     --------
//  row for currents going into node0
//  row for currents going into node1
//  row for currents going into node2
//    .
//    .
//    .
//  row for branch1
//  row for branch2
//  row for branch3
//    .
//    .
//    .

Matrix::Matrix ()
{
    this->numnodes = 0;
    this->numbranches = 0;
    this->rowslist = NULL;
    this->nodelist = NULL;
    this->branchlist = NULL;
}

Matrix::~Matrix ()
{
    this->reset ();
}

Row *Matrix::getnoderow (Number nodenum)
{
    assert (nodenum < this->numnodes);
    return this->rowslist[nodenum];
}

Row *Matrix::getbranchrow (Number branchnum)
{
    assert ((branchnum > 0) && (branchnum <= this->numbranches));
    return this->rowslist[this->numnodes+branchnum-1];
}

void Matrix::addbranch (Branch *branch)
{
    branch->branchnum = ++ this->numbranches;
    branch->nextbranch = this->branchlist;
    this->branchlist = branch;
    branch->plusnode->nextnode = branch->plusnode;
    branch->minusnode->nextnode = branch->minusnode;
}

// all branches have been added to the matrix, finalize matrix
void Matrix::finalize (Node *groundnode)
{
    // build list of all nodes except groundnode
    for (Branch *branch = this->branchlist; branch != NULL; branch = branch->nextbranch) {
        for (Node *node = branch->plusnode;; node = branch->minusnode) {
            if ((node != groundnode) && (node->nextnode == node)) {
                node->nextnode = this->nodelist;
                this->nodelist = node;
            }
            if (node == branch->minusnode) break;
        }
    }

    // make sure groundnode is first in nodelist so it gets to be node #0
    groundnode->nextnode = this->nodelist;
    this->nodelist = groundnode;

    // number all the nodes on the list starting at zero
    this->numnodes = 0;
    for (Node *node = this->nodelist; node != NULL; node = node->nextnode) {
        node->nodenum = this->numnodes ++;
    }

    // allocate matrix memory
    Number numrows = this->numnodes + this->numbranches;
    this->rowslist = malloc (numrows * sizeof *this->rowslist);
    for (Number i = numrows; i > 0;) {
        this->rowslist[--i] = new Row->init (this->numnodes, this->numbranches);
    }

    // fill in matrix with component values
    for (Branch *branch = this->branchlist; branch != NULL; branch = branch->nextbranch) {

        // compute current through branch
        Row *row = this->getbranchrow (branch->branchnum);
        branch->addtomatrix (row);

        // all currents entering a node = all currents leaving a node
        this->getnoderow (branch->plusnode->nodenum)->addtobranchcolumnr (branch->branchnum, 1);
        this->getnoderow (branch->minusnode->nodenum)->addtobranchcolumnr (branch->branchnum, -1);
    }
}

// solve matrix to the form:
//   1  0  0  k1
//   0  1  0  k2
//   0  0  1  k3
//   0  0  0   0
void Matrix::solve ()
{
    // number of rows = number of cols
    Number numrows = this->numnodes + this->numbranches;

    // get all major diagonal values 1.0 except for last row
    // get all values below major diagonal 0.0
    for (Number row = 1; row < numrows; row ++) {

        // point to the row being processed
        Row *rowptr = this->rowslist[row-1];

        // pivot = abs (value from row on major diagonal)
        SVal pivav = rowptr->columns[row-1]->abs ();

        // find row below this one that has larger value in same column as this one's pivot
        // swap this one with it so we get the largest value on the major diagonal
        // this keeps us from using zero or very small values
        for (Number r = row; ++ r <= numrows;) {
            Row *rptr = this->rowslist[r-1];
            SVal pav = rptr->columns[row-1]->abs ();
            if (pav > pivav) {
                this->rowslist[r-1] = rowptr;
                this->rowslist[row-1] = rowptr = rptr;
                pivav = pav;
            }
        }

        if (pivav == 0.0) {
            char *err;
            char const *obj = NULL;
            char const *of  = NULL;
            if (row <= this->numnodes) {
                for (Node *node = this->nodelist; node != NULL; node = node->nextnode) {
                    if (node->nodenum == row) {
                        of  = "voltage of";
                        obj = node->nodename;
                        break;
                    }
                }
            } else {
                Number bn = row - this->numnodes;
                for (Branch *branch = this->branchlist; branch != NULL; branch = branch->nextbranch) {
                    if (branch->branchnum == bn) {
                        of  = "current through";
                        obj = branch->branchname;
                        break;
                    }
                }
            }
            asprintf (&err, "missing pivot value row/col %u, cannot compute %s %s", row, of, obj);
            throw (char const *) err;
        }

        // normalize row such that pivot is 1.0
        CVal pivot = rowptr->columns[row-1];
        for (Number col = row; col <= numrows; col ++) {
            CVal *p = &rowptr->columns[col-1];
            p->diveq (pivot);
        }

        // subtract a multiple of this row from all rows below it such that those rows
        // get zero in this row's major diagonal column
        for (Number r = row; ++ r <= numrows;) {
            Row *rptr = this->rowslist[r-1];
            CVal piv = rptr->columns[row-1];
            CVal *q = rowptr->columns;
            CVal *p = rptr->columns;
            for (Number col = numrows; col > 0; -- col) {
                p->subeq (q->mul (piv));
                p ++;
                q ++;
            }
        }
    }

    // get all values above major diagonal 0.0
    for (Number row = numrows; row > 1;) {
        Row *rowptr = this->rowslist[--row-1];
        for (Number r = row; r > 1;) {
            Row *rptr = this->rowslist[--r-1];
            CVal piv = rptr->columns[row-1];
            CVal *q = rowptr->columns;
            CVal *p = rptr->columns;
            for (Number col = numrows; col > 0; -- col) {
                p->subeq (q->mul (piv));
                p ++;
                q ++;
            }
        }
    }
}

void Matrix::print ()
{
    for (Number vn = 0; ++ vn < this->numnodes;) {
        printf ("              v_%03u             ", vn);
    }
    printf ("  ");
    for (Number in = 0; ++ in <= this->numbranches;) {
        printf ("              i_%03u             ", in);
    }
    printf ("                 =\n");

    Number numrows = this->numnodes + this->numbranches;
    for (Number row = 1; row <= numrows; row ++) {
        Row *rowptr = this->rowslist[row-1];
        CVal *p = rowptr->columns;
        for (Number col = 0; ++ col <= numrows;) {
            if ((col == this->numnodes) || (col == numrows)) printf ("  ");
            printf (" [ %11.4f + j %11.4f ]", p->real, p->imag);
            p ++;
        }
        printf ("\n");
    }
}

void Matrix::reset ()
{
    if (this->rowslist != NULL) {
        Number numrows = this->numnodes + this->numbranches;
        for (Number i = 0; i < numrows; i ++) {
            delete this->rowslist[i];
        }
        free (this->rowslist);
    }

    this->numnodes = 0;
    this->numbranches = 0;
    this->rowslist = NULL;
    this->nodelist = NULL;
    this->branchlist = NULL;
}

///////////
//  Row  //
///////////

Row::Row ();

//  1..nodecols-1   1..branchcols
//  <nodecolumns>  <branchcolumns>  <constcolumn>

Row *Row::init (Number numnodes, Number numbranches)
{
    this->numnodes = numnodes;
    this->numbranches = numbranches;
    this->columns = calloc (this->numnodes + this->numbranches, sizeof *this->columns);
    return this;
}

Row::~Row ()
{
    free (this->columns);
    this->columns = NULL;
}

void Row::addtonodecolumn (Number nodenum, CVal val)
{
    assert (nodenum < this->numnodes);
    if (nodenum > 0) {
        this->columns[nodenum-1].addeq (val);
    }
}

void Row::addtonodecolumnr (Number nodenum, SVal val)
{
    assert (nodenum < this->numnodes);
    if (nodenum > 0) {
        this->columns[nodenum-1].real += val;
    }
}

void Row::addtobranchcolumn (Number branchnum, CVal val)
{
    assert ((branchnum > 0) && (branchnum <= this->numbranches));
    this->columns[this->numnodes+branchnum-2].addeq (val);
}

void Row::addtobranchcolumnr (Number branchnum, SVal val)
{
    assert ((branchnum > 0) && (branchnum <= this->numbranches));
    this->columns[this->numnodes+branchnum-2].real += val;
}

void Row::addtobranchcolumni (Number branchnum, SVal val)
{
    assert ((branchnum > 0) && (branchnum <= this->numbranches));
    this->columns[this->numnodes+branchnum-2].imag += val;
}

void Row::addtoconstantcolumn (CVal val)
{
    this->columns[this->numnodes+this->numbranches-1].addeq (val);
}

CVal Row::getconstantcolumn ()
{
    return this->columns[this->numnodes+this->numbranches-1];
}
