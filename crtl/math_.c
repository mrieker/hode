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

// returns 0..PI
MType MMath(acos) (MType x)
{
    return MPI / 2.0 - MMath(asin) (x);
}

// returns -PI/2..+PI/2
MType MMath(asin) (MType x)
{
    if (MMath(isnan) (x) || (x < -1.0) || (x > 1.0)) return MNAN;
    MType xx = x * x;
    MType i  = 1.0;
    MType y  = x;
    MType y1 = y;
    MType y2 = y;
    while (1) {
        x *= i;
        x /= ++ i;
        x *= xx;
        y += x / (++ i);
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

// returns -PI/2..+PI/2
MType MMath(atan) (MType x)
{
    if (MMath(isnan) (x)) return x;
    if (x == 0.0) return x;
    if (x < 0.0) return - MMath(atan) (- x);
    if (MMath(isinf) (x)) return MPI / 2.0;
    if (x > 1.0) return MPI / 2.0 - MMath(atan) (1.0 / x);
    if (x == 1.0) return MPI / 4.0;

    // y = x - x**3/3 + x**5/5 - x**7/7 + x**9/9 ...
    MType mxx = - x * x;
    MType d   = 1.0;
    MType y1  = x;
    MType y2  = x;
    while (1) {
        x *= mxx;
        d += 2.0;
        MType y = y1 + x / d;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

// returns -PI..+PI
MType MMath(atan2) (MType y, MType x)
{
    if (MMath(isnan) (y) || MMath(isnan) (x)) return MNAN;
    if (x == 0.0) {
        if (y == 0.0) return 0.0;
        if (y < 0.0) return - MPI / 2.0;
        return MPI / 2.0;
    }
    MType theta = MMath(atan) (y / x);
    if (MMath(isnan) (theta)) return theta;
    if (x >= 0.0) return theta;
    return (y >= 0.0) ? theta + MPI : theta - MPI;
}

MType MMath(cos) (MType x)
{
    if (! MMath(isfinite) (x)) return MNAN;
    if (x < 0.0) x = -x;
    while (x >= MPI) x -= MPI * 2.0;
    MType mxx = - x * x;
    MType m   = 0.0;
    MType y   = 1.0;
    MType y1  = 1.0;
    MType y2  = 1.0;
    x = 1.0;
    while (1) {
        x *= mxx;
        x /= ++ m;
        x /= ++ m;
        y += x;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

MType MMath(exp) (MType x)
{
    if (! MMath(isfinite) (x)) {
        if (MMath(isnan) (x)) return MNAN;
        return (x < 0.0) ? 0.0 : MINF;
    }

    if (x == 0.0) return 1.0;
    if (x < 0.0) {
        return 1.0 / MMath (exp) (- x);
    }

    MType n  = x;
    MType f  = 1.0;
    MType y  = x + 1.0;
    MType y1 = y;
    MType y2 = y;
    while (1) {
        n *= x / ++ f;
        y += n;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

MType MMath(log) (MType x)
{
    if (x < 0.0) return MNAN;
    if (x == 0.0) return - MINF;

    bool neg = x > 1.0;
    if (neg) x = 1.0 / x;
    if (x == 1.0) return 0.0;

    MType u  = x - 1.0;
    MType mu = - u;
    MType d  = 1.0;
    MType y  = u;
    MType y1 = u;
    MType y2 = u;
    while (1) {
        u *= mu;
        y += u / ++ d;
        if ((y == y1) || (y == y2)) return neg ? - y : y;
        y2 = y1;
        y1 = y;
    }
}

MType MMath(pow) (MType x, MType y)
{
    if (y == 0.0) return 1.0;
    if (x == 1.0) return 1.0;
    if (x == 0.0) return (y < 0.0) ? MINF : 0.0;
    if (MMath(isnan) (y) || MMath(isnan) (x)) return MNAN;
    if ((x == -1.0) && MMath(isinf) (y)) return 1.0;

    if ((x > -1.0) && (x < 1.0)) {
        if (y ==  MINF) return 0.0;
        if (y == -MINF) return (x < 0.0) ? -MINF : MINF;
    }
    else if ((x < -1.0) && (x > 1.0)) {
        if (y ==  MINF) return (x < 0.0) ? -MINF : MINF;
        if (y == -MINF) return 0.0;
    }

    if (y < 0.0) {
        return 1.0 / MMath(pow) (x, - y);
    }

    __uint64_t i = (__uint64_t) y;
    if ((MType) i == y) {
        MType z = 1.0;
        while (i > 0) {
            if (i & 1) z *= x;
            x *= x;
            i /= 2;
        }
        return z;
    }

    if (x < 0.0) return MNAN;

    return MMath(exp) (MMath(log) (x) * y);
}

MType MMath(sin) (MType x)
{
    if (! MMath(isfinite) (x)) return MNAN;
    while (x < -MPI) x += MPI * 2.0;
    while (x >= MPI) x -= MPI * 2.0;
    MType mxx = - x * x;
    MType m   = 1.0;
    MType y   = x;
    MType y1  = x;
    MType y2  = x;
    while (1) {
        x *= mxx;
        x /= ++ m;
        x /= ++ m;
        y += x;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

MType MMath(sqrt) (MType x)
{
    if (x == 0.0) return 0.0;
    if (x < 0.0) return MNAN;
    if (! MMath(isfinite) (x)) return x;

    MType y1, y2;
    if (sizeof x == 4) {
        union { MType f; __uint16_t u[2]; } v;
        v.f = x;
        v.u[1] = (v.u[1] & 0x007FU) | ((v.u[1] & 0x7F00U) / 2 +  (64 << 7));
        y1 = y2 = v.f;
    }
    if (sizeof x == 8) {
        union { MType f; __uint16_t u[4]; } v;
        v.f = x;
        v.u[3] = (v.u[3] & 0x000FU) | ((v.u[3] & 0x7FE0U) / 2 + (512 << 4));
        y1 = y2 = v.f;
    }

    while (1) {
        MType y = (x / y1 + y1) / 2.0;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

MType MMath(tan) (MType x)
{
    return MMath(sin) (x) / MMath(cos) (x);
}
