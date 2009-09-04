/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "gage.h"
#include "privateGage.h"

/* these functions don't necessarily belong in gage, but we're putting
   them here for the time being.  Being in gage means that vprobe and
   pprobe don't need extra libraries to find them. */

#define BT 2.526917043979558
#define AA 0.629078014852877

double
gageTauOfTee(double tee) {
  double tau;

  tau = (tee < BT ? AA*sqrt(tee) : 0.5365 + log(tee)/2);
  return tau;
}

double
gageTeeOfTau(double tau) {
  double tee;

  /* is it surprising that the transition is at tau = 1 ? */
  tee = (tau < 1 ? tau*tau/(AA*AA) : exp(2*(tau - 0.5365)));
  return tee;
}

#undef BT
#undef AA

double
gageSigOfTau(double tau) {
  
  return sqrt(gageTeeOfTau(tau));
}

double
gageTauOfSig(double sig) {

  return gageTauOfTee(sig*sig);
}

/*
** little helper function to do pre-blurring of a given nrrd 
** of the sort that might be useful for scale-space gage use
**
** OR, as a sneaky hack: if checkPreblurredOutput is non-zero, this
** checks to see if the given array of nrrds (nblur[]) looks like it
** plausibly came from the output of this function before, with the
** same parameter settings.  This hack precludes const correctness,
** sorry!
**
** nblur has to already be allocated for "blnum" Nrrd*s, AND, they all
** have to point to valid (possibly empty) Nrrds, so they can hold the
** results of blurring. "scale" is filled with the result of
** scaleCB(d_i), for "dom" evenly-spaced samples d_i between
** scldomMin and scldomMax
*/
int
gageStackBlur(Nrrd *const nblur[], const double *scale,
              unsigned int blnum, int checkPreblurredOutput,
              const Nrrd *nin, const gageKind *kind,
              const NrrdKernelSpec *_kspec,
              int boundary, int renormalize, int verbose) {
  static const char me[]="gageStackBlur";
  char key[3][AIR_STRLEN_LARGE] = {"gageStackBlur", "scale", "kernel"};
  char val[3][AIR_STRLEN_LARGE] = {"true", "" /* below */, "" /* below */};
  unsigned int blidx, axi;
  size_t sizeIn[NRRD_DIM_MAX];
  gageShape *shapeOld, *shapeNew;
  NrrdResampleContext *rsmc;
  NrrdKernelSpec *kspec;
  airArray *mop;
  int E;

  if (!(nblur && scale && nin && _kspec)) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( blnum >= 2)) {
    biffAddf(GAGE, "%s: need blnum >= 2, not %u", me, blnum);
    return 1;
  }
  for (blidx=0; blidx<blnum; blidx++) {
    if (!AIR_EXISTS(scale[blidx])) {
      biffAddf(GAGE, "%s: scale[%u] = %g doesn't exist", me, blidx,
               scale[blidx]);
      return 1;
    }
    if (blidx) {
      if (!( scale[blidx-1] < scale[blidx] )) {
        biffAddf(GAGE, "%s: scale[%u] = %g not < scale[%u] = %g", me,
                 blidx, scale[blidx-1], blidx+1, scale[blidx]);
        return 1;
      }
    }
    /* see if all nblur[] are plausibly set to some Nrrd */
    if (!nblur[blidx]) {
      biffAddf(GAGE, "%s: got NULL nblur[%u]", me, blidx);
      return 1;
    }
  }
  if (3 + kind->baseDim != nin->dim) {
    biffAddf(GAGE, "%s: need nin->dim %u (not %u) with baseDim %u", me,
             3 + kind->baseDim, nin->dim, kind->baseDim);
    return 1;
  }
  if (airEnumValCheck(nrrdBoundary, boundary)) {
    biffAddf(GAGE, "%s: %d not a valid %s value", me,
             boundary, nrrdBoundary->name);
    return 1;
  }
  /* strictly speaking, only used with checkPreblurredOutput */
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, sizeIn);
  
  mop = airMopNew();
  kspec = nrrdKernelSpecCopy(_kspec);
  if (!kspec) {
    biffAddf(GAGE, "%s: problem copying kernel spec", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, kspec, (airMopper)nrrdKernelSpecNix, airMopAlways);
  rsmc = nrrdResampleContextNew();
  airMopAdd(mop, rsmc, (airMopper)nrrdResampleContextNix, airMopAlways);
  
  E = 0;
  if (!checkPreblurredOutput) {
    if (!E) E |= nrrdResampleDefaultCenterSet(rsmc, nrrdDefaultCenter);
    if (!E) E |= nrrdResampleNrrdSet(rsmc, nin);
    if (kind->baseDim) {
      unsigned int bai;
      for (bai=0; bai<kind->baseDim; bai++) {
        if (!E) E |= nrrdResampleKernelSet(rsmc, bai, NULL, NULL);
      }
    }
    for (axi=0; axi<3; axi++) {
      if (!E) E |= nrrdResampleSamplesSet(rsmc, kind->baseDim + axi,
                                          nin->axis[kind->baseDim + axi].size);
      if (!E) E |= nrrdResampleRangeFullSet(rsmc, kind->baseDim + axi);
    }
    if (!E) E |= nrrdResampleBoundarySet(rsmc, boundary);
    if (!E) E |= nrrdResampleTypeOutSet(rsmc, nrrdTypeDefault);
    if (!E) E |= nrrdResampleRenormalizeSet(rsmc, renormalize);
    if (E) {
      biffAddf(GAGE, "%s: trouble setting up resampling", me);
      airMopError(mop); return 1;
    }
    shapeOld = shapeNew = NULL;
  } else {
    shapeNew = gageShapeNew();
    airMopAdd(mop, shapeNew, (airMopper)gageShapeNix, airMopAlways);
    if (gageShapeSet(shapeNew, nin, kind->baseDim)) {
      biffAddf(GAGE, "%s: trouble setting up reference shape", me);
      airMopError(mop); return 1;
    }
    shapeOld = gageShapeNew();
    airMopAdd(mop, shapeOld, (airMopper)gageShapeNix, airMopAlways);
  }
  for (blidx=0; blidx<blnum; blidx++) {
    unsigned int kvpIdx;
    kspec->parm[0] = scale[blidx];
    if (!checkPreblurredOutput) {
      for (axi=0; axi<3; axi++) {
        if (!E) E |= nrrdResampleKernelSet(rsmc, kind->baseDim + axi,
                                           kspec->kernel, kspec->parm);
      }
      if (verbose) {
        printf("%s: resampling %u of %u (scale %g) ... ", me, blidx,
               blnum, scale[blidx]);
        fflush(stdout);
      }
      if (!E) E |= nrrdResampleExecute(rsmc, nblur[blidx]);
      if (E) {
        if (verbose) {
          printf("problem!\n");
        }
        biffAddf(GAGE, "%s: trouble resampling %u of %u (scale %g)",
                 me, blidx, blnum, scale[blidx]);
        airMopError(mop); return 1;
      }
    } else {
      /* check to see if nblur[blidx] is as expected */
      if (nrrdCheck(nblur[blidx])) {
        biffMovef(GAGE, NRRD, "%s: basic problem with nblur[%u]", me, blidx);
        airMopError(mop); return 1;
      }
      if (nblur[blidx]->dim != nin->dim) {
        biffAddf(GAGE, "%s: nblur[%u]->dim %u != nin->dim %u", me,
                 blidx, nblur[blidx]->dim, nin->dim);
        airMopError(mop); return 1;
      }
      if (gageShapeSet(shapeOld, nblur[blidx], kind->baseDim)) {
        /* oops- this isn't a report of how the shape is different;
           its an actual error; hopefully other things would go 
           wrong at this point ... */
        biffAddf(GAGE, "%s: trouble setting up shape %u", me, blidx);
        airMopError(mop); return 1;
      }
      if (!gageShapeEqual(shapeOld, "nblur", shapeNew, "nin")) {
        biffAddf(GAGE, "%s: nblur[%u] shape != nin shape", me, blidx);
        airMopError(mop); return 1;
      }
      /*
      nrrdAxisInfoGet_nva(nblur[blidx], nrrdAxisInfoSize, sizeOut);
      for (axi=0; axi<nin->dim; axi++) {
        if (sizeIn[axi] != sizeOut[axi]) {
          biffAddf(GAGE, "%s: nblur[%u]->axis[%u].size " _AIR_SIZE_T_CNV
                   " != nin->axis[%u].size " _AIR_SIZE_T_CNV, me,
                   blidx, axi, sizeOut[axi], axi, sizeIn[axi]);
          airMopError(mop); return 1;
        }
      }
      */
    }
    /* fill out the 2nd and 3rd values for the KVPs */
    sprintf(val[1], "%g", scale[blidx]);
    nrrdKernelSpecSprint(val[2], kspec);
    E = 0;
    if (!checkPreblurredOutput) {
      /* add the KVPs to document how these were blurred */
      for (kvpIdx=0; kvpIdx<3; kvpIdx++) {
        if (!E) E |= nrrdKeyValueAdd(nblur[blidx], key[kvpIdx], val[kvpIdx]);
      }
      if (E) {
        if (!checkPreblurredOutput && verbose) {
          printf("problem!\n");
        }
        biffAddf(GAGE, "%s: trouble adding KVP to %u of %u (scale %g)",
                 me, blidx, blnum, scale[blidx]);
        airMopError(mop); return 1;
      }
      if (verbose) {
        printf("done.\n");
      }
    } else {
      /* see if the KVPs are already there */
      for (kvpIdx=0; kvpIdx<3; kvpIdx++) {
        char *tmpval;
        tmpval = nrrdKeyValueGet(nblur[blidx], key[kvpIdx]);
        if (!tmpval) {
          biffAddf(GAGE, "%s: didn't see key \"%s\" in nblur[%u]", me,
                   key[kvpIdx], blidx);
          airMopError(mop); return 1;
        }
        if (strcmp(tmpval, val[kvpIdx])) {
          biffAddf(GAGE, "%s: found key[%s] \"%s\" != wanted \"%s\"", me,
                   key[kvpIdx], tmpval, val[kvpIdx]);
          airMopError(mop); return 1;
        }
        /* else it did match, move on */
        if (!nrrdStateKeyValueReturnInternalPointers) {
          tmpval = airFree(tmpval);
        }
      }
    }
  }

  airMopOkay(mop);
  return 0;
}

/*
** this is a little messy: the given pvlStack array has to be allocated
** by the caller to hold blnum gagePerVolume pointers, BUT, the values
** of pvlStack[i] shouldn't be set to anything: as with gagePerVolumeNew(),
** gage allocates the pervolume itself.
*/
int
gageStackPerVolumeNew(gageContext *ctx,
                      gagePerVolume **pvlStack,
                      const Nrrd *const *nblur, unsigned int blnum,
                      const gageKind *kind) {
  static const char me[]="gageStackPerVolumeNew";
  unsigned int blidx;

  if (!( ctx && pvlStack && nblur && kind )) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!blnum) {
    biffAddf(GAGE, "%s: need non-zero num", me);
    return 1;
  }

  for (blidx=0; blidx<blnum; blidx++) {
    if (!( pvlStack[blidx] = gagePerVolumeNew(ctx, nblur[blidx], kind) )) {
      biffAddf(GAGE, "%s: on pvl %u of %u", me, blidx, blnum);
      return 1;
    }
  }

  return 0;
}

/*
** the "base" pvl is the LAST pvl, ctx->pvl[pvlNum-1]
*/
int
gageStackPerVolumeAttach(gageContext *ctx, gagePerVolume *pvlBase,
                         gagePerVolume **pvlStack, const double *stackPos,
                         unsigned int blnum) {
  static const char me[]="gageStackPerVolumeAttach";
  unsigned int blidx;

  if (!(ctx && pvlBase && pvlStack && stackPos)) { 
    biffAddf(GAGE, "%s: got NULL pointer %p %p %p %p", me,
             ctx, pvlBase, pvlStack, stackPos);
    return 1;
  }
  if (!( blnum >= 2 )) {
    /* this constraint is important for the logic of stack reconstruction:
       minimum number of node-centered samples is 2, and the number of
       pvls has to be at least 3 (two blurrings + one base pvl) */
    biffAddf(GAGE, "%s: need at least two samples along stack", me);
    return 1;
  }
  if (ctx->pvlNum) {
    biffAddf(GAGE, "%s: can't have pre-existing volumes (%u) "
             "prior to stack attachment", me, ctx->pvlNum);
    return 1;
  }
  for (blidx=0; blidx<blnum; blidx++) {
    if (!AIR_EXISTS(stackPos[blidx])) {
      biffAddf(GAGE, "%s: stackPos[%u] = %g doesn't exist", me, blidx, 
               stackPos[blidx]);
      return 1;
    }
    if (blidx < blnum-1) {
      if (!( stackPos[blidx] < stackPos[blidx+1] )) {
        biffAddf(GAGE, "%s: stackPos[%u] = %g not < stackPos[%u] = %g", me,
                 blidx, stackPos[blidx], blidx+1, stackPos[blidx+1]);
        return 1;
      }
    }
  }

  /* the base volume is LAST, after all the stack samples */
  for (blidx=0; blidx<blnum; blidx++) {
    if (gagePerVolumeAttach(ctx, pvlStack[blidx])) {
      biffAddf(GAGE, "%s: on pvl %u of %u", me, blidx, blnum);
      return 1;
    }
  }
  if (gagePerVolumeAttach(ctx, pvlBase)) {
    biffAddf(GAGE, "%s: on base pvl", me);
    return 1;
  }
  
  airFree(ctx->stackPos);
  airFree(ctx->stackFsl);
  airFree(ctx->stackFw);
  ctx->stackPos = calloc(blnum, sizeof(double));
  ctx->stackFsl = calloc(blnum, sizeof(double));
  ctx->stackFw = calloc(blnum, sizeof(double));
  if (!( ctx->stackPos && ctx->stackFsl && ctx->stackFw )) {
    biffAddf(GAGE, "%s: couldn't allocate stack buffers (%p %p %p)", me,
             AIR_CAST(void *, ctx->stackPos),
             AIR_CAST(void *, ctx->stackFsl),
             AIR_CAST(void *, ctx->stackFw));
    return 1;
  }
  for (blidx=0; blidx<blnum; blidx++) {
    ctx->stackPos[blidx] = stackPos[blidx];
  }

  return 0;
}

/*
** _gageStackBaseIv3Fill
**
** after the individual iv3's in the stack have been filled, this does
** the across-stack filtering to fill the iv3 of pvl[pvlNum-1] (the
** "base" pvl) 
*/
int
_gageStackBaseIv3Fill(gageContext *ctx) {
  static const char me[]="_gageStackBaseIv3Fill";
  unsigned int fd, pvlIdx, cacheIdx, cacheLen, baseIdx, valLen;

  fd = 2*ctx->radius;
  /* the "base" pvl is the LAST pvl */
  baseIdx = ctx->pvlNum - 1; 
  cacheLen = fd*fd*fd*ctx->pvl[0]->kind->valLen;
  if (ctx->verbose > 2) {
    printf("%s: cacheLen = %u\n", me, cacheLen);
  }
  if (nrrdKernelHermiteFlag == ctx->ksp[gageKernelStack]->kernel) {
    unsigned int xi, yi, zi, blurIdx, valIdx, fdd;
    double xx, *iv30, *iv31, sigma0, sigma1;
    
    fdd = fd*fd;
    /* initialize the output iv3 to all zeros, since we won't be
       usefully setting the values on the boundary (the boundary which
       is required in the rest of the stack's iv3s in order to do the
       laplacian-based spline recon), and we can't have any
       non-existant values creeping in.  We shouldn't need to do any
       kind of nrrdBoundaryBleed thing here, because the kernel
       weights really should be zero on the boundary. */
    for (cacheIdx=0; cacheIdx<cacheLen; cacheIdx++) {
      ctx->pvl[baseIdx]->iv3[cacheIdx] = 0;
    }

    /* find the interval in the pre-blurred volumes containing the
       desired scale location */
    for (pvlIdx=0; pvlIdx<ctx->pvlNum-1; pvlIdx++) {
      if (ctx->stackFw[pvlIdx]) {
        /* has to be non-zero somewhere, since _gageLocationSet()
           gives an error if there aren't non-zero stackFw[i] */
        break;
      }
    }
    /* so no way that pvlIdx == pvlNum-1 */
    if (pvlIdx == ctx->pvlNum-2) {
      /* pvlNum-2 is pvl index of last pre-blurred volume */
      /* gageStackPerVolumeAttach() enforces getting at least two 
         pre-blurred volumes --> pvlNum >= 3 --> blurIdx >= 0 */
      blurIdx = pvlIdx-1;
      xx = 1;
    } else {
      blurIdx = pvlIdx;
      /* by design, the hermite non-kernel generates the same values as
         the tent kernel (with scale forced == 1), so we can use those
         to control the interpolation */
      xx = 1 - ctx->stackFw[pvlIdx];
    }
    iv30 = ctx->pvl[blurIdx]->iv3;
    iv31 = ctx->pvl[blurIdx+1]->iv3;
    sigma0 = ctx->stackPos[blurIdx];
    sigma1 = ctx->stackPos[blurIdx+1];
    valLen = ctx->pvl[baseIdx]->kind->valLen;
    for (valIdx=0; valIdx<valLen; valIdx++) {
      unsigned iii;
      double val0, val1, drv0, drv1, lapl0, lapl1, aa, bb, cc, dd;
      for (zi=1; zi<fd-1; zi++) {
        for (yi=1; yi<fd-1; yi++) {
          for (xi=1; xi<fd-1; xi++) {
            /* note that iv3 axis ordering is x, y, z, tuple */
            iii = xi + fd*(yi + fd*(zi + fd*valIdx));
            val0 = iv30[iii];
            val1 = iv31[iii];
            lapl0 = (iv30[iii + 1]   + iv30[iii - 1] +
                     iv30[iii + fd]  + iv30[iii - fd] +
                     iv30[iii + fdd] + iv30[iii - fdd] - 6*val0);
            lapl1 = (iv31[iii + 1]   + iv31[iii - 1] +
                     iv31[iii + fd]  + iv31[iii - fd] +
                     iv31[iii + fdd] + iv31[iii - fdd] - 6*val1);
            /* the (sigma1 - sigma0) factor is needed to convert the
               derivative with respect to sigma (sigma*lapl) into the
               derivative with respect to xx */
            drv0 = sigma0*lapl0*(sigma1 - sigma0);
            drv1 = sigma1*lapl1*(sigma1 - sigma0);
            /* Hermite spline coefficients, thanks Mathematica */
            aa = drv0 + drv1 + 2*val0 - 2*val1;
            bb = -2*drv0 - drv1 - 3*val0 + 3*val1;
            cc = drv0;
            dd = val0;
            ctx->pvl[baseIdx]->iv3[iii] = dd + xx*(cc + xx*(bb + aa*xx));
          }
        }
      }
    }
  } else {
    /* we're doing simple convolution-based recon on the stack */
    /* NOTE we are treating the 4D fd*fd*fd*valLen iv3 as a big 1-D array */
    double wght, val;
    for (cacheIdx=0; cacheIdx<cacheLen; cacheIdx++) {
      val = 0;
      for (pvlIdx=0; pvlIdx<ctx->pvlNum-1; pvlIdx++) {
        wght = ctx->stackFw[pvlIdx];
        val += (wght
                ? wght*ctx->pvl[pvlIdx]->iv3[cacheIdx]
                : 0);
      }
      ctx->pvl[baseIdx]->iv3[cacheIdx] = val;
    }
  }
  return 0;
}

/*
******** gageStackProbe()
*/
int
gageStackProbe(gageContext *ctx,
               double xi, double yi, double zi, double stackIdx) {
  static const char me[]="gageStackProbe";

  if (!ctx) {
    return 1;
  }
  if (!ctx->parm.stackUse) {
    sprintf(ctx->errStr, "%s: can't probe stack without parm.stackUse", me);
    ctx->errNum = 1;
    return 1;
  }
  return _gageProbe(ctx, xi, yi, zi, stackIdx);
}

int
gageStackProbeSpace(gageContext *ctx,
                    double xx, double yy, double zz, double ss,
                    int indexSpace, int clamp) {
  static const char me[]="gageStackProbeSpace";

  if (!ctx) {
    return 1;
  }
  if (!ctx->parm.stackUse) {
    sprintf(ctx->errStr, "%s: can't probe stack without parm.stackUse", me);
    ctx->errNum = 1;
    return 1;
  }
  return _gageProbeSpace(ctx, xx, yy, zz, ss, indexSpace, clamp);
}

double
gageStackWtoI(gageContext *ctx, double swrl, int *outside) {
  double si;

  if (ctx && ctx->parm.stackUse && outside) {
    unsigned int sidx;
    if (swrl < ctx->stackPos[0]) {
      /* we'll extrapolate from stackPos[0] and [1] */
      sidx = 0;
      *outside = AIR_TRUE;
    } else if (swrl > ctx->stackPos[ctx->pvlNum-2]) {
      /* extrapolate from stackPos[ctx->pvlNum-3] and [ctx->pvlNum-2];
         gageStackPerVolumeAttach ensures that we there are at least two
         blurrings pvls & one base pvl ==> pvlNum >= 3 ==> pvlNum-3 >= 0 */
      sidx = ctx->pvlNum-3;
      *outside = AIR_TRUE;
    } else {
      /* HEY: stupid linear search */
      for (sidx=0; sidx<ctx->pvlNum-2; sidx++) {
        if (AIR_IN_CL(ctx->stackPos[sidx], swrl, ctx->stackPos[sidx+1])) {
          break;
        }
      }
      if (sidx == ctx->pvlNum-2) {
        /* search failure */
        *outside = AIR_FALSE;
        return AIR_NAN;
      }
      *outside = AIR_FALSE;
    }
    si = AIR_AFFINE(ctx->stackPos[sidx], swrl, ctx->stackPos[sidx+1],
                    sidx, sidx+1);
  } else {
    si = AIR_NAN;
  }
  return si;
}

double
gageStackItoW(gageContext *ctx, double si, int *outside) {
  unsigned int sidx;
  double swrl, sfrac;

  if (ctx && ctx->parm.stackUse && outside) {
    if (si < 0) {
      sidx = 0;
      *outside = AIR_TRUE;
    } else if (si > ctx->pvlNum-2) {
      sidx = ctx->pvlNum-3;
      *outside = AIR_TRUE;
    } else {
      sidx = AIR_CAST(unsigned int, si);
      *outside = AIR_FALSE;
    }
    sfrac = si - sidx;
    swrl = AIR_AFFINE(0, sfrac, 1, ctx->stackPos[sidx], ctx->stackPos[sidx+1]);
    /*
    printf("!%s: si %g (%u) --> %u + %g --> [%g,%g] -> %g\n", me,
           si, ctx->pvlNum, sidx, sfrac, 
           ctx->stackPos[sidx], ctx->stackPos[sidx+1], swrl);
    */
  } else {
    swrl = AIR_NAN;
  }
  return swrl;
}

/*
******** gageStackVolumeGet
**
** will try to load pre-blurred volumes, or will recompute them
** as needed
**
** with respect to biff usage, this is an unusual function, because
** it is very rare for biffGetDone() to be called inside a Teem library 
*/
int
gageStackVolumeGet(Nrrd ***ninSSP, double **scalePosP, int *recomputedP,
                   unsigned int numSS, const double rangeSS[2],
                   int uniformSS, int optimSS,
                   const char *formatSS, unsigned int numStart,
                   const Nrrd *nin, const gageKind *kind,
                   const NrrdKernelSpec *kSSblur, int verbose) {
  char me[]="gageStackVolumeGet", *suberr;
  char *fname;
  FILE *file;
  unsigned int ssi;
  airArray *mop;
  int firstExists, recompute;
  double *scalePos;
  Nrrd **ninSS;

  if (!(ninSSP && scalePosP && recomputedP)) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  mop = airMopNew();

  /* allocate ninSS and scalePos */
  *ninSSP = NULL;
  *scalePosP = NULL;
  scalePos = AIR_CAST(double *, calloc(numSS, sizeof(double)));
  airMopAdd(mop, scalePos, airFree, airMopOnError);
  ninSS = AIR_CAST(Nrrd **, calloc(numSS, sizeof(Nrrd *)));
  airMopAdd(mop, ninSS, airFree, airMopOnError);
  if (!(scalePos && ninSS)) {
    biffAddf(GAGE, "%s: couldn't alloc %u doubles,Nrrd*s", me, numSS);
    return 1;
  }
  *ninSSP = ninSS;
  *scalePosP = scalePos;

  /* set scalePos */
  if (uniformSS) {
    for (ssi=0; ssi<numSS; ssi++) {
      scalePos[ssi] = AIR_AFFINE(0, ssi, numSS-1, rangeSS[0], rangeSS[1]);
    }
  } else {
    if (optimSS
        && 0 == rangeSS[0]
        && rangeSS[1] == AIR_CAST(unsigned int, rangeSS[1])
        && numSS <= GAGE_OPTIMSIG_SAMPLES_MAXNUM
        && rangeSS[1] <= GAGE_OPTIMSIG_SIGMA_MAX) {
      if (gageOptimSigSet(scalePos, numSS, AIR_CAST(unsigned int, rangeSS[1]))) {
        biffAddf(GAGE, "%s: trouble w/ optimal sigmas", me);
        airMopError(mop); return 1;
      }
    } else {
      double tau0, tau1, tau;
      tau0 = gageTauOfSig(rangeSS[0]);
      tau1 = gageTauOfSig(rangeSS[1]);
      for (ssi=0; ssi<numSS; ssi++) {
        tau = AIR_AFFINE(0, ssi, numSS-1, tau0, tau1);
        scalePos[ssi] = gageSigOfTau(tau);
      }
    }
  }
  for (ssi=0; ssi<numSS; ssi++) {
    ninSS[ssi] = nrrdNew();
  }

  /* set recompute flag */
  if (!airStrlen(formatSS)) {
    /* no info about files to load, obviously have to recompute */
    if (verbose) {
      printf("%s: no file info, must compute blurrings\n", me);
    }
    recompute = AIR_TRUE;
  } else {
    /* do have info about files to load, but may fail in many ways */
    fname = AIR_CAST(char *, calloc(strlen(formatSS) + AIR_STRLEN_SMALL, 
                                    sizeof(char)));
    if (!fname) {
      biffAddf(GAGE, "%s: couldn't allocate fname", me);
      airMopError(mop); return 1;
    }
    airMopAdd(mop, fname, airFree, airMopAlways);
    /* see if we can get the first file */
    sprintf(fname, formatSS, 0);
    firstExists = !!(file = fopen(fname, "r"));
    airFclose(file);
    if (!firstExists) {
      if (verbose) {
        printf("%s: no file \"%s\"; will recompute blurrings\n", me, fname);
      }
      recompute = AIR_TRUE;
    } else if (nrrdLoadMulti(ninSS, numSS, formatSS, numStart, NULL)) {
      airMopAdd(mop, suberr = biffGetDone(NRRD), airFree, airMopAlways);
      if (verbose) {
        printf("%s: will recompute blurrings that couldn't be "
               "read:\n%s\n", me, suberr);
      }
      recompute = AIR_TRUE;
    } else if (gageStackBlur(ninSS, scalePos, numSS,
                             AIR_TRUE /* do just check */,
                             nin, kind, kSSblur, 
                             nrrdBoundaryBleed, AIR_TRUE /* do renorm */,
                             verbose)) {
      airMopAdd(mop, suberr = biffGetDone(GAGE), airFree, airMopAlways);
      if (verbose) {
        printf("%s: will recompute blurrings that didn't match:\n%s\n",
               me, suberr);
      }
      recompute = AIR_TRUE;
    } else {
      /* else precomputed blurrings could all be read, and did match */
      if (verbose) {
        fprintf(stderr, "%s: will re-use %s pre-blurrings.\n", me, formatSS);
      }
      recompute = AIR_FALSE;
    }
  }
  if (recompute) {
    if (gageStackBlur(ninSS, scalePos, numSS,
                      AIR_FALSE /* not check, recomputing */,
                      nin, kind, kSSblur, 
                      nrrdBoundaryBleed, AIR_TRUE /* do renorm */,
                      verbose)) {
      biffAddf(GAGE, "%s: trouble computing blurrings", me);
      airMopError(mop); return 1;
    }
    /*
    NrrdIoState *nio;
    int E;
    ... blur ...
    E = 0;
    if (nrrdEncodingGzip->available()) {
      nio = nrrdIoStateNew();
      airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
      E = nrrdIoStateEncodingSet(nio, nrrdEncodingGzip);
    } else {
      nio = NULL;
    }
    if (!E) E |= nrrdSaveMulti(formatSS, AIR_CAST(const Nrrd *const *,
                                                  vspec->ninSS),
                               vspec->numSS, 0, nio);
    if (E) {
      sprintf(err, "%s: trouble saving pre-computed blurrings", me);
      biffMove(PULL, err, NRRD); airMopError(mop); return 1;
    }
    */
  }
  airMopOkay(mop);
  *recomputedP = recompute;
  return 0;
}
