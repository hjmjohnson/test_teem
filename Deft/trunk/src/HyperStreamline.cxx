/*
  Deft: experiments in minimalist scientific visualization 
  Copyright (C) 2005  Gordon Kindlmann

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

HyperStreamline::HyperStreamline(const Nrrd *nten) {
  char me[]="HyperStreamline::HyperStreamline", *err;
  
  _tfx = tenFiberContextNew(nten);
  if (!_tfx) {
    ERROR;
  }
  if (tenFiberParmSet(_tfx, tenFiberParmUseIndexSpace, AIR_FALSE)) {
    ERROR;
  }
  for (unsigned fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = false;
  }

  // bogus initialization to avoid use of unitialized variables
  _tubeDo = true;
  _tubeFacet = 0;
  _endFacet = 0;
  _tubeRadius = -1;
  _seedNum = 0;
  _fiberVertexNum = 0;

  // default fiber context settings
  fiberType(tenFiberTypeEvec1);
  stopConfidence(0.5);
  stopAniso(tenAniso_Cl2, 0.3);
  step(0.01);
  stopHalfStepNum(30);
  integration(tenFiberIntgRK4);
  NrrdKernelSpec *ksp = nrrdKernelSpecNew();
  nrrdKernelSpecParse(ksp, "tent");
  kernel(ksp);
  _volume = new Volume(tenGageKind, nten);
  volume(_volume);
  dynamic_cast<PolyProbe*>(this)->kernel(gageKernel00, ksp);
  _nseeds = nrrdNew();
  nrrdKernelSpecNix(ksp);
  dynamic_cast<PolyProbe*>(this)->evecRgbAnisoGamma(1.3);
  dynamic_cast<PolyProbe*>(this)->evecRgbIsoGray(1.0);
  dynamic_cast<PolyProbe*>(this)->evecRgbMaxSat(1.0);
  brightness(1.0);
  tubeDo(false);
  tubeFacet(4);
  tubeRadius(0.01);

  // TERRIBLE: playing dangerous games with ownership of _lpldOwn...
  limnPolyDataNix(_lpldOwn);
  _lpldFibers = limnPolyDataNew();
  _lpldOwn = _lpldFibers;
  _lpldTubes = limnPolyDataNew();
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

  // HEY: type and size checks!
  if (nseeds) {
    if (nrrdCopy(_nseeds, nseeds)) {
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
HyperStreamline::stopReset() { tenFiberStopReset(_tfx); }

void
HyperStreamline::tubeDo(bool doit) {
  if (_tubeDo != doit) {
    _tubeDo = doit;
    _flag[flagTubeDo] = true;
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
  }
}

void
HyperStreamline::updateFiberGeometry() {
  char me[]="HyperStreamline::updateFiberGeometry";
  char *err;

  if (_flag[flagFiberAllocated]
      || _flag[flagSeedPositions]
      || _flag[flagFiberContext]) {
    float *pos = static_cast<float*>(_nseeds->data);
    unsigned int vertTotalNum = 0;
    unsigned int primNum = 0;
    for (unsigned int seedIdx=0; seedIdx<_seedNum; seedIdx++) {
      ELL_3V_COPY(_fiber[seedIdx]->seedPos, pos + 3*seedIdx);
      if (tenFiberTrace(_tfx, _fiber[seedIdx]->nvert, 
                        _fiber[seedIdx]->seedPos)) { ERROR; }
      _fiber[seedIdx]->whyNowhere = _tfx->whyNowhere;
      if (tenFiberStopUnknown == _fiber[seedIdx]->whyNowhere) {
        /* hooray, we did get somewhere */
        _fiber[seedIdx]->primIdx = primNum++;
        _fiber[seedIdx]->halfLen[0] = _tfx->halfLen[0];
        _fiber[seedIdx]->halfLen[1] = _tfx->halfLen[1];
        _fiber[seedIdx]->seedIdx = _tfx->numSteps[0];
        _fiber[seedIdx]->stepNum[0] = _tfx->numSteps[0];
        _fiber[seedIdx]->stepNum[1] = _tfx->numSteps[1];
        vertTotalNum += _fiber[seedIdx]->nvert->axis[1].size;
        _fiber[seedIdx]->whyStop[0] = _tfx->whyStop[0];
        _fiber[seedIdx]->whyStop[1] = _tfx->whyStop[1];
      } else {
        /* sadly, we got nowhere */
        _fiber[seedIdx]->primIdx = AIR_CAST(unsigned int, -1);
        _fiber[seedIdx]->halfLen[0] = AIR_NAN;
        _fiber[seedIdx]->halfLen[1] = AIR_NAN;
        _fiber[seedIdx]->seedIdx = 0;
        _fiber[seedIdx]->stepNum[0] = 0;
        _fiber[seedIdx]->stepNum[1] = 0;
        _fiber[seedIdx]->whyStop[0] = _fiber[seedIdx]->whyNowhere;
        _fiber[seedIdx]->whyStop[1] = _fiber[seedIdx]->whyNowhere;
      }
    }
    
    // allocate all polylines 
    limnPolyDataAlloc(_lpldFibers, vertTotalNum, vertTotalNum, primNum);
    if (_fiberVertexNum != vertTotalNum) {
      _fiberVertexNum = vertTotalNum;
      _flag[flagFiberVertexNum] = true;
    }
    
    // NOTE: we index through the seed points to iterate through fibers,
    // but primIdx only increments for the seedpoints that went somewhere.
    // So, primIdx should end at primNum (calculated above) for last fiber
    unsigned int primIdx = 0;
    unsigned int vertTotalIdx = 0;
    // char fname[128];
    for (unsigned int seedIdx=0; seedIdx<_seedNum; seedIdx++) {
      if (tenFiberStopUnknown != _fiber[seedIdx]->whyNowhere) {
        continue; // no primIdx++
      }
      unsigned int vertNum = _fiber[seedIdx]->nvert->axis[1].size;
      double *vert = static_cast<double*>(_fiber[seedIdx]->nvert->data);
      /* 
         sprintf(fname, "fiber-%03u.nrrd", seedIdx);
         nrrdSave(fname, _fiber[seedIdx]->nvert, NULL);
      */
      for (unsigned int vertIdx=0; vertIdx<vertNum; vertIdx++) {
        ELL_3V_COPY_TT(_lpldFibers->vert[vertTotalIdx].xyzw, float,
                       vert + 3*vertIdx);
        _lpldFibers->vert[vertTotalIdx].xyzw[3] = 1.0;
        ELL_3V_SET(_lpldFibers->vert[vertTotalIdx].norm, 0, 0, 0);
        ELL_4V_SET(_lpldFibers->vert[vertTotalIdx].rgba, 255, 255, 255, 255);
        _lpldFibers->indx[vertTotalIdx] = vertTotalIdx;
        vertTotalIdx++;
      }
      _lpldFibers->type[primIdx] = limnPrimitiveLineStrip;
      _lpldFibers->vcnt[primIdx] = vertNum;
      primIdx++;
    }
    _flag[flagFiberAllocated] = false;
    _flag[flagSeedPositions] = false;
    _flag[flagFiberContext] = false;
    _flag[flagFiberGeometry] = true;
  }
}

void
HyperStreamline::colorQuantity(int quantity) {
  char me[]="HyperStreamline::colorQuantity";
  if (colorQuantity() != quantity) {
    fprintf(stderr, "!%s: hello\n", me);
    dynamic_cast<PolyProbe*>(this)->colorQuantity(quantity);
    _flag[flagColorMap] = true;
  }
}

int
HyperStreamline::colorQuantity() {
  return dynamic_cast<PolyProbe*>(this)->colorQuantity();
}

void
HyperStreamline::updateFiberColor() {
  char me[]="HyperStreamline::updateFiberColor";
  if (_flag[flagFiberGeometry]
      || _flag[flagColorMap]) {
    fprintf(stderr, "!%s: hello\n", me);
    _lpldOwn = _lpldFibers;   // TERRIBLE ...
    dynamic_cast<PolyProbe*>(this)->update(true);
    _flag[flagFiberGeometry] = false;
    _flag[flagColorMap] = false;
    _flag[flagFiberColor] = true;
  }
}

void
HyperStreamline::updateTubeAllocated() {
  char me[]="HyperStreamline::updateTubeAllocated";
  if (_flag[flagTubeFacet]
      || _flag[flagFiberVertexNum]) {
    unsigned int tubeVertNum = 0;
    unsigned int tubeIndxNum = 0;
    for (unsigned int fi=0; fi<_lpldFibers->primNum; fi++) {
      tubeVertNum += _tubeFacet*(2*_endFacet + _lpldFibers->vcnt[fi]) + 1;
      tubeIndxNum += 2*_tubeFacet*(2*_endFacet + _lpldFibers->vcnt[fi] + 1)-2;
    }
    limnPolyDataAlloc(_lpldTubes,
                      tubeVertNum, tubeIndxNum, _lpldFibers->primNum);
    fprintf(stderr, "!%s: ----- hello %u %u\n", me, tubeVertNum, tubeIndxNum);
    _flag[flagTubeFacet] = false;
    _flag[flagFiberVertexNum] = false;
    _flag[flagTubeAllocated] = true;
  }
}

void
HyperStreamline::updateTubeGeometry() {
  char me[]="HyperStreamline::updateTubeGeometry";
  if (_flag[flagTubeRadius]
      || _flag[flagTubeAllocated]
      || _flag[flagFiberGeometry]) {
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
    for (unsigned int fi=0; fi<_lpldFibers->primNum; fi++) {
      _lpldTubes->type[fi] = limnPrimitiveTriangleStrip;
      _lpldTubes->vcnt[fi] = 2*_tubeFacet*(2*_endFacet 
                                           + _lpldFibers->vcnt[fi] + 1) - 2;
      for (unsigned int inVertIdx=0;
           inVertIdx<_lpldFibers->vcnt[fi];
           inVertIdx++) {
        limnVrt *forw, *inVrt, *back;
        double tang[3], tmp, scl, step, perp[3], pimp[3];
        inVrt = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx];
        if (0 == inVertIdx) {
          forw = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx+1];
          back = inVrt;
          scl = 1;
        } else if (_lpldFibers->vcnt[fi]-1 == inVertIdx) {
          forw = inVrt;
          back = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx-1];
          scl = 1;
        } else {
          forw = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx+1];
          back = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx-1];
          scl = 0.5;
        }
        ELL_3V_SUB(tang, forw->xyzw, back->xyzw);
        ELL_3V_NORM(tang, tang, step);
        step *= scl;  /* step now length of distance between input verts, 
                         which may be different from nominal fiber step
                         size because of anisoSpeed stuff */
        if (0 == inVertIdx) {
          ell_3v_perp_d(perp, tang);
        } else {
          // transport last perp forwards
          double dot = ELL_3V_DOT(perp, tang);
          ELL_3V_SCALE_ADD2(perp, 1.0, perp, -dot, tang);
        }
        ELL_3V_NORM(perp, perp, tmp);
        ELL_3V_CROSS(pimp, perp, tang);
        // (perp, pimp, tang) is a left-handed frame, on purpose
        /*
        fprintf(stderr, "%s(%u,%u): (%g,%g,%g): "
                "(%g,%g,%g) (%g,%g,%g) (%g,%g,%g)\n",
              me, fi, inVertIdx, 
              inVrt->xyzw[0], inVrt->xyzw[1], inVrt->xyzw[2],
              tang[0], tang[1], tang[2],
              perp[0], perp[1], perp[2],
              pimp[0], pimp[2], pimp[2]);
        */
        limnVrt *outVrt;
        // -------------------------------------- BEGIN initial endcap
        if (0 == inVertIdx) {
          unsigned int startIdx = outVertTotalIdx;
          fprintf(stderr, "%s: ---------- BEGIN initial endcap\n", me);
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
              outVrt = _lpldTubes->vert + outVertTotalIdx;
              ELL_3V_SCALE_ADD3_TT(outVrt->norm, float,
                                   -cosph, tang,
                                   costh*sinph, perp,
                                   sinth*sinph, pimp);
              ELL_3V_SCALE_ADD3_TT(outVrt->xyzw, float,
                                   1, inVrt->xyzw,
                                   -step/2, tang,
                                   _tubeRadius, outVrt->norm);
              outVrt->xyzw[3] = 1.0;
              fprintf(stderr, ". %u (%g %g %g)\n", outVertTotalIdx,
                      outVrt->xyzw[0], outVrt->xyzw[1], outVrt->xyzw[2]);
              outVertTotalIdx++;
            }
          }
          for (unsigned int pi=1; pi<_tubeFacet; pi++) {
            fprintf(stderr, "indx[%u,%u] = %u %u\n",
                    outIndxIdx, outIndxIdx+1, startIdx, startIdx + pi);
            _lpldTubes->indx[outIndxIdx++] = startIdx;
            _lpldTubes->indx[outIndxIdx++] = startIdx + pi;
          }
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            /* at the highest ei we're actually linking with the first 
               row of vertices at the start of the tube */
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              fprintf(stderr, "indx[%u,%u] = %u %u\n",
                      outIndxIdx, outIndxIdx+1,
                      (startIdx + pi
                       + (ei + 0)*_tubeFacet),
                      (startIdx + pi
                       + (ei + 1)*_tubeFacet));
              _lpldTubes->indx[outIndxIdx++] = (startIdx + pi
                                                + (ei + 0)*_tubeFacet);
              _lpldTubes->indx[outIndxIdx++] = (startIdx + pi
                                                + (ei + 1)*_tubeFacet);
            }
          }
        }
        // -------------------------------------- END initial endcap
        fprintf(stderr, "%s: ---------- BEGIN tube\n", me);
        for (unsigned int pi=0; pi<_tubeFacet; pi++) {
          double shift = AIR_AFFINE(-0.5, pi, _tubeFacet-0.5, -step/2, step/2);
          double cosa = cost[pi];
          double sina = sint[pi];
          outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_3V_SCALE_ADD2_TT(outVrt->norm, float,
                               cosa, perp, sina, pimp);
          ELL_3V_SCALE_ADD3_TT(outVrt->xyzw, float,
                               1, inVrt->xyzw,
                               _tubeRadius, outVrt->norm,
                               shift, tang);
          outVrt->xyzw[3] = 1.0;
          fprintf(stderr, ". %u (%g %g %g)\n", outVertTotalIdx,
                  outVrt->xyzw[0], outVrt->xyzw[1], outVrt->xyzw[2]);
          fprintf(stderr, "indx[%u,%u] = %u %u\n",
                  outIndxIdx, outIndxIdx+1,
                  outVertTotalIdx, outVertTotalIdx + _tubeFacet);
          _lpldTubes->indx[outIndxIdx++] = outVertTotalIdx;
          _lpldTubes->indx[outIndxIdx++] = outVertTotalIdx + _tubeFacet;
          outVertTotalIdx++;
        }
        unsigned int tubeEndIdx = outVertTotalIdx;
        // -------------------------------------- BEGIN final endcap
        if (inVertIdx == _lpldFibers->vcnt[fi]-1) {
          fprintf(stderr, "%s: ---------- BEGIN final endcap\n", me);
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
              outVrt = _lpldTubes->vert + outVertTotalIdx;
              ELL_3V_SCALE_ADD3_TT(outVrt->norm, float,
                                   -cosph, tang,
                                   costh*sinph, perp,
                                   sinth*sinph, pimp);
              ELL_3V_SCALE_ADD3_TT(outVrt->xyzw, float,
                                   1, inVrt->xyzw,
                                   step/2, tang,
                                   _tubeRadius, outVrt->norm);
              fprintf(stderr, ". %u (%g, %g %g)\n", outVertTotalIdx,
                      outVrt->xyzw[0], outVrt->xyzw[1], outVrt->xyzw[2]);
              outVrt->xyzw[3] = 1.0;
              outVertTotalIdx++;
            }
          }
          outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_3V_COPY_TT(outVrt->norm, float, tang);
          ELL_3V_SCALE_ADD3_TT(outVrt->xyzw, float,
                               1, inVrt->xyzw,
                               step/2, tang,
                               _tubeRadius, outVrt->norm);
          fprintf(stderr, "! %u (%g, %g %g)\n", outVertTotalIdx,
                  outVrt->xyzw[0], outVrt->xyzw[1], outVrt->xyzw[2]);
          outVrt->xyzw[3] = 1.0;
          outVertTotalIdx++;
          for (unsigned int ei=0; ei<_endFacet-1; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              fprintf(stderr, "indx[%u,%u] = %u %u\n",
                      outIndxIdx, outIndxIdx+1,
                      (tubeEndIdx + pi
                       + (ei + 0)*_tubeFacet),
                      (tubeEndIdx + pi
                       + (ei + 1)*_tubeFacet));
              _lpldTubes->indx[outIndxIdx++] = (tubeEndIdx + pi
                                                + (ei + 0)*_tubeFacet);
              _lpldTubes->indx[outIndxIdx++] = (tubeEndIdx + pi
                                                + (ei + 1)*_tubeFacet);
            }
          }
          for (unsigned int pi=0; pi<_tubeFacet; pi++) {
            fprintf(stderr, "indx[%u,%u] = %u %u\n",
                    outIndxIdx, outIndxIdx+1,
                    (tubeEndIdx + pi
                     + (_endFacet - 1)*_tubeFacet),
                    (tubeEndIdx
                     + (_endFacet - 0)*_tubeFacet));
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
    fprintf(stderr, "%s: outVertTotalIdx = %u\n", me, outVertTotalIdx);
    fprintf(stderr, "%s: outIndxIdx = %u\n", me, outIndxIdx);
    _flag[flagTubeRadius] = false;
    _flag[flagTubeGeometry] = true;
  }
}

void
HyperStreamline::updateFiberStopColor() {
  char me[]="HyperStreamline::updateFiberStopColor";
  if (_flag[flagFiberColor]
      || _flag[flagFiberStopColorDo]) {
    fprintf(stderr, "!%s: hello\n", me);

    _flag[flagFiberColor] = false;
    _flag[flagFiberStopColorDo] = false;
    _flag[flagFiberStopColor] = true;
  }
}

void
HyperStreamline::updateTubeColor() {
  char me[]="HyperStreamline::updateTubeColor";
  if (_flag[flagTubeAllocated]
      || _flag[flagFiberStopColor]) {
    unsigned int inVertTotalIdx = 0;
    unsigned int outVertTotalIdx = 0;
    limnVrt *inVrt, *outVrt;
    for (unsigned int fi=0; fi<_lpldFibers->primNum; fi++) {
      for (unsigned int inVertIdx=0;
           inVertIdx<_lpldFibers->vcnt[fi];
           inVertIdx++) {
        inVrt = _lpldFibers->vert + _lpldFibers->indx[inVertTotalIdx];
        if (0 == inVertIdx) {
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              outVrt = _lpldTubes->vert + outVertTotalIdx;
              fprintf(stderr, "%s: %u start color\n", me, outVertTotalIdx);
              ELL_4V_COPY(outVrt->rgba, inVrt->rgba);
              outVertTotalIdx++;
            }
          }
        }
        for (unsigned int pi=0; pi<_tubeFacet; pi++) {
          fprintf(stderr, "%s: %u tube\n", me, outVertTotalIdx);
          outVrt = _lpldTubes->vert + outVertTotalIdx;
          ELL_4V_COPY(outVrt->rgba, inVrt->rgba);
          outVertTotalIdx++;
        }
        if (inVertIdx == _lpldFibers->vcnt[fi]-1) {
          for (unsigned int ei=0; ei<_endFacet; ei++) {
            for (unsigned int pi=0; pi<_tubeFacet; pi++) {
              fprintf(stderr, "%s: %u end color\n", me, outVertTotalIdx);
              outVrt = _lpldTubes->vert + outVertTotalIdx;
              ELL_4V_COPY(outVrt->rgba, inVrt->rgba);
              outVertTotalIdx++;
            }
          }
          outVrt = _lpldTubes->vert + outVertTotalIdx;
          fprintf(stderr, "%s: %u end color\n", me, outVertTotalIdx);
          ELL_4V_COPY(outVrt->rgba, inVrt->rgba);
          outVertTotalIdx++;
        }
        inVertTotalIdx++;
      }
    }
    _flag[flagTubeAllocated] = false;
    _flag[flagTubeColor] = true;
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
