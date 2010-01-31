/*
  Deft: experiments in minimalist scientific visualization 
  Copyright (C) 2006, 2005  Gordon Kindlmann

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Deft.h"
#include "Contour.h"

namespace Deft {

Contour::Contour() {

  _sctx = seekContextNew();
  seekVerboseSet(_sctx, 1);
  seekNormalsFindSet(_sctx, AIR_TRUE);
}

Contour::~Contour() {

  _sctx = seekContextNix(_sctx);
}

void
Contour::volumeSet(const Nrrd *nvol) {
  char me[]="Contour::volumeSet", *err;
  airArray *mop;
  int ftype;

  this->wireframe(false);

  ftype = seekTypeIsocontour;
  /*
  gageKind *kind = gageKindScl;
  int sclvItem = gageSclValue;
  int normItem = gageSclNormal;
  int strengthItem = gageSclGradMag;
  int strengthSign = 1;
  double strength = 0.2;
  */

  /* ftype = seekTypeRidgeSurface; */
  /*
  this->twoSided(true);
  this->normalsUse(false);
  this->lightingUse(true);
  int normItem = gageSclHessEvec2;
  int gradItem = gageSclGradVec;
  int evalItem = gageSclHessEval;
  int evecItem = gageSclHessEvec;
  int strengthItem = gageSclHessEval2;
  int strengthSign = -1;
  double strength = 0.03;
  */

  /* gageKind *kind = tenGageKind; */
  /*
  ftype = seekTypeIsocontour;
  int sclvItem = tenGageFA;
  int normItem = tenGageFANormal;
  */
  /*
  ftype = seekTypeRidgeSurface;
  this->twoSided(true);
  int normItem = tenGageFAHessianEvec2;
  int gradItem = tenGageFAGradVec;
  int evecItem = tenGageFAHessianEvec;
  int evalItem = tenGageFAHessianEval;
  int strengthItem = tenGageFAHessianEval2;
  int strengthSign = -1;
  double strength = 17;
  */
  /* 
  gageKind *kind = tenGageKind;

  ftype = seekTypeRidgeSurface;
  int sclvItem = tenGageFA;
  int normItem = tenGageFANormal;
  int gradItem = tenGageFAGradVec;
  int evalItem = tenGageFAHessianEval;
  int evecItem = tenGageFAHessianEvec;

  ftype = seekTypeValleySurface;
  int sclvItem = tenGageMode;
  int normItem = tenGageModeNormal;
  int gradItem = tenGageModeGradVec;
  int evalItem = tenGageModeHessianEval;
  int evecItem = tenGageModeHessianEvec;
  */

  /*
  ** holy crap, it actually works 
  */
  /*
  gageContext *gctx;
  gagePerVolume *gpvl;
  double kparm[3];
  gctx = gageContextNew();
  int E = 0;
  if (!E) E |= !(gpvl = gagePerVolumeNew(gctx, nvol, kind));
  if (!E) E |= gagePerVolumeAttach(gctx, gpvl);
  ELL_3V_SET(kparm, 2, 1.0, 0.0);
  if (!E) E |= gageKernelSet(gctx, gageKernel00, nrrdKernelBCCubic, kparm);
  ELL_3V_SET(kparm, 2, 1.0, 0.0);
  if (!E) E |= gageKernelSet(gctx, gageKernel11, nrrdKernelBCCubicD, kparm);
  ELL_3V_SET(kparm, 2, 1.0, 0.0);
  if (!E) E |= gageKernelSet(gctx, gageKernel22, nrrdKernelBCCubicDD, kparm);

  if (!E) E |= gageQueryItemOn(gctx, gpvl, sclvItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, normItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, strengthItem);
  */

  /*
  if (!E) E |= gageQueryItemOn(gctx, gpvl, normItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, gradItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, evalItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, evecItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, strengthItem);
  */
  /*
  if (!E) E |= gageUpdate(gctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(GAGE));
  }
  */
  /* */

  size_t samples[3];
  ELL_3V_SET(samples, 61, 61, 31);
  
  mop = airMopNew();
  if (seekDataSet(_sctx, nvol /* NULL */, NULL /* gctx */, 0)
      /* */
      || seekTypeSet(_sctx, ftype)
      /* || seekSamplesSet(_sctx, samples) */
      /*
      || seekStrengthUseSet(_sctx, AIR_TRUE)
      || seekStrengthSet(_sctx, strengthSign, strength)

      || seekItemStrengthSet(_sctx, strengthItem)
      || seekItemScalarSet(_sctx, sclvItem)
      || seekItemNormalSet(_sctx, normItem)
      */
      /*
      || seekItemGradientSet(_sctx, gradItem)
      || seekItemEigensystemSet(_sctx, evalItem, evecItem)
      || seekItemNormalSet(_sctx, normItem)
      || seekItemStrengthSet(_sctx, strengthItem)
      */
      /* */
      || seekUpdate(_sctx)
      ) {
    airMopAdd(mop, err=biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop); return;
  }
  airMopOkay(mop);
  return;
}

void
Contour::lowerInsideSet(int val) {
  char me[]="Contour::lowerInsideSet", *err;

  if (seekLowerInsideSet(_sctx, val)) {
    err = biffGetDone(SEEK);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    free(err); return;
  }
  return;
}

void
Contour::extract(double isovalue) {
  char me[]="Contour::extract", *err;
  double time0, time1;

  time0 = airTime();
  if (seekIsovalueSet(_sctx, isovalue)
      || seekUpdate(_sctx)
      || seekExtract(_sctx, _lpldOwn)) {
    err = biffGetDone(SEEK);
    fprintf(stderr, "%s: trouble getting surface:\n%s", me, err);
    free(err); 
    return;
  }
  time1 = airTime();
  /*
  fprintf(stderr, "%s: %d tris, %g|%g sec: %g|%g Ktris/sec\n", me,
          _lpldOwn->indxNum/3, _sctx->time, time1-time0,
          (double)(_lpldOwn->indxNum/3)/(1000.0*_sctx->time),
          (double)(_lpldOwn->indxNum/3)/(1000.0*(time1 - time0)));
  */
  this->changed();

  if (limnPolyDataVertexWindingFix(_lpldOwn, AIR_TRUE)) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble fixing winding:\n%s", me, err);
    free(err); 
  }

  return;
}

double
Contour::minimum() {
  return _sctx->range->min;
}

double
Contour::maximum() {
  return _sctx->range->max;
}

} /* namespace Deft */
