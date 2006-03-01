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
  seekNormalsFindSet(_sctx, AIR_TRUE);
}

Contour::~Contour() {

  _sctx = seekContextNix(_sctx);
}

void
Contour::volumeSet(const Nrrd *nvol) {
  char me[]="Contour::volumeSet", *err;
  const char *key;
  gageShape *shape;
  airArray *mop;
  int ftype;

  /*
  ftype = seekTypeRidgeSurface;
  gageKind *kind = gageKindScl;
  int valItem = gageSclValue;
  int normItem = gageSclNormal;
  int gradItem = gageSclGradVec;
  int evalItem = gageSclHessEval;
  int evecItem = gageSclHessEvec;
  */

  gageKind *kind = tenGageKind;
  ftype = seekTypeIsocontour;
  int valItem = tenGageFA;
  int normItem = tenGageFANormal;
  int gradItem = tenGageFAGradVec;
  int evecItem = tenGageFAHessianEvec;
  int evalItem = tenGageFAHessianEval;

  /* 
  gageKind *kind = tenGageKind;

  ftype = seekTypeRidgeSurface;
  int valItem = tenGageFA;
  int normItem = tenGageFANormal;
  int gradItem = tenGageFAGradVec;
  int evalItem = tenGageFAHessianEval;
  int evecItem = tenGageFAHessianEvec;

  ftype = seekTypeValleySurface;
  int valItem = tenGageMode;
  int normItem = tenGageModeNormal;
  int gradItem = tenGageModeGradVec;
  int evalItem = tenGageModeHessianEval;
  int evecItem = tenGageModeHessianEvec;
  */

  /*
  ** holy crap, it actually works 
  */
  gageContext *gctx;
  gagePerVolume *gpvl;
  double kparm[3];
  gctx = gageContextNew();
  int E = 0;
  if (!E) E |= !(gpvl = gagePerVolumeNew(gctx, nvol, kind));
  if (!E) E |= gagePerVolumeAttach(gctx, gpvl);
  ELL_3V_SET(kparm, 1, 1.0, 0.0);
  if (!E) E |= gageKernelSet(gctx, gageKernel00, nrrdKernelBCCubic, kparm);
  ELL_3V_SET(kparm, 1, 1.0, 0.0);
  if (!E) E |= gageKernelSet(gctx, gageKernel11, nrrdKernelBCCubicD, kparm);
  ELL_3V_SET(kparm, 1, 1.0, 0.0);
  if (!E) E |= gageKernelSet(gctx, gageKernel22, nrrdKernelBCCubicDD, kparm);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, valItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, normItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, gradItem);
  if (!E) E |= gageQueryItemOn(gctx, gpvl, evecItem);
  if (!E) E |= gageUpdate(gctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(GAGE));
  }
  /* */
  
  mop = airMopNew();
  shape = gageShapeNew();
  airMopAdd(mop, shape, (airMopper)gageShapeNix, airMopAlways);
  if (!(key = GAGE)
      || gageShapeSet(shape, nvol, kind->baseDim)
      || !(key = LIMN)
      /* */
      || seekDataSet(_sctx, nvol, gctx, 0)
      || seekItemValueSet(_sctx, valItem)
      || seekItemNormalSet(_sctx, normItem)
      /* */
      /*
      || seekDataSet(_sctx, nvol, NULL, 0)
      || seekTypeSet(_sctx, seekTypeIsocontour)
      */
      /* */
      || seekTypeSet(_sctx, ftype)
      || seekItemGradientSet(_sctx, gradItem)
      || seekItemEigensystemSet(_sctx, evalItem, evecItem)
      /* */
      || seekUpdate(_sctx)
      ) {
    airMopAdd(mop, err=biffGetDone(key), airFree, airMopAlways);
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
    err = biffGetDone(LIMN);
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
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble getting isosurface:\n%s", me, err);
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
