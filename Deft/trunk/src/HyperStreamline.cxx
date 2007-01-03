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

HyperStreamlineSingle::HyperStreamlineSingle() {
  // char me[]="HyperStreamlineSingle::HyperStreamlineSingle";
  nvert = nrrdNew();
  primIdx = 0;
  ELL_3V_SET(seedPos, AIR_NAN, AIR_NAN, AIR_NAN);
  halfLen[0] = AIR_NAN;
  halfLen[1] = AIR_NAN;
  seedIdx = 0;
  stepNum[0] = 0;
  stepNum[1] = 0;
  whyStop[0] = tenFiberStopUnknown;
  whyStop[1] = tenFiberStopUnknown;
  whyNowhere = tenFiberStopUnknown;
}

HyperStreamlineSingle::~HyperStreamlineSingle() {
  
  nrrdNuke(nvert);
}

// ---------------------------------------------------------------

enum {
  flagUnknown,
  flagSeeds,
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
  flagTubeGeometry,
  flagTubeColor,
  flagTubeDo,
  flagResult,
  flagLast
};

#define ERROR \
  err = biffGetDone(TEN); \
  fprintf(stderr, "%s: trouble modifying the fiber context:\n%s\n", me, err); \
  free(err); exit(1)

HyperStreamline::HyperStreamline(const Volume *vol) {
  char me[]="HyperStreamline::HyperStreamline", *err;
  NrrdKernelSpec lksp;

  this->_dwi = !strcmp(TEN_DWI_GAGE_KIND_NAME, vol->kind()->name);

  fprintf(stderr, "!%s: welcome gotDwi = %s (%s)\n", me,
          this->_dwi ? "true" : "false",
          vol->kind()->name);

  _tfx = (this->_dwi
          ? tenFiberContextDwiNew(vol->nrrd())
          : tenFiberContextNew(vol->nrrd()));
  if (!_tfx) {
    ERROR;
  }
  if (tenFiberParmSet(_tfx, tenFiberParmUseIndexSpace, AIR_FALSE)) {
    ERROR;
  }
  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = false;
  }

  _seedNum = 0;
  _fiberNum = 0;
  _fiberVertexNum = 0;

  // TERRIBLE: playing dangerous games with ownership of _lpldOwn...
  limnPolyDataNix(_lpldOwn);
  _lpldFibers = limnPolyDataNew();
  _lpldOwn = _lpldFibers;
  _lpldTubes = limnPolyDataNew();

  // default fiber context settings
  this->color(true);
  fiberType(tenFiberTypeEvec1);
  stopConfidence(0.5);
  stopAniso(tenAniso_Cl2, 0.3);
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

  _nseeds = nrrdNew();
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

  _fiberAllocatedTime = 0;
  _fiberGeometryTime = 0;
  _fiberColorTime = 0;
  _fiberStopColorTime = 0;
  _tubeAllocatedTime = 0;
  _tubeGeometryTime = 0;
  _tubeColorTime = 0;
}

HyperStreamline::~HyperStreamline() {

  for (unsigned int fi=0; fi<_fiber.size(); fi++) {
    delete _fiber[fi];
  }
  tenFiberContextNix(_tfx);
  // TERRIBLE
  _lpldOwn = _lpldFibers;
  // limnPolyDataNix(_lpldFibers);   (that's now done by PolyData's dtor)
  limnPolyDataNix(_lpldTubes);
  nrrdNuke(_nseeds);
}

void
HyperStreamline::seedsSet(const Nrrd *nseeds) {
  char me[]="HyperStreamline::seedsSet";
  unsigned int seedNumNew;

  if (nseeds) {
    if (!( 2 == nseeds->dim && 3 == nseeds->axis[0].size)) {
      fprintf(stderr, "%s: trouble: nseeds %u-D %u-by-X, not 2-D 3-by-X\n",
              me, nseeds->dim, AIR_CAST(unsigned int, nseeds->axis[0].size));
    }
    if (nrrdConvert(_nseeds, nseeds, nrrdTypeFloat)) {
      fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(NRRD));
    }
    seedNumNew = _nseeds->axis[1].size;
  } else {
    seedNumNew = 0;
  }
  _flag[flagSeeds] = false;  // HEY- is this serving any purpose?

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
HyperStreamline::parmCopy(HyperStreamline *src) {

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
    // fprintf(stderr, "!%s: hello\n", me);
    // currently allocate more when needed, and never free
    double time0 = airTime();
    if (_seedNum > _fiber.size()) {
      size_t sizeOld = _fiber.size();
      _fiber.resize(_seedNum);
      for (unsigned int seedIdx=sizeOld;
           seedIdx<_fiber.size();
           seedIdx++) {
        _fiber[seedIdx] = new HyperStreamlineSingle();
      }
    }
    _flag[flagSeedNum] = false;
    _flag[flagFiberAllocated] = true;
    _fiberAllocatedTime = airTime() - time0;
  }
}

void
HyperStreamline::updateFiberGeometry() {
  char me[]="HyperStreamline::updateFiberGeometry";
  char *err;

  if (_flag[flagFiberAllocated]
      || _flag[flagSeedPositions]
      || _flag[flagFiberContext]) {
    double time0 = airTime();
    float *pos = static_cast<float*>(_nseeds->data);
    unsigned int vertTotalNum = 0;
    _fiberNum = 0;
    bool verbose = _seedNum < 100;
    for (unsigned int seedIdx=0; seedIdx<_seedNum; seedIdx++) {
      ELL_3V_COPY(_fiber[seedIdx]->seedPos, pos + 3*seedIdx);
      if (this->_dwi) {
        _tfx->ten2Which = seedIdx % 2;
      }
      if (verbose) {
        fprintf(stderr, "!%s: trace %u/%u:\n", me,
                seedIdx, _seedNum);
      }
      if (tenFiberTrace(_tfx, _fiber[seedIdx]->nvert, 
                        _fiber[seedIdx]->seedPos)) { ERROR; }
      if (verbose) {
        fprintf(stderr, " (%g,%g,%g): ", 
                _fiber[seedIdx]->seedPos[0],
                _fiber[seedIdx]->seedPos[1],
                _fiber[seedIdx]->seedPos[2]);
        if (_tfx->whyNowhere) {
          fprintf(stderr, "whyNowhere = %s\n", 
                  airEnumStr(tenFiberStop, _tfx->whyNowhere));
        } else {
          fprintf(stderr, "whyStop[b,f] = %s,%s\n",
                  airEnumStr(tenFiberStop, _tfx->whyStop[0]),
                  airEnumStr(tenFiberStop, _tfx->whyStop[1]));
        }
      }
      _fiber[seedIdx]->whyNowhere = _tfx->whyNowhere;
      if (tenFiberStopUnknown == _fiber[seedIdx]->whyNowhere) {
        _fiber[seedIdx]->primIdx = _fiberNum++;
        _fiber[seedIdx]->halfLen[0] = _tfx->halfLen[0];
        _fiber[seedIdx]->halfLen[1] = _tfx->halfLen[1];
        _fiber[seedIdx]->seedIdx = _tfx->numSteps[0];
        _fiber[seedIdx]->stepNum[0] = _tfx->numSteps[0];
        _fiber[seedIdx]->stepNum[1] = _tfx->numSteps[1];
        vertTotalNum += _fiber[seedIdx]->nvert->axis[1].size;
        _fiber[seedIdx]->whyStop[0] = _tfx->whyStop[0];
        _fiber[seedIdx]->whyStop[1] = _tfx->whyStop[1];
        /*
        fprintf(stderr, "!%s: A fiber[%u/%u] (%p) "
                "len = %u (%u+%u) (%s/%s)\n", me,
                seedIdx, _fiber[seedIdx]->primIdx,
		&(_fiber[seedIdx]),
                AIR_CAST(unsigned int, _fiber[seedIdx]->nvert->axis[1].size),
		_fiber[seedIdx]->stepNum[0],
		_fiber[seedIdx]->stepNum[1],
                airEnumStr(tenFiberStop, _fiber[seedIdx]->whyStop[0]),
                airEnumStr(tenFiberStop, _fiber[seedIdx]->whyStop[1]));
        */
      } else {
        /* the seed point was a non-starter- either things died
           immediately, or it was a stub fiber */
        _fiber[seedIdx]->primIdx = AIR_CAST(unsigned int, -1);
        _fiber[seedIdx]->halfLen[0] = AIR_NAN;
        _fiber[seedIdx]->halfLen[1] = AIR_NAN;
        _fiber[seedIdx]->seedIdx = 0;
        _fiber[seedIdx]->stepNum[0] = 0;
        _fiber[seedIdx]->stepNum[1] = 0;
        _fiber[seedIdx]->whyStop[0] = _fiber[seedIdx]->whyNowhere;
        _fiber[seedIdx]->whyStop[1] = _fiber[seedIdx]->whyNowhere;
	/*
        fprintf(stderr, "!%s: B fiber[%u] (%p) len = %u+%u (%s/%s)\n", me,
                seedIdx,
		&(_fiber[seedIdx]),
		_fiber[seedIdx]->stepNum[0],
		_fiber[seedIdx]->stepNum[1],
                airEnumStr(tenFiberStop, _fiber[seedIdx]->whyStop[0]),
                airEnumStr(tenFiberStop, _fiber[seedIdx]->whyStop[1]));
	*/
      }
    }
    
    // allocate all polylines; _fiberNum == primNum
    limnPolyDataAlloc(_lpldFibers, 
                      (1 << limnPolyDataInfoRGBA)
                      | (1 << limnPolyDataInfoNorm),
                      vertTotalNum, vertTotalNum, _fiberNum);
    if (_fiberVertexNum != vertTotalNum) {
      _fiberVertexNum = vertTotalNum;
      _flag[flagFiberVertexNum] = true;
    }
    
    // NOTE: we index through the seed points to iterate through fibers,
    // but primIdx only increments for the seedpoints that went somewhere.
    // So, primIdx should end at _fiberNum (calculated above) for last fiber
    unsigned int primIdx = 0;
    unsigned int vertTotalIdx = 0;
    for (unsigned int seedIdx=0; seedIdx<_seedNum; seedIdx++) {

      if (!(tenFiberStopUnknown == _fiber[seedIdx]->whyNowhere)) {
        continue; // no primIdx++
      }

      unsigned int vertNum = _fiber[seedIdx]->nvert->axis[1].size;
      double *vert = static_cast<double*>(_fiber[seedIdx]->nvert->data);
      /*
      char fname[128];
      sprintf(fname, "fiber-%03u.txt", seedIdx);
      nrrdSave(fname, _fiber[seedIdx]->nvert, NULL);
      */
      for (unsigned int vertIdx=0; vertIdx<vertNum; vertIdx++) {
        ELL_3V_COPY_TT(_lpldFibers->xyzw + 4*vertTotalIdx, float,
                       vert + 3*vertIdx);
        (_lpldFibers->xyzw + 4*vertTotalIdx)[3] = 1.0;
        ELL_3V_SET(_lpldFibers->norm + 3*vertTotalIdx, 0, 0, 0);
        ELL_4V_SET(_lpldFibers->rgba + 4*vertTotalIdx, 255, 255, 255, 255);
        _lpldFibers->indx[vertTotalIdx] = vertTotalIdx;
        vertTotalIdx++;
      }
      _lpldFibers->type[primIdx] = limnPrimitiveLineStrip;
      _lpldFibers->icnt[primIdx] = vertNum;
      primIdx++;
    }
    _flag[flagFiberAllocated] = false;
    _flag[flagSeedPositions] = false;
    _flag[flagFiberContext] = false;
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
  // char me[]="HyperStreamline::updateFiberColor";
  if (_flag[flagFiberGeometry]
      || _flag[flagColorMap]
      || _flag[flagFiberStopColorDo]) { // may have to refresh endpoint colors
    double time0 = airTime();
    _lpldOwn = _lpldFibers;   // TERRIBLE ...
    dynamic_cast<PolyProbe*>(this)->update(true);
    _flag[flagFiberGeometry] = false;
    _flag[flagColorMap] = false;
    _flag[flagFiberColor] = true;
    _fiberColorTime = airTime() - time0;
  }
}

void
HyperStreamline::updateTubeAllocated() {
  // char me[]="HyperStreamline::updateTubeAllocated";
  if (_flag[flagTubeFacet]
      || _flag[flagFiberVertexNum]) {
    double time0 = airTime();
    unsigned int tubeVertNum = 0;
    unsigned int tubeIndxNum = 0;
    for (unsigned int primIdx=0; primIdx<_lpldFibers->primNum; primIdx++) {
      tubeVertNum += _tubeFacet*(2*_endFacet
                                 + _lpldFibers->icnt[primIdx]) + 1;
      tubeIndxNum += 2*_tubeFacet*(2*_endFacet
                                   + _lpldFibers->icnt[primIdx] + 1)-2;
    }
    limnPolyDataAlloc(_lpldTubes,
                      (1 << limnPolyDataInfoRGBA)
                      | (1 << limnPolyDataInfoNorm),
                      tubeVertNum, tubeIndxNum, _lpldFibers->primNum);
    _flag[flagTubeFacet] = false;
    _flag[flagFiberVertexNum] = false;
    _flag[flagTubeAllocated] = true;
    _tubeAllocatedTime = airTime() - time0;
  }
}

void
HyperStreamline::updateTubeGeometry() {
  // char me[]="HyperStreamline::updateTubeGeometry";
  if (_flag[flagTubeRadius]
      || _flag[flagTubeAllocated]
      || _flag[flagFiberGeometry]) {
    double time0 = airTime();
    std::vector<double> cost(_tubeFacet);
    std::vector<double> sint(_tubeFacet);
    for (unsigned int pi=0; pi<_tubeFacet; pi++) {
      double angle = AIR_AFFINE(0, pi, _tubeFacet, 0, 2*AIR_PI);
      cost[pi] = cos(angle);
      sint[pi] = sin(angle);
    }
    unsigned int inVertTotalIdx = 0;
    unsigned int outVertTotalIdx = 0;
    unsigned int outIndxIdx = 0;
    for (unsigned int primIdx=0; primIdx<_lpldFibers->primNum; primIdx++) {
      _lpldTubes->type[primIdx] = limnPrimitiveTriangleStrip;
      _lpldTubes->icnt[primIdx] = 2*_tubeFacet*(2*_endFacet 
						+ _lpldFibers->icnt[primIdx] + 1) - 2;
      for (unsigned int inVertIdx=0;
           inVertIdx<_lpldFibers->icnt[primIdx];
           inVertIdx++) {
        unsigned int forwIdx, backIdx;
        double tang[3], tmp, scl, step, perp[3], pimp[3];
        /* inVrt = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx]; */
        if (0 == inVertIdx) {
          forwIdx = inVertTotalIdx+1;
          backIdx = inVertTotalIdx;
          scl = 1;
        } else if (_lpldFibers->icnt[primIdx]-1 == inVertIdx) {
          forwIdx = inVertTotalIdx;
          backIdx = inVertTotalIdx-1;
          scl = 1;
        } else {
          forwIdx = inVertTotalIdx+1;
          backIdx = inVertTotalIdx-1;
          scl = 0.5;
        }
        if (1 == _lpldFibers->icnt[primIdx]) {
          ELL_3V_SET(tang, 0, 0, 1); // completely arbitrary, as it must be
          step = 0;
        } else {
          ELL_3V_SUB(tang,
                     _lpldFibers->xyzw + 4*forwIdx,
                     _lpldFibers->xyzw + 4*backIdx);
          ELL_3V_NORM(tang, tang, step);
          step *= scl;  /* step now length of distance between input verts, 
                           which may be different from nominal fiber step
                           size because of anisoSpeed stuff */
        }
        if (0 == inVertIdx || 1 == _lpldFibers->icnt[primIdx]) {
          ell_3v_perp_d(perp, tang);
        } else {
          // transport last perp forwards
          double dot = ELL_3V_DOT(perp, tang);
          ELL_3V_SCALE_ADD2(perp, 1.0, perp, -dot, tang);
        }
        ELL_3V_NORM(perp, perp, tmp);
        ELL_3V_CROSS(pimp, perp, tang);
        // (perp, pimp, tang) is a left-handed frame, on purpose
        // limnVrt *outVrt;
        // -------------------------------------- BEGIN initial endcap
        if (0 == inVertIdx) {
          unsigned int startIdx = outVertTotalIdx;
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              double costh, sinth, cosph, sinph;
              double phi = (AIR_AFFINE(0, ei, _endFacet, 0, AIR_PI/2)
                            + AIR_AFFINE(0, pi, _tubeFacet, 
                                         0, AIR_PI/2)/_endFacet);
              double theta = AIR_AFFINE(0, pi, _tubeFacet, 0.0, 2*AIR_PI);
              cosph = cos(phi);
              sinph = sin(phi);
              costh = cos(theta);
              sinth = sin(theta);
              ELL_3V_SCALE_ADD3_TT(_lpldTubes->norm + 3*outVertTotalIdx, float,
                                   -cosph, tang,
                                   costh*sinph, perp,
                                   sinth*sinph, pimp);
              ELL_3V_SCALE_ADD3_TT(_lpldTubes->xyzw + 4*outVertTotalIdx, float,
                                   1, _lpldFibers->xyzw + 4*inVertTotalIdx,
                                   -step/2, tang,
                                   _tubeRadius,
                                   _lpldTubes->norm + 3*outVertTotalIdx);
              (_lpldTubes->xyzw + 4*outVertTotalIdx)[3] = 1.0;
              outVertTotalIdx++;
            }
          }
          for (unsigned int pi=1; pi<_tubeFacet; pi++) {
            _lpldTubes->indx[outIndxIdx++] = startIdx;
            _lpldTubes->indx[outIndxIdx++] = startIdx + pi;
          }
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            /* at the highest ei we're actually linking with the first 
               row of vertices at the start of the tube */
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              _lpldTubes->indx[outIndxIdx++] = (startIdx + pi
                                                + (ei + 0)*_tubeFacet);
              _lpldTubes->indx[outIndxIdx++] = (startIdx + pi
                                                + (ei + 1)*_tubeFacet);
            }
          }
        }
        // -------------------------------------- END initial endcap
        for (unsigned int pi=0; pi<_tubeFacet; pi++) {
          double shift = AIR_AFFINE(-0.5, pi, _tubeFacet-0.5, -step/2, step/2);
          double cosa = cost[pi];
          double sina = sint[pi];
          // outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_3V_SCALE_ADD2_TT(_lpldTubes->norm + 3*outVertTotalIdx, float,
                               cosa, perp, sina, pimp);
          ELL_3V_SCALE_ADD3_TT(_lpldTubes->xyzw + 4*outVertTotalIdx, float,
                               1, _lpldFibers->xyzw + 4*inVertTotalIdx,
                               _tubeRadius,
                               _lpldTubes->norm + 3*outVertTotalIdx,
                               shift, tang);
          (_lpldTubes->xyzw + 4*outVertTotalIdx)[3] = 1.0;
          _lpldTubes->indx[outIndxIdx++] = outVertTotalIdx;
          _lpldTubes->indx[outIndxIdx++] = outVertTotalIdx + _tubeFacet;
          outVertTotalIdx++;
        }
        unsigned int tubeEndIdx = outVertTotalIdx;
        // -------------------------------------- BEGIN final endcap
        if (inVertIdx == _lpldFibers->icnt[primIdx]-1) {
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              double costh, sinth, cosph, sinph;
              double phi = (AIR_AFFINE(0, ei, _endFacet, AIR_PI/2, AIR_PI)
                            + AIR_AFFINE(0, pi, _tubeFacet, 
                                         0, AIR_PI/2)/_endFacet);
              double theta = AIR_AFFINE(0, pi, _tubeFacet, 0.0, 2*AIR_PI);
              cosph = cos(phi);
              sinph = sin(phi);
              costh = cos(theta);
              sinth = sin(theta);
              // outVrt = _lpldTubes->vert + outVertTotalIdx;
              ELL_3V_SCALE_ADD3_TT(_lpldTubes->norm + 3*outVertTotalIdx, float,
                                   -cosph, tang,
                                   costh*sinph, perp,
                                   sinth*sinph, pimp);
              ELL_3V_SCALE_ADD3_TT(_lpldTubes->xyzw + 4*outVertTotalIdx, float,
                                   1, _lpldFibers->xyzw + 4*inVertTotalIdx,
                                   step/2, tang,
                                   _tubeRadius,
                                   _lpldTubes->norm + 3*outVertTotalIdx);
              (_lpldTubes->xyzw + 4*outVertTotalIdx)[3] = 1.0;
              outVertTotalIdx++;
            }
          }
          // outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_3V_COPY_TT(_lpldTubes->norm + 3*outVertTotalIdx, float, tang);
          ELL_3V_SCALE_ADD3_TT(_lpldTubes->xyzw + 4*outVertTotalIdx, float,
                               1, _lpldFibers->xyzw + 4*inVertTotalIdx,
                               step/2, tang,
                               _tubeRadius,
                               _lpldTubes->norm + 3*outVertTotalIdx);
          (_lpldTubes->xyzw + 4*outVertTotalIdx)[3] = 1.0;
          outVertTotalIdx++;
          for (unsigned int ei=0; ei<_endFacet-1; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              _lpldTubes->indx[outIndxIdx++] = (tubeEndIdx + pi
                                                + (ei + 0)*_tubeFacet);
              _lpldTubes->indx[outIndxIdx++] = (tubeEndIdx + pi
                                                + (ei + 1)*_tubeFacet);
            }
          }
          for (unsigned int pi=0; pi<_tubeFacet; pi++) {
            _lpldTubes->indx[outIndxIdx++] = (tubeEndIdx + pi
                                              + (_endFacet - 1)*_tubeFacet);
            _lpldTubes->indx[outIndxIdx++] = (tubeEndIdx
                                              + (_endFacet - 0)*_tubeFacet);
          }
        }
        // -------------------------------------- END final endcap
        inVertTotalIdx++;
      }
    }
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
    { 25, 255,  25, 255}}; /* tenFiberStopStub: wacky, should never see */

  // limnVrt *inVrt;
  if (_flag[flagFiberColor]
      || _flag[flagFiberStopColorDo]) {
    double time0 = airTime();
    if (_stopColorDo) {
      unsigned int inVertTotalIdx = 0;
      unsigned int primIdx = 0;
      for (unsigned int fi=0; fi<_fiber.size(); fi++) {
	if (0 == _fiber[fi]->stepNum[0] + _fiber[fi]->stepNum[1]) {
	  // this fiber was a non-starter
	  continue;
	}
        for (unsigned int inVertIdx=0;
             inVertIdx<_lpldFibers->icnt[primIdx];
             inVertIdx++) {
          // inVrt = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx];
          if (0 == inVertIdx) {
            ELL_4V_COPY(_lpldFibers->rgba + 4*inVertTotalIdx,
                        stcol[_fiber[fi]->whyStop[0]]);
          } else if (inVertIdx == _lpldFibers->icnt[primIdx]-1) {
            ELL_4V_COPY(_lpldFibers->rgba + 4*inVertTotalIdx,
                        stcol[_fiber[fi]->whyStop[1]]);
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
    unsigned int outVertTotalIdx = 0;
    unsigned int primIdx = 0;
    // limnVrt *inVrt, *outVrt;
    for (unsigned int fi=0; fi<_fiber.size(); fi++) {
      if (0 == _fiber[fi]->stepNum[0] + _fiber[fi]->stepNum[1]) {
	// this fiber was a non-starter
	continue;
      }
      for (unsigned int inVertIdx=0;
           inVertIdx<_lpldFibers->icnt[primIdx];
           inVertIdx++) {
        // inVrt = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx];
        if (0 == inVertIdx) {
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              // outVrt = _lpldTubes->vert + outVertTotalIdx;
              ELL_4V_COPY(_lpldTubes->rgba + 4*outVertTotalIdx,
                          _lpldFibers->rgba + 4*inVertTotalIdx);
              outVertTotalIdx++;
            }
          }
        }
        for (unsigned int pi=0; pi<_tubeFacet; pi++) {
          // outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_4V_COPY(_lpldTubes->rgba + 4*outVertTotalIdx,
                      _lpldFibers->rgba + 4*inVertTotalIdx);
          outVertTotalIdx++;
        }
        if (inVertIdx == _lpldFibers->icnt[primIdx]-1) {
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              // outVrt = _lpldTubes->vert + outVertTotalIdx;
              ELL_4V_COPY(_lpldTubes->rgba + 4*outVertTotalIdx,
                          _lpldFibers->rgba + 4*inVertTotalIdx);
              outVertTotalIdx++;
            }
          }
          // outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_4V_COPY(_lpldTubes->rgba + 4*outVertTotalIdx,
                      _lpldFibers->rgba + 4*inVertTotalIdx);
          outVertTotalIdx++;
        }
        inVertTotalIdx++;
      }
      primIdx++;
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
    updateTubeAllocated();
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
      || _flag[flagFiberStopColor]) {
    if (_tubeDo) {
      _lpldOwn = _lpldTubes;
      lightingUse(true);
    } else {
      _lpldOwn = _lpldFibers;
      lightingUse(false);
    }
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
