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

#include "HyperStreamline.h"

namespace Deft {

/*
** thoughts on ways that fibers can be characterized:
**
** Cool: be able to draw some as polylines, some as tubes, so that those going
** through some ROI are highlighted
**
** Cool: have scroll bar that can index through entire fiber set,
** hightlighting one at a time
**
** certainly need to be able to click on individual fibers
*/

// ---------------------------------------------------------------

enum {
  flagUnknown,
  flagSeedNum,
  flagSeedPositions,
  flagFiberAllocated,
  flagFiberParm,
  flagFiberContext,
  flagFiberGeometry,
  flagColorMap,
  flagFiberColor,
  flagFiberStopColorDo,
  flagFiberStopColor,
  flagTubeFacet,
  flagFiberVertexNum,
  flagTubeAllocated,
  flagTubeRadius,
  flagProbeItem,
  flagProbeMeasr,
  flagProbeCap,
  flagTubeGeometry,
  flagTubeColor,
  flagTubeDo,
  flagResult,
  flagLast
};

#define ERROR \
  err = biffGetDone(TEN); \
  fprintf(stderr, "%s: trouble:\n%s\n", me, err); \
  free(err); exit(1)

HyperStreamline::HyperStreamline(const Volume *vol) {
  char me[]="HyperStreamline::HyperStreamline", *err;
  NrrdKernelSpec lksp;

  this->_dwi = !strcmp(TEN_DWI_GAGE_KIND_NAME, vol->kind()->name);

  fprintf(stderr, "!%s: welcome gotDwi = %s (%s)\n", me,
          this->_dwi ? "true" : "false",
          vol->kind()->name);

  _tfx = (this->_dwi
          ? tenFiberContextDwiNew(vol->nrrd(),
                                  1, 1, 1, /* thresh, soft, min */
                                  tenEstimate1MethodLLS,
                                  tenEstimate2MethodPeled)
          : tenFiberContextNew(vol->nrrd()));
  if (!_tfx) {
    ERROR;
  }
  if (tenFiberParmSet(_tfx, tenFiberParmUseIndexSpace, AIR_FALSE)) {
    ERROR;
  }
  tenFiberVerboseSet(_tfx, 0);
  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = false;
  }
  
  _fibers = tenFiberMultiNew();
  _seedNum = 0;
  _fiberVertexNum = 0;

  // TERRIBLE: playing dangerous games with ownership of _lpldOwn...
  limnPolyDataNix(_lpldOwn);
  _lpldFibers = limnPolyDataNew();
  _lpldOwn = _lpldFibers;
  _lpldTubes = limnPolyDataNew();

  // default fiber context settings
  this->color(true);
  if (this->_dwi) {
    /* this default does not depend on levmar */
    fiberType(tenDwiFiberType1Evec0);
  } else {
    fiberType(tenFiberTypeEvec0);
  }
  stopAniso(tenAniso_Cl2, 0.3);
  stopConfidence(0.5);
  step(1);
  stopHalfStepNum(30);
  stopStubDo(true);
  integration(tenFiberIntgMidpoint);
  NrrdKernelSpec *ksp = nrrdKernelSpecNew();
  nrrdKernelSpecParse(ksp, "tent");
  kernel(ksp);
  _volume = vol;
  volume(_volume);
  dynamic_cast<PolyProbe*>(this)->kernel(gageKernel00, ksp);

  // HEY: hack!
  lksp.kernel = nrrdKernelBCCubicD;
  ELL_3V_SET(lksp.parm, 1, 1, 0);
  dynamic_cast<PolyProbe*>(this)->kernel(gageKernel11, &lksp);

  _nseed = nrrdNew();
  _nTubeVertMap = nrrdNew();
  nrrdKernelSpecNix(ksp);
  dynamic_cast<PolyProbe*>(this)->evecRgbAnisoGamma(1.3);
  dynamic_cast<PolyProbe*>(this)->evecRgbIsoGray(1.0);
  dynamic_cast<PolyProbe*>(this)->evecRgbMaxSat(1.0);
  _brightness = 0.0;
  brightness(1.0);
  _tubeDo = true;
  tubeDo(false);
  _stopColorDo = false;
  stopColorDo(true);
  _tubeFacet = 0;
  _endFacet = 0;
  tubeFacet(4);
  _tubeRadius = -1;
  tubeRadius(0.5);
  
  _probeNrrd = NULL;
  _probeKind = NULL;
  _probeItem = 1;
  probeItem(0);
  _probeMeasr = nrrdMeasureUnknown;
  probeMeasr(nrrdMeasureMean);
  _probeCap = 1;
  probeCap(0);

  _fiberAllocatedTime = 0;
  _fiberGeometryTime = 0;
  _fiberColorTime = 0;
  _fiberStopColorTime = 0;
  _tubeAllocatedTime = 0;
  _tubeGeometryTime = 0;
  _tubeColorTime = 0;
}

HyperStreamline::~HyperStreamline() {

  _fibers = tenFiberMultiNix(_fibers);
  tenFiberContextNix(_tfx);
  // TERRIBLE
  _lpldOwn = _lpldFibers;
  // limnPolyDataNix(_lpldFibers);   (that's now done by PolyData's dtor)
  limnPolyDataNix(_lpldTubes);
  nrrdNuke(_nseed);
  nrrdNuke(_nTubeVertMap);
}

bool
HyperStreamline::useDwi() const {

  return _dwi;
}

void
HyperStreamline::seedsSet(const Nrrd *nseed) {
  char me[]="HyperStreamline::seedsSet";
  unsigned int seedNumNew;

  if (nseed) {
    if (!( 2 == nseed->dim && 3 == nseed->axis[0].size)) {
      fprintf(stderr, "%s: trouble: nseed %u-D %u-by-X, not 2-D 3-by-X\n",
              me, nseed->dim, AIR_CAST(unsigned int, nseed->axis[0].size));
    }
    if (nrrdConvert(_nseed, nseed, nrrdTypeDouble)) {
      fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(NRRD));
    }
    seedNumNew = _nseed->axis[1].size;
  } else {
    seedNumNew = 0;
  }
  fprintf(stderr, "!%s: seedNumNew = %u\n", me, seedNumNew);

  if (_seedNum != seedNumNew) {
    _seedNum = seedNumNew;
    _flag[flagSeedNum] = true;
  }

  _flag[flagSeedPositions] = true;
}

void
HyperStreamline::fiberType(int type) { 
  char me[]="HyperStreamline::fiberType", *err;
  if (tenFiberTypeSet(_tfx, type)) { ERROR; } 
  fprintf(stderr, "!%s: %p fiberType now %d\n", me, this, _tfx->fiberType);
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::integration(int intg) { 
  char me[]="HyperStreamline::integration", *err;
  if (tenFiberIntgSet(_tfx, intg)) { ERROR; } 
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::step(double step) { 
  char me[]="HyperStreamline::step", *err;
  if (tenFiberParmSet(_tfx, tenFiberParmStepSize, step)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::puncture(double punct) {
  char me[]="HyperStreamline::puncture", *err;
  if (tenFiberParmSet(_tfx, tenFiberParmWPunct, punct)) { ERROR; }  
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::kernel(const NrrdKernelSpec *ksp) {
  char me[]="HyperStreamline::kernel", *err;
  if (tenFiberKernelSet(_tfx, ksp->kernel, ksp->parm)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopAniso(int aniso, double thresh) {
  char me[]="HyperStreamline::stopAniso", *err;
  if (tenFiberStopSet(_tfx, tenFiberStopAniso, aniso, thresh)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopAnisoDo(bool doit) {
  bool olddo = (_tfx->stop & (1 << tenFiberStopAniso)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopAniso); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopAniso); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopHalfLength(double maxHalfLength) {
  char me[]="HyperStreamline::stopHalfLength", *err;
  if (tenFiberStopSet(_tfx, tenFiberStopLength, maxHalfLength)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopHalfLengthDo(bool doit) { 
  bool olddo = (_tfx->stop & (1 << tenFiberStopLength)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopLength); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopLength); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopHalfStepNum(unsigned int steps) {
  char me[]="HyperStreamline::stopHalfStepNum", *err;
  if (tenFiberStopSet(_tfx, tenFiberStopNumSteps, steps)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopHalfStepNumDo(bool doit) {
  bool olddo = (_tfx->stop & (1 << tenFiberStopNumSteps)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopNumSteps); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopNumSteps); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopConfidence(double conf) {
  char me[]="HyperStreamline::stopConfidence", *err;
  if (tenFiberStopSet(_tfx, tenFiberStopConfidence, conf)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopConfidenceDo(bool doit) {
  bool olddo = (_tfx->stop & (1 << tenFiberStopConfidence)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopConfidence); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopConfidence); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopFraction(double frac) {
  char me[]="HyperStreamline::stopFraction", *err;
  if (tenFiberStopSet(_tfx, tenFiberStopFraction, frac)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopFractionDo(bool doit) {
  bool olddo = (_tfx->stop & (1 << tenFiberStopFraction)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopFraction); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopFraction); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopRadius(double conf) {
  char me[]="HyperStreamline::stopRadius", *err;
  if (tenFiberStopSet(_tfx, tenFiberStopRadius, conf)) { ERROR; }
  _flag[flagFiberParm] = true;
}

void
HyperStreamline::stopRadiusDo(bool doit) {
  bool olddo = (_tfx->stop & (1 << tenFiberStopRadius)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopRadius); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopRadius); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopStubDo(bool doit) {
  bool olddo = (_tfx->stop & (1 << tenFiberStopStub)) ? true : false;
  if (olddo != doit) {
    if (doit) {
      tenFiberStopOn(_tfx, tenFiberStopStub); 
    } else {
      tenFiberStopOff(_tfx, tenFiberStopStub); 
    }
    _flag[flagFiberParm] = true;
  }
}

void
HyperStreamline::stopReset() { tenFiberStopReset(_tfx); }

void
HyperStreamline::tubeDo(bool doit) {
  if (_tubeDo != doit) {
    _tubeDo = doit;
    _flag[flagTubeDo] = true;
  }
}

void
HyperStreamline::stopColorDo(bool doit) {
  if (_stopColorDo != doit) {
    _stopColorDo = doit;
    _flag[flagFiberStopColorDo] = true;
  }
}

void
HyperStreamline::tubeFacet(unsigned int facet) {
  facet = AIR_MAX(3, facet);
  if (_tubeFacet != facet) {
    _tubeFacet = facet;
    _endFacet = _tubeFacet/4 + 1;
    _flag[flagTubeFacet] = true;
  }
}

void
HyperStreamline::tubeRadius(double radius) {
  if (_tubeRadius != radius) {
    _tubeRadius = radius;
    _flag[flagTubeRadius] = true;
  }
}

void 
HyperStreamline::probeVol(const Nrrd *nin, const gageKind *kind) {
  _probeNrrd = nin;
  _probeKind = kind;
}

void
HyperStreamline::probeItem(int item) {
  if (_probeItem != item) {
    _probeItem = item;
    _flag[flagProbeItem] = true;
  }
}

void
HyperStreamline::probeMeasr(int measr) {
  if (_probeMeasr != measr) {
    _probeMeasr = measr;
    _flag[flagProbeMeasr] = true;
  }
}

void
HyperStreamline::probeCap(unsigned int cap) {
  if (_probeCap != cap) {
    _probeCap = cap;
    _flag[flagProbeCap] = true;
  }
}

void
HyperStreamline::parmCopy(HyperStreamline *src) {
  // char me[]="HyperStreamline::parmCopy";
  
  this->fiberType(src->fiberType());

  this->integration(src->integration());
  this->step(src->step());
  this->puncture(src->puncture());
  this->kernel(src->kernel());

  // have to do it in this order, because setting the stop parameters
  // implicitly turns on that stop method
  this->stopAniso(src->stopAnisoType(), src->stopAnisoThreshold());
  this->stopAnisoDo(src->stopAnisoDo());

  this->stopHalfLength(src->stopHalfLength());
  this->stopHalfLengthDo(src->stopHalfLengthDo());

  this->stopHalfStepNum(src->stopHalfStepNum());
  this->stopHalfStepNumDo(src->stopHalfStepNumDo());

  this->stopConfidence(src->stopConfidence());
  this->stopConfidenceDo(src->stopConfidenceDo());

  if (this->useDwi()) {
    this->stopFraction(src->stopFraction());
    this->stopFractionDo(src->stopFractionDo());
  }

  this->stopRadius(src->stopRadius());
  this->stopRadiusDo(src->stopRadiusDo());

  this->stopStubDo(src->stopStubDo());

  this->colorQuantity(src->colorQuantity());
  this->brightness(src->brightness());
  this->tubeDo(src->tubeDo());
  this->stopColorDo(src->stopColorDo());
  this->tubeFacet(src->tubeFacet());
  this->tubeRadius(src->tubeRadius());

}

void
HyperStreamline::updateFiberContext() {
  char me[]="HyperStreamline::updateFiberContext", *err;

  if (_flag[flagFiberParm]) {
    if (tenFiberUpdate(_tfx)) { ERROR; }
    _flag[flagFiberParm] = false;
    _flag[flagFiberContext] = true;
  }
}

void
HyperStreamline::updateFiberAllocated() {
  // char me[]="HyperStreamline::updateFiberAllocated";

  if (_flag[flagSeedNum]) {
    double time0 = airTime();
    // NO MORE: "currently allocate more when needed, and never free"
    // that was causing problems because some code was using 
    // _fiber.size() as the number of fibers, so you'd segfault
    // if the number of fibers decreased!
    // Wed Sep 12 12:12:50 EDT 2007: now with the change to an airArray of
    // tenFiberSingle, and the possibility of having multiple fibers per 
    // seed point, we can't allocate ahead of time the fibers. 
    // But, we know that we'll need at least as many as seed points
    airArrayLenSet(_fibers->fiberArr, _seedNum);
    // HEY: error if (!_fibers->fiberArr->data)
    _flag[flagSeedNum] = false;
    _flag[flagFiberAllocated] = true;
    _fiberAllocatedTime = airTime() - time0;
  }
}

int
_fiberCompare(const void *_fa, const void *_fb) {
  tenFiberSingle *fa, *fb;
  int ret;
  double a, b;
  
  fa = AIR_CAST(tenFiberSingle *, _fa);
  fb = AIR_CAST(tenFiberSingle *, _fb);
  a = fa->measr[nrrdMeasureUnknown];
  b = fb->measr[nrrdMeasureUnknown];
  if (AIR_EXISTS(a) && AIR_EXISTS(b)) {
    ret = (a < b
           ? +1
           : (a == b
              ? 0
              : -1));
  } else {
    ret = 0;
  }
  return ret;
}

void
HyperStreamline::updateProbe() {
  char me[]="HyperStreamline::updateProbe", *err;
  gageContext *ctx;
  gagePerVolume *pvl;
  double kparm[3]={1,1,0};
  const double *answer;
  airArray *mop;
  int E;

  fprintf(stderr, "!%s: hello\n", me);
  mop = airMopNew();
  ctx = gageContextNew();
  airMopAdd(mop, ctx, AIR_CAST(airMopper, gageContextNix), airMopAlways);
  gageParmSet(ctx, gageParmVerbose, 0);

  E = 0;
  if (!E) E |= !(pvl = gagePerVolumeNew(ctx, _probeNrrd, _probeKind));
  if (!E) E |= gageKernelSet(ctx, gageKernel00, nrrdKernelBCCubic, kparm);
  if (!E) E |= gageKernelSet(ctx, gageKernel11, nrrdKernelBCCubicD, kparm);
  if (!E) E |= gageKernelSet(ctx, gageKernel22, nrrdKernelBCCubicDD, kparm);
  if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  if (!E) E |= gageQueryItemOn(ctx, pvl, _probeItem);
  if (!E) E |= gageUpdate(ctx);
  answer = gageAnswerPointer(ctx, pvl, _probeItem);
  if (E) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return;
  }

  fprintf(stderr, "!%s: fiber probing ... ", me); fflush(stderr);
  unsigned int fidx;
  for (fidx=0; fidx<_fibers->fiberArr->len; fidx++) {
    tenFiberSingle *tfs = _fibers->fiber + fidx;
    if (tenFiberStopUnknown != tfs->whyNowhere) {
      /* whyNowhere is something ==> it really did go nowhere, its a stub */
      tfs->measr[_probeMeasr] = AIR_NAN;
    } else {
      double *pos = AIR_CAST(double *, tfs->nvert->data);
      size_t len = tfs->nvert->axis[1].size;
      nrrdMaybeAlloc_va(tfs->nval, nrrdTypeDouble, 1, len);
      double *val = AIR_CAST(double *, tfs->nval->data);
      for (unsigned int vi=0; vi<len; vi++) {
        gageProbeSpace(ctx, (pos + 3*vi)[0], (pos + 3*vi)[1], (pos + 3*vi)[2],
                       AIR_FALSE, AIR_TRUE);
        val[vi] = *answer;
      }
      nrrdMeasureLine[_probeMeasr](tfs->measr + _probeMeasr, nrrdTypeDouble,
                                   val, nrrdTypeDouble, len,
                                   0, len-1);
    }
    // putting value in a standard place for the sake of qsort
    tfs->measr[nrrdMeasureUnknown] = tfs->measr[_probeMeasr];
  }
  fprintf(stderr, "sorting ... "); fflush(stderr);
  qsort(_fibers->fiber, _fibers->fiberNum,
        sizeof(tenFiberSingle), _fiberCompare);
  fprintf(stderr, "done\n");

  fprintf(stderr, "!%s: bye.\n", me);
  airMopOkay(mop);
  return;
}

void
HyperStreamline::updateFiberGeometry() {
  char me[]="HyperStreamline::updateFiberGeometry", *err;

  if (_flag[flagFiberAllocated]
      || _flag[flagSeedPositions]
      || _flag[flagFiberContext]
      /* HEY: wrong: probe stuff should have some dedicated logic! */
      || _flag[flagProbeItem] || _flag[flagProbeMeasr]) {  
    double time0 = airTime();
    
    if (tenFiberMultiTrace(_tfx, _fibers, _nseed)) { ERROR; }
    if (_probeNrrd && _probeKind && _probeItem && _probeMeasr) {
      updateProbe();
    }
    if (tenFiberMultiPolyData(_tfx, _lpldFibers, _fibers)) { ERROR; }

    if (_fiberVertexNum != _lpldFibers->xyzwNum) {
      _fiberVertexNum = _lpldFibers->xyzwNum;
      _flag[flagFiberVertexNum] = true;
    }
    // allocate the RGBA colors, which are filled later
    if (limnPolyDataAlloc(_lpldFibers, 
                          (limnPolyDataInfoBitFlag(_lpldFibers) 
                           | 1 << limnPolyDataInfoRGBA),
                          _lpldFibers->xyzwNum,
                          _lpldFibers->xyzwNum,
                          _lpldFibers->primNum)) {
      fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(LIMN));
    }
      
    _flag[flagFiberAllocated] = false;
    _flag[flagSeedPositions] = false;
    _flag[flagFiberContext] = false;
    _flag[flagProbeItem] = false;
    _flag[flagProbeMeasr] = false;
    _flag[flagFiberGeometry] = true;
    _fiberGeometryTime = airTime() - time0;
  }
}

void
HyperStreamline::colorQuantity(int quantity) {
  // char me[]="HyperStreamline::colorQuantity";
  if (colorQuantity() != quantity) {
    // fprintf(stderr, "!%s: hello %d\n", me, quantity);
    dynamic_cast<PolyProbe*>(this)->colorQuantity(quantity);
    _flag[flagColorMap] = true;
  }
}

int
HyperStreamline::colorQuantity() {
  return dynamic_cast<PolyProbe*>(this)->colorQuantity();
}

void
HyperStreamline::brightness(double br) {
  // char me[]="void PolyProbe::brightness";
  limnPolyData *lpldTmp;

  if (_brightness != br) {
    // fprintf(stderr, "!%s: %g\n", me, br);
    _brightness = br;
    lpldTmp = _lpldOwn; // TERRIBLE ...
    _lpldOwn = _lpldFibers;
    dynamic_cast<PolyProbe*>(this)->brightness(_brightness);
    dynamic_cast<PolyProbe*>(this)->update(false);
    _lpldOwn = lpldTmp;
    _flag[flagColorMap] = true;
  }
}

void
HyperStreamline::updateFiberColor() {
  /* char me[]="HyperStreamline::updateFiberColor"; */
  if (_flag[flagFiberGeometry]
      || _flag[flagColorMap]
      || _flag[flagFiberStopColorDo]) { // may have to refresh endpoint colors
    double time0 = airTime();

    _lpldOwn = _lpldFibers;   // TERRIBLE ...
    if (this->color()) {
      dynamic_cast<PolyProbe*>(this)->update(true);
    } else {
      for (unsigned int cidx=0; cidx<_lpldFibers->rgbaNum; cidx++) {
        ELL_4V_SET(_lpldFibers->rgba + 4*cidx, 255, 255, 255, 255);
      }
    }

    _flag[flagFiberGeometry] = false;
    _flag[flagColorMap] = false;
    _flag[flagFiberColor] = true;
    _fiberColorTime = airTime() - time0;
  }
}

void
HyperStreamline::updateTubeGeometry() {
  char me[]="HyperStreamline::updateTubeGeometry";
  if (_flag[flagTubeRadius]
      || _flag[flagTubeAllocated]
      || _flag[flagFiberGeometry]
      || _flag[flagTubeFacet]
      || _flag[flagFiberVertexNum]) {

    double time0 = airTime();
    
    /* the smarts that were developed here have been moved down to Teem */
    if (limnPolyDataSpiralTubeWrap(_lpldTubes, _lpldFibers,
                                   (1 << limnPolyDataInfoRGBA)
                                   | (1 << limnPolyDataInfoNorm),
                                   _nTubeVertMap, _tubeFacet, _endFacet,
                                   _tubeRadius)) {

      fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(LIMN));
    }

    _flag[flagTubeFacet] = false;
    _flag[flagFiberVertexNum] = false;
    _flag[flagTubeAllocated] = true;
    _flag[flagTubeRadius] = false;
    _flag[flagTubeGeometry] = true;
    _tubeGeometryTime = airTime() - time0;
  }
}

void
HyperStreamline::updateFiberStopColor() {
  // char me[]="HyperStreamline::updateFiberStopColor";
  unsigned char stcol[TEN_FIBER_STOP_MAX+1][4] = {
    {  0,   0,   0, 255},  /* tenFiberStopUnknown */
    {255, 255, 255, 255},  /* tenFiberStopAniso: white */
    {255,   0, 255, 255},  /* tenFiberStopLength: magenta */
    {  0, 255, 255, 255},  /* tenFiberStopNumSteps: cyan */
    {128, 128, 128, 255},  /* tenFiberStopConfidence: gray */
    {255, 255,   0, 255},  /* tenFiberStopRadius: yellow */
    {  0,   0,   0, 255},  /* tenFiberStopBounds: black */
    { 16, 255,  16, 255},  /* tenFiberStopFraction: green */
    { 25, 255,  25, 255}}; /* tenFiberStopStub: wacky, should never see */

  if (_flag[flagFiberColor]
      || _flag[flagFiberStopColorDo]) {
    double time0 = airTime();
    if (_stopColorDo) {
      unsigned int inVertTotalIdx = 0;
      unsigned int primIdx = 0;
      for (unsigned int fi=0; fi<_fibers->fiberNum; fi++) {
        if (!(tenFiberStopUnknown == _fibers->fiber[fi].whyNowhere)) {
	  // this fiber was a non-starter
          continue;
        }
        for (unsigned int inVertIdx=0;
             inVertIdx<_lpldFibers->icnt[primIdx];
             inVertIdx++) {
          if (0 == inVertIdx) {
            ELL_4V_COPY(_lpldFibers->rgba + 4*inVertTotalIdx,
                        stcol[_fibers->fiber[fi].whyStop[0]]);
          } else if (inVertIdx == _lpldFibers->icnt[primIdx]-1) {
            ELL_4V_COPY(_lpldFibers->rgba + 4*inVertTotalIdx,
                        stcol[_fibers->fiber[fi].whyStop[1]]);
          }
          inVertTotalIdx++;
        }
	primIdx++;
      }
    }
    _fiberStopColorTime = airTime() - time0;
    _flag[flagFiberColor] = false;
    _flag[flagFiberStopColorDo] = false;
    _flag[flagFiberStopColor] = true;
  }
}

void
HyperStreamline::updateTubeColor() {
  // char me[]="HyperStreamline::updateTubeColor";
  if (_flag[flagTubeAllocated]
      || _flag[flagFiberStopColor]) {
    double time0 = airTime();
    unsigned int inVertTotalIdx = 0;
    unsigned int outVertTotalIdx = 0, num;
    unsigned int *vertmap;

    /* the smarts for mapping from tube to fiber indices has been moved
       down to the nvertmap of limnPolyDataSpiralTubeWrap() */
    num = AIR_CAST(unsigned int, _lpldTubes->xyzwNum);
    vertmap = AIR_CAST(unsigned int *, _nTubeVertMap->data);
    for (outVertTotalIdx=0; outVertTotalIdx<num; outVertTotalIdx++) {
      inVertTotalIdx = vertmap[outVertTotalIdx];
      ELL_4V_COPY(_lpldTubes->rgba + 4*outVertTotalIdx,
                  _lpldFibers->rgba + 4*inVertTotalIdx);
    }

    _flag[flagTubeAllocated] = false;
    _flag[flagTubeColor] = true;
    _tubeColorTime = airTime() - time0;
  }
}

void
HyperStreamline::update() {
  char me[]="HyperStreamline::update";

  if (0 == _seedNum) {
    fprintf(stderr, "%s: (no seed points, bye)\n", me);
    return;
  }

  updateFiberAllocated();
  updateFiberContext();
  updateFiberGeometry();
  if (_tubeDo) {
    updateTubeGeometry();
  }
  updateFiberColor();
  updateFiberStopColor();
  if (_tubeDo) {
    updateTubeColor();
  }
  if (_flag[flagTubeDo]
      || _flag[flagTubeGeometry]
      || _flag[flagTubeColor]
      || _flag[flagFiberStopColor]
      || _flag[flagProbeCap]) {
    if (_tubeDo) {
      _lpldOwn = _lpldTubes;
      lightingUse(true);
    } else {
      _lpldOwn = _lpldFibers;
      lightingUse(false);
    }

    if (_flag[flagProbeCap]) {
      unsigned int primIdx = 0;
      for (unsigned int fi=0; fi<_fibers->fiberArr->len; fi++) {
        if (!(tenFiberStopUnknown == _fibers->fiber[fi].whyNowhere)) {
          continue; // no primIdx++
        }
        if (_probeCap && fi > _probeCap) {
          _lpldOwn->type[primIdx] = limnPrimitiveNoop;
        }  else {
          _lpldOwn->type[primIdx] = (_tubeDo
                                     ? limnPrimitiveTriangleStrip
                                     : limnPrimitiveLineStrip);
        }
        primIdx++;
      }
      _flag[flagProbeCap] = false;
    }

    /*
    if (1 == dynamic_cast<PolyData*>(this)->values()[0]->length()) {
      // hack here to do per-fiber statistics
      unsigned int primIdx = 0;
      for (unsigned int fi=0; fi<_fibers->fiberArr->len; fi++) {
        if (!(tenFiberStopUnknown == _fiber[fi].whyNowhere)) {
          continue; // no primIdx++
        }

        if (_fiber[fi].measr[nrrdMeasureMean] < 1.06) {
          _lpldOwn->type[primIdx] = limnPrimitiveNoop;
        } else {
          _lpldOwn->type[primIdx] = (_tubeDo
                                     ? limnPrimitiveTriangleStrip
                                     : limnPrimitiveLineStrip);
        }
        primIdx++;
      }
    } else {
      // undo hack 
      unsigned int primIdx = 0;
      for (unsigned int fi=0; fi<_fibers->fiberArr->len; fi++) {
        if (!(tenFiberStopUnknown == _fiber[fi].whyNowhere)) {
          continue; // no primIdx++
        }
        _lpldOwn->type[primIdx] = (_tubeDo
                                   ? limnPrimitiveTriangleStrip
                                   : limnPrimitiveLineStrip);
        primIdx++;
      }

    }
    */

    _flag[flagTubeDo] = false;
    _flag[flagTubeGeometry] = false;
    _flag[flagTubeColor] = false;
    _flag[flagFiberStopColor] = false;
    _flag[flagResult] = true;
  }
  if (_flag[flagResult]) {
    changed();  // to invalidate display lists
    _flag[flagResult] = false;
  }
}

} /* namespace Deft */
