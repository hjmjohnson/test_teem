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

#include "ten.h"
#include "tenPrivate.h"

int
_tenEpiRegCheck(Nrrd *nout, Nrrd **nin, int ninLen, Nrrd *ngrad,
		int reference,
		float bwX, float bwY, float B0thr, float DWthr,
		NrrdKernel *kern, double *kparm) {
  char me[]="_tenEpiRegCheck", err[AIR_STRLEN_MED];
  int ni;

  if (!( nout && nin && ngrad && kern && kparm )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( ninLen >= 3 )) {
    sprintf(err, "%s: given ninLen (%d) not >= 3", me, ninLen);
    biffAdd(TEN, err); return 1;
  }
  if (!( 2 == ngrad->dim 
	 && (nrrdTypeFloat == ngrad->type || nrrdTypeDouble == ngrad->type)
	 && 3 == ngrad->axis[0].size
	 && ninLen-1 == ngrad->axis[1].size )) {
    sprintf(err, "%s: given gradient list not a 2-D 3-by-%d array of "
	    "floats or doubles", me, ninLen-1);
    biffAdd(TEN, err); return 1;
  }
  for (ni=0; ni<ninLen; ni++) {
    if (nrrdCheck(nin[ni])) {
      sprintf(err, "%s: basic nrrd validity failed on nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    if (!nrrdSameSize(nin[0], nin[ni], AIR_TRUE)) {
      sprintf(err, "%s: nin[%d] is different from nin[0]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
  }
  if (!( 3 == nin[0]->dim )) {
    sprintf(err, "%s: didn't get a set of 3-D arrays (got %d-D)", me,
	    nin[0]->dim);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_IN_CL(0, reference, ninLen-1) )) {
    sprintf(err, "%s: reference index %d not in valid range [0,%d]", 
	    me, reference, ninLen-1);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_EXISTS(bwX) && AIR_EXISTS(bwY) && 
	 AIR_EXISTS(B0thr) && AIR_EXISTS(DWthr) )) {
    sprintf(err, "%s: not all bwX, bwY, B0thr, DWthr exist", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( bwX >= 0 && bwY >= 0 )) {
    sprintf(err, "%s: bwX (%g) and bwY (%g) are not both non-negative",
	    me, bwX, bwY);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

/*
** this assumes that all nblur[i] are valid nrrds, and does nothing
** to manage them
*/
int
_tenEpiRegBlur(Nrrd **nblur, Nrrd **nin, int ninLen,
	       float bwX, float bwY, int verb) {
  char me[]="_tenEpiRegBlur", err[AIR_STRLEN_MED];
  NrrdResampleInfo *rinfo;
  airArray *mop;
  int ni, sx, sy, sz;
  double savemin[2], savemax[2];

  if (!( bwX || bwY )) {
    if (verb) {
      fprintf(stderr, "%s:\n            ", me); fflush(stderr);
    }
    for (ni=0; ni<ninLen; ni++) {
      if (verb) {
	fprintf(stderr, "% 2d ", ni); fflush(stderr);
      }
      if (nrrdCopy(nblur[ni], nin[ni])) {
	sprintf(err, "%s: trouble copying nin[%d]", me, ni);
	biffMove(TEN, err, NRRD); return 1;
      }
    }
    if (verb) {
      fprintf(stderr, "done\n");
    }
    return 0;
  }
  /* else we need to blur */
  sx = nin[0]->axis[0].size;
  sy = nin[0]->axis[1].size;
  sz = nin[0]->axis[2].size;
  mop = airMopNew();
  rinfo = nrrdResampleInfoNew();
  airMopAdd(mop, rinfo, (airMopper)nrrdResampleInfoNix, airMopAlways);
  if (bwX) {
    rinfo->kernel[0] = nrrdKernelGaussian;
    rinfo->parm[0][0] = bwX;
    rinfo->parm[0][1] = 3.0; /* how many stnd devs do we cut-off at */
  }
  if (bwY) {
    rinfo->kernel[1] = nrrdKernelGaussian;
    rinfo->parm[1][0] = bwY;
    rinfo->parm[1][1] = 3.0; /* how many stnd devs do we cut-off at */
  }
  rinfo->kernel[2] = NULL;
  ELL_3V_SET(rinfo->samples, sx, sy, sz);
  ELL_3V_SET(rinfo->min, 0, 0, 0);
  ELL_3V_SET(rinfo->max, sx-1, sy-1, sz-1);
  rinfo->boundary = nrrdBoundaryBleed;
  rinfo->type = nrrdTypeUnknown;
  rinfo->renormalize = AIR_TRUE;
  rinfo->clamp = AIR_TRUE;
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    savemin[0] = nin[ni]->axis[0].min; savemax[0] = nin[ni]->axis[0].max; 
    savemin[1] = nin[ni]->axis[1].min; savemax[1] = nin[ni]->axis[1].max;
    nin[ni]->axis[0].min = 0; nin[ni]->axis[0].max = sx-1;
    nin[ni]->axis[1].min = 0; nin[ni]->axis[1].max = sy-1;
    if (nrrdSpatialResample(nblur[ni], nin[ni], rinfo)) {
      sprintf(err, "%s: trouble blurring nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    nin[ni]->axis[0].min = savemin[0]; nin[ni]->axis[0].max = savemax[0]; 
    nin[ni]->axis[1].min = savemin[1]; nin[ni]->axis[1].max = savemax[1];
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }
  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegThreshold(Nrrd **nthresh, Nrrd **nblur, int ninLen,
		    float B0thr, float DWthr, int verb) {
  char me[]="_tenEpiRegThreshold", err[AIR_STRLEN_MED];
  airArray *mop;
  int I, sx, sy, sz, ni;
  float val;
  unsigned char *thr;
  
  mop = airMopNew();
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  sx = nblur[0]->axis[0].size;
  sy = nblur[0]->axis[1].size;
  sz = nblur[0]->axis[2].size;
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdMaybeAlloc(nthresh[ni], nrrdTypeUChar, 3, sx, sy, sz)) {
      sprintf(err, "%s: trouble allocating threshold %d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    thr = (unsigned char *)(nthresh[ni]->data);
    for (I=0; I<sx*sy*sz; I++) {
      val = nrrdFLookup[nblur[ni]->type](nblur[ni]->data, I);
      val -= !ni ? B0thr : DWthr;
      thr[I] = (val >= 0 ? 1 : 0);
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }
  
  airMopOkay(mop); 
  return 0;
}

int
_tenEpiRegCC(Nrrd **nthr, int ninLen,
	     int conny, int darkSize, int brightSize, int verb) {
  char me[]="_tenEpiRegCC", err[AIR_STRLEN_MED];
  Nrrd *nslc, *ncc, *nval;
  airArray *mop;
  int ni, z, sz, num;
  
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  mop = airMopNew();
  airMopAdd(mop, nslc=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nval=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ncc=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  sz = nthr[0]->axis[2].size;
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    for (z=0; z<sz; z++) {
      if ( nrrdSlice(nslc, nthr[ni], 2, z)
	   || nrrdCCFind(ncc, &nval, nslc, nrrdTypeUnknown, conny)
	   || nrrdCCMerge(ncc, ncc, nval, 1, darkSize, 0, conny)
	   || nrrdCCMerge(ncc, ncc, nval, -1, brightSize, 0, conny)
	   || nrrdCCRevalue(nslc, ncc, nval)
	   || nrrdSplice(nthr[ni], nthr[ni], nslc, 2, z) ) {
	sprintf(err, "%s: trouble processing slice %d of nthr[%d]", me, z, ni);
	biffMove(TEN, err, NRRD); return 1;
      }
      num = nrrdCCNum(ncc);
      if (2 != num) {
	fprintf(stderr, "%s: slice %d of nthr[%d] has %d CCs\n",
		me, z, ni, num);
      }
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  airMopOkay(mop);
  return 0;
}

#define MEAN_X 0 
#define MEAN_Y 1
#define M_02   2
#define M_11   3
#define M_20   4

/*
** _tenEpiRegMoments()
**
** the moments are stored in (of course) a nrrd, one scanline per slice,
** with each scanline containing:
**
**       0       1       2       3       4
**   mean(x)  mean(y)  M_02    M_11    M_20
*/
int
_tenEpiRegMoments(Nrrd **nmom, Nrrd **nthresh, int ninLen, int verb) {
  char me[]="_tenEpiRegMoments", err[AIR_STRLEN_MED];
  int sx, sy, sz, xi, yi, zi, ni;
  double N, mx, my, cx, cy, x, y, M02, M11, M20, *mom;
  float val;
  unsigned char *thr;

  sx = nthresh[0]->axis[0].size;
  sy = nthresh[0]->axis[1].size;
  sz = nthresh[0]->axis[2].size;
  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdMaybeAlloc(nmom[ni], nrrdTypeDouble, 2, 5, sz)) {
      sprintf(err, "%s: couldn't allocate nmom[%d]", me, ni);
      biffMove(TEN, err, NRRD); return 1;
    }
    nrrdAxesSet(nmom[ni], nrrdAxesInfoLabel, "mx,my,h,s,t", "z");
    thr = (unsigned char *)(nthresh[ni]->data);
    mom = (double *)(nmom[ni]->data);
    for (zi=0; zi<sz; zi++) {
      /* ------ find mx, my */
      N = 0;
      mx = my = 0.0;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  val = thr[xi + sx*yi];
	  N += val;
	  mx += xi*val;
	  my += yi*val;
	}
      }
      if (!N) {
	sprintf(err, "%s: saw no non-zero pixels in nthresh[%d]; "
		"%s threshold too high?", me, ni, !ni ? "B0" : "DW");
	biffAdd(TEN, err); return 1;
      }
      if (N == sx*sy) {
	sprintf(err, "%s: saw only non-zero pixels in nthresh[%d]; "
		"%s threshold too low?", me, ni, !ni ? "B0" : "DW");
	biffAdd(TEN, err); return 1;
      }
      mx /= N;
      my /= N;
      cx = sx/2.0;
      cy = sy/2.0;
      /* ------ find M02, M11, M20 */
      M02 = M11 = M20 = 0.0;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  val = thr[xi + sx*yi];
	  x = xi - cx;
	  y = yi - cy;
	  M02 += y*y*val;
	  M11 += x*y*val;
	  M20 += x*x*val;
	}
      }
      M02 /= N;
      M11 /= N;
      M20 /= N;
      /* ------ set output */
      mom[MEAN_X] = mx;
      mom[MEAN_Y] = my;
      mom[M_02] = M02;
      mom[M_11] = M11;
      mom[M_20] = M20;
      thr += sx*sy;
      mom += 5;
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  return 0;
}

/*
** _tenEpiRegPairXforms
**
** uses moment information to compute all pair-wise transforms, which are
** stored in the 3 x ninLen x ninLen x sizeZ output.  If xfr = npxfr->data,
** xfr[0 + 3*(zi + sz*(A + ninLen*B))] is shear,
** xfr[1 +              "            ] is scale, and 
** xfr[2 +              "            ] is translate in the transform
** that maps slice zi from volume A to volume B.
*/
int
_tenEpiRegPairXforms(Nrrd *npxfr, Nrrd **nmom, int ninLen) {
  char me[]="_tenEpiRegPairXforms", err[AIR_STRLEN_MED];
  double *xfr, *A, *B, hh, ss, tt;
  int ai, bi, zi, sz;
  
  sz = nmom[0]->axis[1].size;
  if (nrrdMaybeAlloc(npxfr, nrrdTypeDouble, 4,
		     5, sz, ninLen, ninLen)) {
    sprintf(err, "%s: couldn't allocate transform nrrd", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  nrrdAxesSet(npxfr, nrrdAxesInfoLabel, "mx,my,h,s,t", "zi", "orig", "txfd");
  xfr = (double *)(npxfr->data);
  for (bi=0; bi<ninLen; bi++) {
    for (ai=0; ai<ninLen; ai++) {
      for (zi=0; zi<sz; zi++) {
	A = (double*)(nmom[ai]->data) + 5*zi;
	B = (double*)(nmom[bi]->data) + 5*zi;
	ss = sqrt((A[M_20]*B[M_02] - B[M_11]*B[M_11]) /
		  (A[M_20]*A[M_02] - A[M_11]*A[M_11]));
	hh = (B[M_11] - ss*A[M_11])/A[M_20];
	tt = B[MEAN_Y] - A[MEAN_Y];
	ELL_5V_SET(xfr + 5*(zi + sz*(ai + ninLen*bi)),
		   A[MEAN_X], A[MEAN_Y], hh, ss, tt);
      }
    }
  }
  return 0;
}

#define SHEAR  2
#define MAG    3
#define TRAN   4

int
_tenEpiRegSMT(Nrrd *nsmt, Nrrd *npxfr, int ninLen, Nrrd *ngrad) {
  char me[]="_tenEpiRegSMT", err[AIR_STRLEN_MED];
  double *smt, *grad, *mat, *vec, *ans, *pxfr, MA, MB,
    *gA, *gB;
  int z, sz, A, B, npairs, ri;
  Nrrd *nmat, *nvec, *ninv, *nans;
  airArray *mop;

  mop = airMopNew();
  airMopAdd(mop, nmat=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ninv=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nvec=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nans=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  npairs = (ninLen-1)*(ninLen-1) - (ninLen-1);
  sz = npxfr->axis[1].size;
  if (nrrdMaybeAlloc(nsmt, nrrdTypeDouble, 2, 9, sz)
      || nrrdMaybeAlloc(nmat, nrrdTypeDouble, 2, 3, npairs)
      || nrrdMaybeAlloc(nvec, nrrdTypeDouble, 2, 1, npairs)) {
    sprintf(err, "%s: couldn't allocate SMT nrrd", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  nrrdAxesSet(nsmt, nrrdAxesInfoLabel, "Sx,Sy,Sz,Mx,My,Mz,Tx,Ty,Tz", "z");
  grad = (double *)(ngrad->data);
  mat = (double *)(nmat->data);
  vec = (double *)(nvec->data);

  /* ------ find Mx, My, Mz per slice */
  for (z=0; z<sz; z++) {
    smt = (double *)(nsmt->data) + 0 + 9*z;
    ri = 0;
    for (A=1; A<ninLen; A++) {
      for (B=1; B<ninLen; B++) {
	if (A == B) continue;
	pxfr = (double *)(npxfr->data) + 0 + 5*(z + sz*(A + ninLen*B));
	gA = grad + 0 + 3*(A-1);
	gB = grad + 0 + 3*(B-1);
	ELL_3V_SET(mat + 3*ri,
		   pxfr[MAG]*gA[0] - gB[0],
		   pxfr[MAG]*gA[1] - gB[1],
		   pxfr[MAG]*gA[2] - gB[2]);
	vec[ri] = 1 - pxfr[MAG];
	ri += 1;
      }
    }
    ellNmPseudoInverse(ninv, nmat);
    ellNmMultiply(nans, ninv, nvec);
    ans = (double *)(nans->data);
    smt[3] = ans[0];
    smt[4] = ans[1];
    smt[5] = ans[2];
  }

  /* ------ find Sx, Sy, Sz per slice */
  for (z=0; z<sz; z++) {
    smt = (double *)(nsmt->data) + 0 + 9*z;
    ri = 0;
    for (A=1; A<ninLen; A++) {
      for (B=1; B<ninLen; B++) {
	if (A == B) continue;
	pxfr = (double *)(npxfr->data) + 0 + 5*(z + sz*(A + ninLen*B));
	gA = grad + 0 + 3*(A-1);
	gB = grad + 0 + 3*(B-1);
	MA = 1 + ELL_3V_DOT(gA, smt + 3);
	MB = 1 + ELL_3V_DOT(gB, smt + 3);
	ELL_3V_SET(mat + 3*ri,
		   gB[0] - MB*gA[0]/MA,
		   gB[1] - MB*gA[1]/MA,
		   gB[2] - MB*gA[2]/MA);
	vec[ri] = pxfr[SHEAR];
	ri += 1;
      }
    }
    ellNmPseudoInverse(ninv, nmat);
    ellNmMultiply(nans, ninv, nvec);
    ans = (double *)(nans->data);
    smt[0] = ans[0];
    smt[1] = ans[1];
    smt[2] = ans[2];
  }

  /* ------ find Tx, Ty, Tz per slice */
  for (z=0; z<sz; z++) {
    smt = (double *)(nsmt->data) + 0 + 9*z;
    ri = 0;
    for (A=1; A<ninLen; A++) {
      for (B=1; B<ninLen; B++) {
	if (A == B) continue;
	pxfr = (double *)(npxfr->data) + 0 + 5*(z + sz*(A + ninLen*B));
	gA = grad + 0 + 3*(A-1);
	gB = grad + 0 + 3*(B-1);
	MA = 1 + ELL_3V_DOT(gA, smt + 3);
	MB = 1 + ELL_3V_DOT(gB, smt + 3);
#if 0
	ELL_3V_SET(mat + 3*ri,
		   gB[0] - MB*gA[0]/MA,
		   gB[1] - MB*gA[1]/MA,
		   gB[2] - MB*gA[2]/MA);
#else
	ELL_3V_SET(mat + 3*ri,
		   MB*(gB[0] - MB*gA[0]),
		   MB*(gB[1] - MB*gA[1]),
		   MB*(gB[2] - MB*gA[2]));

#endif
	vec[ri] = pxfr[TRAN];
	ri += 1;
      }
    }
    ellNmPseudoInverse(ninv, nmat);
    ellNmMultiply(nans, ninv, nvec);
    ans = (double *)(nans->data);
    smt[6] = ans[0];
    smt[7] = ans[1];
    smt[8] = ans[2];
  }

  airMopOkay(mop);
  return 0;
}

int
_tenEpiRegHST(double *hhP, double *ssP, double *ttP,
	      int ref, int ni, int zi,
	      Nrrd *npxfr, Nrrd *nsmt, Nrrd *ngrad) {
  double *xfr, *smt, *grad, zero[3]={0,0,0};
  int sz, ninLen;

  /* these could also have been passed to us, but we can also discover them */
  sz = npxfr->axis[1].size;
  ninLen = npxfr->axis[2].size;

  if (ref) {
    /* we register against a specific DWI */
    xfr = (double*)(npxfr->data) + 0 + 5*(zi + sz*(ref + ninLen*ni));
    *hhP = xfr[2];
    *ssP = xfr[3];
    *ttP = xfr[4];
  } else {
    /* we use the estimated S,M,T vectors to determine distortion
       as a function of gradient direction, and then invert this */
    smt = (double*)(nsmt->data) + 0 + 9*zi;
    grad = (ni
	    ? (double*)(ngrad->data) + 0 + 3*(ni-1)
	    : zero);
    *hhP = ELL_3V_DOT(grad, smt + 0*3);
    *ssP = 1 + ELL_3V_DOT(grad, smt + 1*3);
    *ttP = ELL_3V_DOT(grad, smt + 2*3);
  }
  return 0;
}

int
_tenEpiRegDoit1(Nrrd **ndone, Nrrd *npxfr, Nrrd *nsmt, Nrrd *ngrad,
		Nrrd **nin, int ninLen,
		int ref, NrrdKernel *kern, double *kparm,
		int verb) {
  char me[]="_tenEpiRegDoit1", err[AIR_STRLEN_MED];
  gageContext *gtx;
  gagePerVolume *pvl=NULL;
  airArray *mop;
  int E, ni, xi, yi, zi, sx, sy, sz;
  gage_t *val;
  double cx, cy, hh, ss, tt;

  mop = airMopNew();
  gtx = gageContextNew();
  airMopAdd(mop, gtx, (airMopper)gageContextNix, airMopAlways);

  /* gageSet(gtx, gageParmRenormalize, AIR_TRUE); */
  gageSet(gtx, gageParmCheckIntegrals, AIR_TRUE);
  gageSet(gtx, gageParmRequireAllSpacings, AIR_FALSE);
  gageSet(gtx, gageParmRequireEqualCenters, AIR_FALSE);
  gageSet(gtx, gageParmDefaultCenter, nrrdCenterCell);

  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  sx = nin[0]->axis[0].size;
  sy = nin[0]->axis[1].size;
  sz = nin[0]->axis[2].size;
  cx = sx/2.0;
  cy = sy/2.0;
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdCopy(ndone[ni], nin[ni])) {
      sprintf(err, "%s: couldn't do initial copy of nin[%d]", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    if (!ni) {
      /* we've just copied the B=0 image, and that's all we're supposed 
	 to do, so we're done.  All successive dwi volumes will have to
	 deal with gage */
      continue;
    }
    E = 0;
    if (pvl) {
      if (!E) E |= gagePerVolumeDetach(gtx, pvl);
      if (!E) gagePerVolumeNix(pvl);
    }
    if (!E) E |= !(pvl = gagePerVolumeNew(gtx, nin[ni], gageKindScl));
    if (!E) E |= gageQuerySet(gtx, pvl, 1 << gageSclValue);
    /* this next one is needlessly repeated */
    if (!E) E |= gageKernelSet(gtx, gageKernel00, kern, kparm);
    if (!E) E |= gagePerVolumeAttach(gtx, pvl);
    if (!E) E |= gageUpdate(gtx);
    if (E) {
      sprintf(err, "%s: trouble with gage on nin[%d]", me, ni);
      biffMove(TEN, err, GAGE); airMopError(mop); return 1;
    }
    val = gageAnswerPointer(gtx, pvl, gageSclValue);
    if (gageProbe(gtx, 0, 0, 0)) {
      sprintf(err, "%s: gage failed initial probe: %s", me, gageErrStr);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    for (zi=0; zi<sz; zi++) {
      _tenEpiRegHST(&hh, &ss, &tt, ref, ni, zi, npxfr, nsmt, ngrad);
      tt += (1-ss)*cy - hh*cx;
      for (yi=0; yi<sy; yi++) {
	for (xi=0; xi<sx; xi++) {
	  gageProbe(gtx, xi, hh*xi + ss*yi + tt, zi);
	  nrrdDInsert[ndone[ni]->type](ndone[ni]->data, xi + sx*(yi + sy*zi),
				       nrrdDClamp[ndone[ni]->type](ss*(*val)));
	}
      }
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  airMopOkay(mop);
  return 0;
}

/*
** _tenEpiRegSliceWarp
**
** Apply [hh,ss,tt] transform to nin, putting results in nout, but with
** some trickiness:
** - nwght and nidx are already allocated to the the weights (type float)
**   and indices for resampling nin with "kern" and "kparm"
** - nout is already allocated to the correct size and type
** - nin is type float, but output must be type nout->type
** - nin is been transposed to have the resampled axis fastest in memory,
**   but nout output will not be transposed
*/
int
_tenEpiRegSliceWarp(Nrrd *nout, Nrrd *nin, Nrrd *nwght, Nrrd *nidx, 
		    NrrdKernel *kern, double *kparm,
		    double hh, double ss, double tt, double cx, double cy) {
  float *wght, *in, pp, pf, tmp;
  int *idx, supp, sx, sy, xi, yi, pb, pi;
  double (*ins)(void *, size_t, double);
  
  sy = nin->axis[0].size;
  sx = nin->axis[1].size;
  supp = kern->support(kparm);
  ins = nrrdDInsert[nout->type];

  in = (float*)(nin->data);
  for (xi=0; xi<sx; xi++) {
    idx = (int*)(nidx->data);
    wght = (float*)(nwght->data);
    for (yi=0; yi<sy; yi++) {
      pp = hh*(xi - cx) + ss*(yi - cy) + tt + cy;
      pb = floor(pp);
      pf = pp - pb;
      for (pi=-(supp-1); pi<=supp; pi++) {
	idx[pi+(supp-1)] = AIR_CLAMP(0, pb + pi, sy-1);
	wght[pi+(supp-1)] = pi - pf;
      }
      idx += 2*supp;
      wght += 2*supp;
    }
    idx = (int*)(nidx->data);
    wght = (float*)(nwght->data);
    kern->evalN_f(wght, wght, 2*supp*sy, kparm);
    for (yi=0; yi<sy; yi++) {
      tmp = 0;
      for (pi=0; pi<2*supp; pi++) {
	tmp += in[idx[pi]]*wght[pi];
      }
      ins(nout->data, xi + sx*yi, ss*tmp);
      idx += 2*supp;
      wght += 2*supp;
    }
    in += sy;
  }

  return 0;
}

/*
** _tenEpiRegDoit2()
**
** an optimized version of _tenEpiRegDoit1(), which is MUCH faster because
** it doesn't use gage, which is appropriate, since the resampling is really
** only happening along one dimension.
*/
int
_tenEpiRegDoit2(Nrrd **ndone, Nrrd *npxfr, Nrrd *nsmt, Nrrd *ngrad,
		Nrrd **nin, int ninLen,
		int ref, NrrdKernel *kern, double *kparm,
		int verb) {
  char me[]="_tenEpiRegDoit2", err[AIR_STRLEN_MED];
  Nrrd *ntmp, *nfin, *nslcA, *nslcB, *nwght, *nidx;
  airArray *mop;
  int sx, sy, sz, ni, zi, supp;
  double hh, ss, tt, cx, cy;

  mop = airMopNew();
  airMopAdd(mop, ntmp=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nfin=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nslcA=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nslcB=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nwght=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nidx=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  if (verb) {
    fprintf(stderr, "%s:\n            ", me); fflush(stderr);
  }
  sx = nin[0]->axis[0].size;
  sy = nin[0]->axis[1].size;
  sz = nin[0]->axis[2].size;
  cx = sx/2.0;
  cy = sy/2.0;
  supp = kern->support(kparm);
  if (nrrdMaybeAlloc(nwght, nrrdTypeFloat, 2, 2*supp, sy)
      || nrrdMaybeAlloc(nidx, nrrdTypeInt, 2, 2*supp, sy)) {
    sprintf(err, "%s: trouble allocating buffers", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  for (ni=0; ni<ninLen; ni++) {
    if (verb) {
      fprintf(stderr, "% 2d ", ni); fflush(stderr);
    }
    if (nrrdCopy(ndone[ni], nin[ni])
	|| (!ni && nrrdSlice(nslcB, ndone[ni], 2, 0))  /* slice when ni==0 */
	|| nrrdAxesSwap(ntmp, nin[ni], 0, 1)
	|| nrrdConvert(nfin, ntmp, nrrdTypeFloat)) {
      sprintf(err, "%s: trouble prepping at ni=%d", me, ni);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    for (zi=0; zi<sz; zi++) {
      if (_tenEpiRegHST(&hh, &ss, &tt, ref, ni, zi, npxfr, nsmt, ngrad)
	  || nrrdSlice(nslcA, nfin, 2, zi)
	  || _tenEpiRegSliceWarp(nslcB, nslcA, nwght, nidx, kern, kparm,
				 hh, ss, tt, cx, cy)
	  || nrrdSplice(ndone[ni], ndone[ni], nslcB, 2, zi)) {
	sprintf(err, "%s: trouble on slice %d if ni=%d", me, zi, ni);
	/* because the _tenEpiReg calls above don't use biff */
	biffMove(TEN, err, NRRD); airMopError(mop); return 1;
      }
    }
  }
  if (verb) {
    fprintf(stderr, "done\n");
  }

  airMopOkay(mop);
  return 0;
}

int
tenEpiRegister(Nrrd *nout, Nrrd **nin, int ninLen, Nrrd *_ngrad,
	       int reference,
	       float bwX, float bwY,
	       float B0thr, float DWthr, int darkSize, int brightSize,
	       NrrdKernel *kern, double *kparm,
	       int progress, int verbose) {
  char me[]="tenEpiRegister", err[AIR_STRLEN_MED];
  airArray *mop;
  Nrrd **nbuffA, **nbuffB, *npxfr, *nprog, *nsmt, *ngrad;
  int i;

  mop = airMopNew();
  if (_tenEpiRegCheck(nout, nin, ninLen, _ngrad, reference,
		      bwX, bwY, B0thr, DWthr,
		      kern, kparm)) {
    sprintf(err, "%s: trouble with input", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  nbuffA = (Nrrd **)calloc(ninLen, sizeof(Nrrd*));
  nbuffB = (Nrrd **)calloc(ninLen, sizeof(Nrrd*));
  if (!( nbuffA && nbuffB )) {
    sprintf(err, "%s: couldn't allocate tmp nrrd pointer arrays", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, nbuffA, airFree, airMopAlways);
  airMopAdd(mop, nbuffB, airFree, airMopAlways);
  for (i=0; i<ninLen; i++) {
    airMopAdd(mop, nbuffA[i] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
    airMopAdd(mop, nbuffB[i] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  }
  airMopAdd(mop, npxfr = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nprog = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nsmt = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ngrad = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (nrrdConvert(ngrad, _ngrad, nrrdTypeDouble)) {
    sprintf(err, "%s: trouble converting gradients to doubles", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }

  /* ------ blur */
  if (_tenEpiRegBlur(nbuffA, nin, ninLen, bwX, bwY, verbose)) {
    sprintf(err, "%s: trouble %s", me, (bwX || bwY) ? "blurring" : "copying");
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdJoin(nprog, nbuffA, ninLen, 0, AIR_TRUE)
	|| nrrdSave("blur.nrrd", nprog, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: blurred", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved blur.nrrd\n", me);
  }

  /* ------ threshold */
  if (_tenEpiRegThreshold(nbuffB, nbuffA, ninLen, 
			  B0thr, DWthr, verbose)) {
    sprintf(err, "%s: trouble thresholding", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdJoin(nprog, nbuffB, ninLen, 0, AIR_TRUE)
	|| nrrdSave("thresh.nrrd", nprog, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: tresholded", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved thresh.nrrd\n", me);
  }

  /* ------ connected components */
  if (darkSize || brightSize) {
    if (_tenEpiRegCC(nbuffB, ninLen,
		     2, darkSize, brightSize, verbose)) {
      sprintf(err, "%s: trouble doing connected components", me);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    if (progress) {
      if (nrrdJoin(nprog, nbuffB, ninLen, 0, AIR_TRUE)
	  || nrrdSave("ccs.nrrd", nprog, NULL)) {
	sprintf(err, "%s: trouble saving intermediate: "
		"connected components ", me);
	biffMove(TEN, err, NRRD); airMopError(mop); return 1;
      }
      fprintf(stderr, "%s: saved ccs.nrrd\n", me);
    }
  }

  /* ------ moments */
  if (_tenEpiRegMoments(nbuffA, nbuffB, ninLen, verbose)) {
    sprintf(err, "%s: trouble finding moments", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdJoin(nprog, nbuffA, ninLen, 0, AIR_TRUE)
	|| nrrdSave("mom.nrrd", nprog, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: moments", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved mom.nrrd\n", me);
  }

  /* ------ transforms */
  if (_tenEpiRegPairXforms(npxfr, nbuffA, ninLen)) {
    sprintf(err, "%s: trouble calculating transforms", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdSave("pxfr.nrrd", npxfr, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: pair-wise xforms", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved pxfr.nrrd\n", me);
  }

  /* ------ SMT estimation */
  if (_tenEpiRegSMT(nsmt, npxfr, ninLen, ngrad)) {
    sprintf(err, "%s: trouble estimating SMT", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  if (progress) {
    if (nrrdSave("smt.nrrd", nsmt, NULL)) {
      sprintf(err, "%s: trouble saving intermediate: SMT estimates", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    fprintf(stderr, "%s: saved smt.nrrd\n", me);
  }

  /* ------ doit */
  /* filter/regularize transforms? */
  if (_tenEpiRegDoit2(nbuffB, npxfr, nsmt, ngrad, nin, ninLen,
		      reference, kern, kparm, verbose)) {
    sprintf(err, "%s: trouble performing final registration", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  
  if (verbose) {
    fprintf(stderr, "%s: creating final output ... ", me); fflush(stderr);
  }
  if (nrrdJoin(nout, nbuffB, ninLen, 0, AIR_TRUE)) {
    sprintf(err, "%s: trouble creating final output", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  if (verbose) {
    fprintf(stderr, "done\n");
  }
  
  airMopOkay(mop);
  return 0;
}
