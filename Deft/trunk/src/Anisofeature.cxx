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

#include "Anisofeature.h"

#define ERROR_CHECK(key) \
  if (E) { \
    char *err; \
    fprintf(stderr, "!%s: ******* \n!%s: problem:\n%s\n", me, me, \
            err = biffGetDone(key)); \
    free(err); \
  }

namespace Deft {

enum {
  flagUnknown,
  flagFeatureParm,
  flagLpldWhole,
  flagCCParm,
  flagLast
};

Anisofeature::Anisofeature(const Volume *) {
  char me[]="Anisofeature::Anisofeature";

  fprintf(stderr, "!%s: don't use me, bye\n", me);
  exit(0);
}

// make this separate as part of debugging, could be back in the .h file
double
Anisofeature::sampling() const {

  return _sampling; 
}

Anisofeature::Anisofeature(const Volume *vol, const limnPolyData *lpld) {
  char me[]="Anisofeature::Anisofeature";
  int E;

  if (tenGageKind != vol->kind()) {
    // um, HEY!
    fprintf(stderr, "%s: PANIC, got %s kind, not %s\n", me,
            vol->kind()->name, tenGageKind->name);
    exit(1);
  }

  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = false;
  }

  if (lpld) {
    _hack = true;
  } else {
    _hack = false;
  }
  _volume = vol;
  dynamic_cast<PolyProbe*>(this)->volume(_volume);
  dynamic_cast<PolyProbe*>(this)->color(true);

  _glyphsDo = false;
  _tractsDo = false;

  _ksp00 = nrrdKernelSpecNew();
  _ksp11 = nrrdKernelSpecNew();
  _ksp22 = nrrdKernelSpecNew();

  _gctx = gageContextNew();
  E = 0;
  if (!E) E |= !(_gpvl = gagePerVolumeNew(_gctx, vol->nrrd(), tenGageKind));
  if (!E) E |= gagePerVolumeAttach(_gctx, _gpvl);
  // just to have some item prior to update test
  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, tenGageTensor);
  _scale = AIR_NAN;
  this->scale(1.0);
  if (!E) E |= gageUpdate(_gctx);
  ERROR_CHECK(GAGE);

  _sctx = seekContextNew();
  _lpldWhole = limnPolyDataNew();
  _lpldOrig = _lpldOwn;
  _lpldCC = limnPolyDataNew();
  _lpldSelect = limnPolyDataNew();
  if (_hack) {
    _flag[flagLpldWhole] = true;
    limnPolyDataCopy(_lpldWhole, lpld);
  } else {
    seekVerboseSet(_sctx, 10);
    if (!E) E |= seekDataSet(_sctx, NULL, _gctx, 0);
    _sampling = AIR_NAN;
    this->sampling(0.0);
    this->strengthUse(true);
    this->strength(+1, 0.0);
    ERROR_CHECK(SEEK);
  }

  this->alphaMask(true);
  this->evecRgbBgGray(0.0);
  this->evecRgbIsoGray(0.4);
  this->evecRgbMaxSat(0.95);

  this->ccDo(true);
  this->ccSingle(false);
  this->ccId(0);

  dynamic_cast<PolyProbe*>(this)->brightness(1.0);
  this->twoSided(true);
  fprintf(stderr, "!%s: returning\n", me);
}

Anisofeature::~Anisofeature() {

  limnPolyDataNix(_lpldWhole);
  limnPolyDataNix(_lpldCC);
  limnPolyDataNix(_lpldSelect);
  _lpldOwn = _lpldOrig;
  _ksp00 = nrrdKernelSpecNix(_ksp00);
  _ksp11 = nrrdKernelSpecNix(_ksp11);
  _ksp22 = nrrdKernelSpecNix(_ksp22);
  _sctx = seekContextNix(_sctx);
  _gctx = gageContextNix(_gctx);
}

void
Anisofeature::sampling(double smpl) {
  char me[]="Anisofeature::sampling";
  size_t samplesOut[3];
  int E=0;

  // no dependency problems because the volume can't change
  if (_sampling != smpl) {
    double mgsp; // mean gage spacing

    _sampling = smpl;
    mgsp = (_gctx->shape->spacing[0]
            + _gctx->shape->spacing[1]
            + _gctx->shape->spacing[2])/3;
    for (unsigned int axi=0; axi<3; axi++) {
      unsigned int baxi = _volume->kind()->baseDim + axi;
      size_t samplesIn = _volume->nrrd()->axis[baxi].size;
      samplesOut[axi] = static_cast<size_t>(pow(2.0, _sampling)
                                            * (_gctx->shape->spacing[0])
                                            * samplesIn
                                            / mgsp);
    }
    fprintf(stderr, "!%s: smpl = %g --> samples(%u,%u,%u)\n", me, smpl,
            AIR_CAST(unsigned int, samplesOut[0]),
            AIR_CAST(unsigned int, samplesOut[1]),
            AIR_CAST(unsigned int, samplesOut[2]));
    if (!E) E |= seekSamplesSet(_sctx, samplesOut);
    ERROR_CHECK(SEEK);
    _flag[flagFeatureParm] = true;
  }
}

void
Anisofeature::scale(double scl) {
  char me[]="Anisofeature::scale";
  double kparm[3];
  int E=0;

  fprintf(stderr, "!%s: scl = %g\n", me, scl);

  _scale = scl;
  ELL_3V_SET(kparm, scl, 1.0, 0.0);

  nrrdKernelSpecSet(_ksp00, nrrdKernelBCCubic, kparm);
  nrrdKernelSpecSet(_ksp11, nrrdKernelBCCubicD, kparm);
  nrrdKernelSpecSet(_ksp22, nrrdKernelBCCubicDD, kparm);

  if (!E) E |= gageKernelSet(_gctx, gageKernel00, 
                             _ksp00->kernel, _ksp00->parm);
  if (!E) E |= gageKernelSet(_gctx, gageKernel11,
                             _ksp11->kernel, _ksp11->parm);
  if (!E) E |= gageKernelSet(_gctx, gageKernel22, 
                             _ksp22->kernel, _ksp22->parm);
  if (!E) E |= gageUpdate(_gctx);
  ERROR_CHECK(GAGE);
  dynamic_cast<PolyProbe*>(this)->kernel(gageKernel00, _ksp00);
  dynamic_cast<PolyProbe*>(this)->kernel(gageKernel11, _ksp11);
  dynamic_cast<PolyProbe*>(this)->kernel(gageKernel22, _ksp22);

  _flag[flagFeatureParm] = true;
}

void
Anisofeature::type(int type) {
  char me[]="Anisofeature::type";
  int E=0;

  if (!E) E |= gageQueryReset(_gctx, _gpvl);
  ERROR_CHECK(GAGE);
  if (!_hack) {
    if (!E) E |= seekTypeSet(_sctx, type);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::itemScalar(int item) {
  char me[]="Anisofeature::itemScalar";
  int E=0;

  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, item);
  ERROR_CHECK(GAGE);
  if (!_hack) {
    if (!E) E |= seekItemScalarSet(_sctx, item);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::itemStrength(int item) {
  char me[]="Anisofeature::itemStrength";
  int E=0;

  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, item);
  ERROR_CHECK(GAGE);
  if (!_hack) {
    if (!E) E |= seekItemStrengthSet(_sctx, item);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::itemNormal(int item) {
  char me[]="Anisofeature::itemNormal";
  int E=0;

  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, item);
  ERROR_CHECK(GAGE);
  if (!_hack) {
    if (!E) E |= seekItemNormalSet(_sctx, item);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::itemGradient(int item) {
  char me[]="Anisofeature::itemGradient";
  int E=0;

  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, item);
  ERROR_CHECK(GAGE);
  if (!_hack) {
    if (!E) E |= seekItemGradientSet(_sctx, item);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::itemEigensystem(int evalItem, int evecItem) {
  char me[]="Anisofeature::itemEigensystem";
  int E=0;

  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, evalItem);
  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, evecItem);
  ERROR_CHECK(GAGE);
  if (!_hack) {
    if (!E) E |= seekItemEigensystemSet(_sctx, evalItem, evecItem);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::strengthUse(bool doit) {
  char me[]="Anisofeature::strengthUse";

  if (!_hack) {
    int E=0;
    if (!E) E |= seekStrengthUseSet(_sctx, doit ? AIR_TRUE : AIR_FALSE);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::strength(int sign, double strength) {
  char me[]="  Anisofeature::strength";

  if (!_hack) {
    int E=0;
    if (!E) E |= seekStrengthSet(_sctx, sign, strength);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::isovalue(double ival) {
  char me[]="Anisofeature::isovalue";

  if (!_hack) {
    int E=0;
    if (!E) E |= seekIsovalueSet(_sctx, ival);
    ERROR_CHECK(SEEK);
  }
  _flag[flagFeatureParm] = true;
}

void
Anisofeature::glyphsDo(bool doit) {
  AIR_UNUSED(doit);
}

void
Anisofeature::tractsDo(bool doit) {
  AIR_UNUSED(doit);
}

void
Anisofeature::ccDo(bool doit) {

  if (_ccDo != doit) {
    _ccDo = doit;
    _flag[flagCCParm] = true;
  }
}

void
Anisofeature::ccSingle(bool doit) {

  if (_ccSingle != doit) {
    _ccSingle = doit;
    _flag[flagCCParm] = true;
  }
}

void
Anisofeature::ccId(unsigned int id) {

  if (_ccId != id) {
    _ccId = id;
    _flag[flagCCParm] = true;
  }
}

unsigned int
Anisofeature::ccNum() const {

  if (_ccDo) {
    return _lpldCC->primNum;
  } else {
    return 1;
  }
}

void
Anisofeature::update() {
  char me[]="Anisofeature::update";
  int E=0;
  
  fprintf(stderr, "!%s: hello (hack %s, whole %s)\n", me,
          _hack ? "true" : "false",
          _flag[flagLpldWhole] ? "true" : "false");
  if (_flag[flagFeatureParm]) {
    if (!E) E |= gageUpdate(_gctx);
    ERROR_CHECK(GAGE);
    if (!_hack) {
      if (!E) E |= seekUpdate(_sctx);
      if (!E) E |= seekExtract(_sctx, _lpldWhole);
      ERROR_CHECK(SEEK);
      if (_sctx->strengthUse) {
        fprintf(stderr, "!%s: max strength seen = %g\n", me,
                _sctx->strengthSeenMax);
      }
    }
    _flag[flagFeatureParm] = false;
    _flag[flagLpldWhole] = true;
  }
  
  if (_flag[flagLpldWhole]
      || _flag[flagCCParm]) {
    if (_ccDo) {

      Nrrd *nmeas = nrrdNew();
      
      if (_flag[flagLpldWhole]) {
        if (!E) E |= limnPolyDataCopy(_lpldCC, _lpldWhole);
        if (!E) E |= limnPolyDataCCFind(_lpldCC);
        if (!E) E |= limnPolyDataPrimitiveArea(nmeas, _lpldCC);
        if (!E) E |= limnPolyDataPrimitiveSort(_lpldCC, nmeas);
        ERROR_CHECK(LIMN);
      }
      Nrrd *nmask = nrrdNew();
      if (!E) E |= nrrdConvert(nmask, nmeas, nrrdTypeDouble);
      ERROR_CHECK(NRRD);
      double *mask = static_cast<double *>(nmask->data);
      for (unsigned int pi=0; pi<nmask->axis[0].size; pi++) {
        mask[pi] = 0;
      }
      if (_ccSingle) {
        if (_ccId < nmask->axis[0].size) {
          mask[_ccId] = 1;
        }
      } else {
        unsigned int piTop = AIR_MIN(nmask->axis[0].size, _ccId+1);
        for (unsigned int pi=0; pi<piTop; pi++) {
          mask[pi] = 1;
        }
      }
      if (!E) E |= limnPolyDataCopy(_lpldSelect, _lpldCC);
      if (!E) E |= limnPolyDataPrimitiveSelect(_lpldSelect, _lpldCC, nmask);
      ERROR_CHECK(LIMN);
      nrrdNuke(nmeas);
      nrrdNuke(nmask);
      _lpldOwn = _lpldSelect;
    } else {
      fprintf(stderr, "%s: setting _lpldOwn = %p\n", me, _lpldOwn);
      _lpldOwn = _lpldWhole;
    }
  }
  
  // needed for colormapping 
  if (!E) E |= limnPolyDataAlloc(_lpldOwn,
                                 (limnPolyDataInfoBitFlag(_lpldOwn)
                                  | (1 << limnPolyDataInfoRGBA)),
                                 _lpldOwn->xyzwNum,
                                 _lpldOwn->indxNum,
                                 _lpldOwn->primNum);
  ERROR_CHECK(LIMN);
  if (!E) E |= limnPolyDataVertexWindingFix(_lpldOwn, AIR_TRUE);
  if (!E) E |= limnPolyDataVertexNormals(_lpldOwn);
  ERROR_CHECK(LIMN);
  this->changed();
}

} /* namespace Deft */
