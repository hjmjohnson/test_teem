#include "bane.h"

double
_baneMeasrUnknown(Nrrd *n, NRRD_BIG_INT idx) {
  char me[]="_baneMeasrUnknown";
  
  fprintf(stderr, "%s: Need To Specify A Measure !!!\n", me);
  return airNand();
}

double
_baneMeasrVal(Nrrd *n, NRRD_BIG_INT idx) {

  return nrrdDLookup[n->type](n->data, idx);
}

double
_baneMeasrGradMag_cd(Nrrd *n, NRRD_BIG_INT idx) {
  double (*lookup)(void *, NRRD_BIG_INT), dx, dy, dz, gm, *spc;
  int sx, sxy;

  lookup = nrrdDLookup[n->type]; sx = n->size[0]; sxy = sx*n->size[1];
  spc = n->spacing;

  dx = (lookup(n->data, idx   +1) - lookup(n->data, idx   -1))/(2*spc[0]);
  dy = (lookup(n->data, idx  +sx) - lookup(n->data, idx  -sx))/(2*spc[1]);
  dz = (lookup(n->data, idx +sxy) - lookup(n->data, idx -sxy))/(2*spc[2]);
  gm = sqrt(dx*dx + dy*dy + dz*dz);

  return gm;
}

double
_baneMeasrLapl_cd(Nrrd *n, NRRD_BIG_INT idx) {
  double (*lookup)(void *, NRRD_BIG_INT), cv, *spc, lapl;
  int sx, sxy;

  lookup = nrrdDLookup[n->type]; sx = n->size[0]; sxy = sx*n->size[1];
  spc = n->spacing;
  cv = lookup(n->data, idx);

  lapl = ( (lookup(n->data, idx + 1) - 2*cv + 
	    lookup(n->data, idx - 1))/(spc[0]*spc[0]) +
	   (lookup(n->data, idx + sx) - 2*cv +
	    lookup(n->data, idx - sx))/(spc[1]*spc[1]) +
	   (lookup(n->data, idx + sxy) - 2*cv + 
	    lookup(n->data, idx - sxy))/(spc[2]*spc[2]) );

  return lapl;
}

void
_baneMeasrFillCache(Nrrd *n, NRRD_BIG_INT idx, double V[3][3][3]) {
  double (*lookup)(void *, NRRD_BIG_INT);
  int sx, sxy;

  lookup = nrrdDLookup[n->type]; sx = n->size[0]; sxy = sx*n->size[1];

  V[0][0][0] = lookup(n->data, idx -1 -sx -sxy);
  V[0][0][1] = lookup(n->data, idx    -sx -sxy);
  V[0][0][2] = lookup(n->data, idx +1 -sx -sxy);
  V[0][1][0] = lookup(n->data, idx -1     -sxy);
  V[0][1][1] = lookup(n->data, idx        -sxy);
  V[0][1][2] = lookup(n->data, idx +1     -sxy);
  V[0][2][0] = lookup(n->data, idx -1 +sx -sxy);
  V[0][2][1] = lookup(n->data, idx    +sx -sxy);
  V[0][2][2] = lookup(n->data, idx +1 +sx -sxy);
  V[1][0][0] = lookup(n->data, idx -1 -sx     );
  V[1][0][1] = lookup(n->data, idx    -sx     );
  V[1][0][2] = lookup(n->data, idx +1 -sx     );
  V[1][1][0] = lookup(n->data, idx -1         );
  V[1][1][1] = lookup(n->data, idx            );
  V[1][1][2] = lookup(n->data, idx +1         );
  V[1][2][0] = lookup(n->data, idx -1 +sx     );
  V[1][2][1] = lookup(n->data, idx    +sx     );
  V[1][2][2] = lookup(n->data, idx +1 +sx     );
  V[2][0][0] = lookup(n->data, idx -1 -sx +sxy);
  V[2][0][1] = lookup(n->data, idx    -sx +sxy);
  V[2][0][2] = lookup(n->data, idx +1 -sx +sxy);
  V[2][1][0] = lookup(n->data, idx -1     +sxy);
  V[2][1][1] = lookup(n->data, idx        +sxy);
  V[2][1][2] = lookup(n->data, idx +1     +sxy);
  V[2][2][0] = lookup(n->data, idx -1 +sx +sxy);
  V[2][2][1] = lookup(n->data, idx    +sx +sxy);
  V[2][2][2] = lookup(n->data, idx +1 +sx +sxy);
}

/*
** layout of V
** 
**    Z
**    ^
**    |
**    |
**    | 220  221  222
**     210  211  212
**    200  201  202
**                       Y
**      120  121  122   ^
**     110  111  112   /
**    100  101  102   /
**                      
**      020  021  022
**     010  011  012
**    000  001  002  ----> X
**
**
** layout of H
**
** 00  01  02
** 10  11  12
** 20  21  22
**
** dxy= (V[1][2][2] - V[1][0][2] - V[1][2][0] + V[1][0][0])/(4*spc[0]*spc[1]);
*/
double
_baneMeasrHess_cd(Nrrd *n, NRRD_BIG_INT idx) {
  double V[3][3][3], dxx, dyy, dzz, dxy, dxz, dyz, 
    dx, dy, dz, *spc, dot1, dot2, dot3, hess, mag;

  _baneMeasrFillCache(n, idx, V);
  spc = n->spacing;

  dx = (V[1][1][2] - V[1][1][0])/(2*spc[0]);
  dy = (V[1][2][1] - V[1][0][1])/(2*spc[1]);
  dz = (V[2][1][1] - V[0][1][1])/(2*spc[2]);
  mag = sqrt(dx*dx + dy*dy + dz*dz);

  if (mag) {
    dx /= mag;
    dy /= mag;
    dz /= mag;
    dxx = (V[1][1][2] - 2*V[1][1][1] + V[1][1][0])/(spc[0]*spc[0]);
    dyy = (V[1][2][1] - 2*V[1][1][1] + V[1][0][1])/(spc[1]*spc[1]);
    dzz = (V[2][1][1] - 2*V[1][1][1] + V[0][1][1])/(spc[2]*spc[2]);
    dxy= (V[1][2][2] - V[1][0][2] - V[1][2][0] + V[1][0][0])/(4*spc[0]*spc[1]);
    dxz= (V[2][1][2] - V[0][1][2] - V[2][1][0] + V[0][1][0])/(4*spc[0]*spc[2]);
    dyz= (V[2][2][1] - V[0][2][1] - V[2][0][1] + V[0][0][1])/(4*spc[1]*spc[2]);
    dot1 = dxx*dx + dxy*dy + dxz*dz;
    dot2 = dxy*dx + dyy*dy + dyz*dz;
    dot3 = dxz*dx + dyz*dy + dzz*dz;
    hess = dx*dot1 + dy*dot2 + dz*dot3;
  }
  else {
    /* I doubt this is correct */
    hess = 0;
  }

  return hess;
}

double
_baneMeasrGMG_cd(Nrrd *n, NRRD_BIG_INT idx) {
  double (*lookup)(void *, NRRD_BIG_INT), d[3], G[3], *spc, mag, gmg;
  int sx, sxy;

  lookup = nrrdDLookup[n->type]; sx = n->size[0]; sxy = sx*n->size[1];
  spc = n->spacing;
  
  G[0] = (lookup(n->data, idx   +1) - lookup(n->data, idx   -1))/(2*spc[0]);
  G[1] = (lookup(n->data, idx  +sx) - lookup(n->data, idx  -sx))/(2*spc[1]);
  G[2] = (lookup(n->data, idx +sxy) - lookup(n->data, idx -sxy))/(2*spc[2]);
  mag = sqrt(G[0]*G[0] + G[1]*G[1] + G[2]*G[2]);

  if (mag) {
    d[0] = (_baneMeasrGradMag_cd(n,idx  +1) - 
	    _baneMeasrGradMag_cd(n,idx  -1))/(2*spc[0]);
    d[1] = (_baneMeasrGradMag_cd(n,idx +sx) - 
	    _baneMeasrGradMag_cd(n,idx -sx))/(2*spc[1]);
    d[2] = (_baneMeasrGradMag_cd(n,idx+sxy) - 
	    _baneMeasrGradMag_cd(n,idx-sxy))/(2*spc[2]);
    gmg = (d[0]*G[0] + d[1]*G[1] + d[2]*G[2])/mag;
  }
  else {
    /* I doubt this is correct */
    gmg = 0;
  }

  return gmg;
}

double
(*baneMeasr[BANE_MEASR_MAX+1])(Nrrd *n, NRRD_BIG_INT idx) = {
  _baneMeasrUnknown,
  _baneMeasrVal,
  _baneMeasrGradMag_cd,
  _baneMeasrLapl_cd,
  _baneMeasrHess_cd,
  _baneMeasrGMG_cd
};
