/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ell.h"

#define PERP \
  idx = 0; \
  if (AIR_ABS(b[0]) < AIR_ABS(b[1])) \
    idx = 1; \
  if (AIR_ABS(b[idx]) < AIR_ABS(b[2])) \
    idx = 2; \
  switch (idx) { \
  case 0: \
    ELL_3V_SET(a, b[1] - b[2], -b[0], b[0]); \
    break; \
  case 1: \
    ELL_3V_SET(a, -b[1], b[0] - b[2], b[1]); \
    break; \
  case 2: \
    ELL_3V_SET(a, -b[2], b[2], b[0] - b[1]); \
    break; \
  }

/*
******** ell3vPerp_f()
**
** Given a 3-vector, produce one which is perpendicular.
** Output length won't be same as input length, but it will always
** be non-zero, if input length is non-zero.   This will NOT produce
** a unit-length vector if the input is unit length.
*/
void
ell3vPerp_f(float a[3], float b[3]) {
  int idx;
  PERP;
}

/*
******** ell3vPerp_d()
**
** same as above, but for doubles
*/
void
ell3vPerp_d(double a[3], double b[3]) {
  int idx;
  PERP;
}

void
ell3mvMul_f(float v2[3], float m[9], float v1[3]) {
  ELL_3MV_MUL(v2, m, v1);
}

void
ell3mvMul_d(double v2[3], double m[9], double v1[3]) {
  ELL_3MV_MUL(v2, m, v1);
}

void
ell4mvMul_f(float v2[4], float m[16], float v1[4]) {
  ELL_4MV_MUL(v2, m, v1);
}

void
ell4mvMul_d(double v2[4], double m[16], double v1[4]) {
  ELL_4MV_MUL(v2, m, v1);
}

