/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "gage.h"
#include "private.h"

void
_gageVecPrint_query(unsigned int query) {
  unsigned int q;

  fprintf(stderr, "query = %u ...\n", query);
  q = GAGE_VEC_MAX+1;
  do {
    q--;
    if ((1<<q) & query) {
      fprintf(stderr, "    %3d: %s\n", q, airEnumStr(gageVec, q));
    }
  } while (q);
}

void
_gageVecPrint_iv3(gageContext *ctx, gagePerVolume *pvl) {
  
  fprintf(stderr, "_gageVecPrint_iv3() not implemented\n");
}
