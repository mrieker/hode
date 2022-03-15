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

#include <math.h>

CType CType::make (CData r, CData i)
{
    CType c;
    c.real = r;
    c.imag = i;
    return c;
}

CType CType::add (CType x)
{
    CType y;
    y.real = this->real + x.real;
    y.imag = this->imag + x.imag;
    return y;
}

CType CType::sub (CType x)
{
    CType y;
    y.real = this->real - x.real;
    y.imag = this->imag - x.imag;
    return y;
}

//  (t.real + j * t.imag) * (x.real + j * x.imag) = (t.real * x.real - t.imag * x.imag) + j * (t.real * x.imag + t.imag * x.real)
CType CType::mul (CType x)
{
    CData r = this->real * x.real - this->imag * x.imag;
    CData i = this->real * x.imag + this->imag * x.real;
    CType y;
    y.real = r;
    y.imag = i;
    return y;
}

//  t.real + j * t.imag   (t.real + j * t.imag) * (x.real - j * x.imag)
//  ------------------- = ---------------------------------------------
//  x.real + j * x.imag             x.real ** 2 + x.imag ** 2
CType CType::div (CType x)
{
    CData d = x.real * x.real + x.imag * x.imag;
    CType y;
    y.real = (this->real * x.real + this->imag * x.imag) / d;
    y.imag = (this->imag * x.real - this->real * x.imag) / d;
    return y;
}

// 1 / (1 / this + 1 / x))
CType CType::par (CType x)
{
    CType n = this->mul (x);
    CType d = this->add (x);
    return n.div (d);
}

CType CType::neg ()
{
    CType y;
    y.real = - this->real;
    y.imag = - this->imag;
    return y;
}

// 1.0 / this
CType CType::rec ()
{
    CData d = this->real * this->real + this->imag * this->imag;
    CType y;
    y.real =   this->real / d;
    y.imag = - this->imag / d;
    return y;
}

CData CType::abs ()
{
    return CMath (sqrt) (this->real * this->real + this->imag * this->imag);
}

CData CType::ang ()
{
    return CMath (atan2) (this->imag, this->real);
}

void CType::addeq (CType x)
{
    this->real += x.real;
    this->imag += x.imag;
}

void CType::subeq (CType x)
{
    this->real -= x.real;
    this->imag -= x.imag;
}

//  (t.real + j * t.imag) * (x.real + j * x.imag) = (t.real * x.real - t.imag * x.imag) + j * (t.real * x.imag + t.imag * x.real)
void CType::muleq (CType x)
{
    CData r = this->real * x.real - this->imag * x.imag;
    this->imag = this->real * x.imag + this->imag * x.real;
    this->real = r;
}

//  t.real + j * t.imag   (t.real + j * t.imag) * (x.real - j * x.imag)
//  ------------------- = ---------------------------------------------
//  x.real + j * x.imag             x.real ** 2 + x.imag ** 2
void CType::diveq (CType x)
{
    CData d = x.real * x.real + x.imag * x.imag;
    CData r = (this->real * x.real + this->imag * x.imag) / d;
    this->imag = (this->imag * x.real - this->real * x.imag) / d;
    this->real = r;
}
