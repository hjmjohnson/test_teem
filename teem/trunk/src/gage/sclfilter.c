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

#define X 0
#define Y 1
#define Z 2

void
_gageScl3PFilter2(gage_t *ivX, gage_t *ivY, gage_t *ivZ,
		  gage_t *fw0, gage_t *fw1, gage_t *fw2,
		  gage_t *val, gage_t *gvec, gage_t *hess,
		  int doV, int doD1, int doD2) {

  /* fw? + 2*?
       |     |  
       |     +- along which axis (0:x, 1:y, 2:z)
       |
       + what information (0:value, 1:1st deriv, 2:2nd deriv)

     ivX: 3D cube cache of original volume values
          (its scanlines are along the X axis)
     ivY: 2D square cache of intermediate filter results
          (its scanlines are along the Y axis)
     ivZ: 1D linear cache of intermediate filter results
          (it is a scanline along the Z axis)
  */

#define DOT2(a, b) ((a)[0]*(b)[0] + (a)[1]*(b)[1])
#define VL_2(i, axis) (DOT2(fw0 + (axis)*2, iv##axis + i*2))
#define D1_2(i, axis) (DOT2(fw1 + (axis)*2, iv##axis + i*2))
#define D2_2(i, axis) (DOT2(fw2 + (axis)*2, iv##axis + i*2))

  /* x0 */
  ivY[0] = VL_2(0,X); /* interpolate values of 0th scanline along X axis */
  ivY[1] = VL_2(1,X);
  ivY[2] = VL_2(2,X);
  ivY[3] = VL_2(3,X);
  /* x0y0 */
  ivZ[0] = VL_2(0,Y);
  ivZ[1] = VL_2(1,Y);
  /* x0y0z0 */
  /* if (doV) ... but just do it anyway */
  *val = VL_2(0,Z);                            /* f */

  if (!( doD1 || doD2 ))
    return;

  /* x0y0z1 */
  /* if (doD1) */
  gvec[2] = D1_2(0,Z);                         /* g_z */
  if (doD2) {
    /* x0y0z2 */
    hess[8] = D2_2(0,Z);                       /* h_zz */
  }
  /* x0y1 */
  ivZ[0] = D1_2(0,Y);
  ivZ[1] = D1_2(1,Y);
  /* x0y1z0 */
  /* if (doD1) */
  gvec[1] = VL_2(0,Z);                         /* g_y */
  if (doD2) {
    /* x0y1z1 */
    hess[5] = hess[7] = D1_2(0,Z);             /* h_yz */
    /* x0y2 */
    ivZ[0] = D2_2(0,Y);
    ivZ[1] = D2_2(1,Y);
    /* x0y2z0 */
    hess[4] = VL_2(0,Z);                       /* h_yy */
  }
  /* x1 */
  ivY[0] = D1_2(0,X);
  ivY[1] = D1_2(1,X);
  ivY[2] = D1_2(2,X);
  ivY[3] = D1_2(3,X);
  /* x1y0 */
  ivZ[0] = VL_2(0,Y);
  ivZ[1] = VL_2(1,Y);
  /* x1y0z0 */
  /* if (doD1) */
  gvec[0] = VL_2(0,Z);                         /* g_x */

  if (!doD2)
    return;

  /* x1y0z1 */
  hess[2] = hess[6] = D1_2(0,Z);               /* h_xz */
  /* x1y1 */
  ivZ[0] = D1_2(0,Y);
  ivZ[1] = D1_2(1,Y);
  /* x1y1z0 */
  hess[1] = hess[3] = VL_2(0,Z);               /* h_xy */
  /* x2 */
  ivY[0] = D2_2(0,X);
  ivY[1] = D2_2(1,X);
  ivY[2] = D2_2(2,X);
  ivY[3] = D2_2(3,X);
  /* x2y0 */
  ivZ[0] = VL_2(0,Y);
  ivZ[1] = VL_2(1,Y);
  /* x2y0z0 */
  hess[0] = VL_2(0,Z);                         /* h_xx */

  return;
}

void
_gageScl3PFilter4(gage_t *ivX, gage_t *ivY, gage_t *ivZ,
		  gage_t *fw0, gage_t *fw1, gage_t *fw2,
		  gage_t *val, gage_t *gvec, gage_t *hess,
		  int doV, int doD1, int doD2) {

  /* fw? + 4*?
       |     |  
       |     +- along which axis (0:x, 1:y, 2:z)
       |
       + what information (0:value, 1:1st deriv, 2:2nd deriv)

     ivX: 3D cube cache of original volume values
          (its scanlines are along the X axis)
     ivY: 2D square cache of intermediate filter results
          (its scanlines are along the Y axis)
     ivZ: 1D linear cache of intermediate filter results
          (it is a scanline along the Z axis)
  */

#define DOT4(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
#define VL_4(i, axis) (DOT4(fw0 + (axis)*4, iv##axis + i*4))
#define D1_4(i, axis) (DOT4(fw1 + (axis)*4, iv##axis + i*4))
#define D2_4(i, axis) (DOT4(fw2 + (axis)*4, iv##axis + i*4))

  /* x0 */
  printf("fw0 = %p; ivX = %p\n", fw0, ivX);
  ivY[ 0] = VL_4( 0,X);
  ivY[ 1] = VL_4( 1,X);
  ivY[ 2] = VL_4( 2,X);
  ivY[ 3] = VL_4( 3,X);
  ivY[ 4] = VL_4( 4,X);
  ivY[ 5] = VL_4( 5,X);
  ivY[ 6] = VL_4( 6,X);
  ivY[ 7] = VL_4( 7,X);
  ivY[ 8] = VL_4( 8,X);
  ivY[ 9] = VL_4( 9,X);
  ivY[10] = VL_4(10,X);
  ivY[11] = VL_4(11,X);
  ivY[12] = VL_4(12,X);
  ivY[13] = VL_4(13,X);
  ivY[14] = VL_4(14,X);
  ivY[15] = VL_4(15,X);
  /* x0y0 */
  ivZ[ 0] = VL_4( 0,Y);
  ivZ[ 1] = VL_4( 1,Y);
  ivZ[ 2] = VL_4( 2,Y);
  ivZ[ 3] = VL_4( 3,Y);
  /* x0y0z0 */
  /* if (doV) ... but just do it anyway */
  *val = VL_4( 0,Z);                            /* f */

  if (!( doD1 || doD2 ))
    return;

  /* x0y0z1 */
  if (doD1) {
    gvec[2] = D1_4( 0,Z);                       /* g_z */
  }
  if (doD2) {
    /* x0y0z2 */
    hess[8] = D2_4( 0,Z);                       /* h_zz */
  }
  /* x0y1 */
  ivZ[ 0] = D1_4( 0,Y);
  ivZ[ 1] = D1_4( 1,Y);
  ivZ[ 2] = D1_4( 2,Y);
  ivZ[ 3] = D1_4( 3,Y);
  /* x0y1z0 */
  if (doD1) {
    gvec[1] = VL_4( 0,Z);                       /* g_y */
  }
  if (doD2) {
    /* x0y1z1 */
    hess[5] = hess[7] = D1_4( 0,Z);             /* h_yz */
    /* x0y2 */
    ivZ[ 0] = D2_4( 0,Y);
    ivZ[ 1] = D2_4( 1,Y);
    ivZ[ 2] = D2_4( 2,Y);
    ivZ[ 3] = D2_4( 3,Y);
    /* x0y2z0 */
    hess[4] = VL_4( 0,Z);                       /* h_yy */
  }
  /* x1 */
  ivY[ 0] = D1_4( 0,X);
  ivY[ 1] = D1_4( 1,X);
  ivY[ 2] = D1_4( 2,X);
  ivY[ 3] = D1_4( 3,X);
  ivY[ 4] = D1_4( 4,X);
  ivY[ 5] = D1_4( 5,X);
  ivY[ 6] = D1_4( 6,X);
  ivY[ 7] = D1_4( 7,X);
  ivY[ 8] = D1_4( 8,X);
  ivY[ 9] = D1_4( 9,X);
  ivY[10] = D1_4(10,X);
  ivY[11] = D1_4(11,X);
  ivY[12] = D1_4(12,X);
  ivY[13] = D1_4(13,X);
  ivY[14] = D1_4(14,X);
  ivY[15] = D1_4(15,X);
  /* x1y0 */
  ivZ[ 0] = VL_4( 0,Y);
  ivZ[ 1] = VL_4( 1,Y);
  ivZ[ 2] = VL_4( 2,Y);
  ivZ[ 3] = VL_4( 3,Y);
  /* x1y0z0 */
  if (doD1) {
    gvec[0] = VL_4( 0,Z);                       /* g_x */
  }

  if (!doD2)
    return;

  /* x1y0z1 */
  hess[2] = hess[6] = D1_4( 0,Z);               /* h_xz */
  /* x1y1 */
  ivZ[ 0] = D1_4( 0,Y);
  ivZ[ 1] = D1_4( 1,Y);
  ivZ[ 2] = D1_4( 2,Y);
  ivZ[ 3] = D1_4( 3,Y);
  /* x1y1z0 */
  hess[1] = hess[3] = VL_4( 0,Z);               /* h_xy */
  /* x2 */
  ivY[ 0] = D2_4( 0,X);
  ivY[ 1] = D2_4( 1,X);
  ivY[ 2] = D2_4( 2,X);
  ivY[ 3] = D2_4( 3,X);
  ivY[ 4] = D2_4( 4,X);
  ivY[ 5] = D2_4( 5,X);
  ivY[ 6] = D2_4( 6,X);
  ivY[ 7] = D2_4( 7,X);
  ivY[ 8] = D2_4( 8,X);
  ivY[19] = D2_4( 9,X);
  ivY[10] = D2_4(10,X);
  ivY[11] = D2_4(11,X);
  ivY[12] = D2_4(12,X);
  ivY[13] = D2_4(13,X);
  ivY[14] = D2_4(14,X);
  ivY[15] = D2_4(15,X);
  /* x2y0 */
  ivZ[ 0] = VL_4( 0,Y);
  ivZ[ 1] = VL_4( 1,Y);
  ivZ[ 2] = VL_4( 2,Y);
  ivZ[ 3] = VL_4( 3,Y);
  /* x2y0z0 */
  hess[0] = VL_4( 0,Z);                         /* h_xx */

  return;
}

