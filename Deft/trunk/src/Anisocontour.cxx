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

#include "Anisocontour.h"

namespace Deft {

enum {
  flagUnknown,
  flagSamplings,
  flagKernel,
  flagVolumeRsmp,
  flagAnisoType,
  flagMaskThresh,
  flagAnisoVolume,
  flagIsovalue,
  flagContour,
  flagLast
};

Anisocontour::Anisocontour(const Volume *vol) {
  char me[]="Anisocontour::Anisocontour";

  if (tenGageKind != vol->kind()) {
    // um, HEY!
    fprintf(stderr, "%s: PANIC, got %s kind, not %s\n", me,
            vol->kind()->name, tenGageKind->name);
    exit(1);
  }

  // initialize all the flags
  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = false;
  }

  _volume = vol;
  dynamic_cast<PolyProbe*>(this)->volume(_volume);
  dynamic_cast<PolyProbe*>(this)->color(true);
  _rsmc = nrrdResampleContextNew();
  int E = 0;
  if (!E) E |= nrrdResampleNrrdSet(_rsmc, _volume->nrrd());
  if (!E) E |= nrrdResampleDefaultCenterSet(_rsmc, nrrdCenterCell);
  if (!E) E |= nrrdResampleRangeFullSet(_rsmc, 1);
  if (!E) E |= nrrdResampleRangeFullSet(_rsmc, 2);
  if (!E) E |= nrrdResampleRangeFullSet(_rsmc, 3);
  if (!E) E |= nrrdResampleBoundarySet(_rsmc, nrrdBoundaryBleed);
  if (!E) E |= nrrdResampleRenormalizeSet(_rsmc, AIR_TRUE);
  if (E) {
    fprintf(stderr, "%s: PROBLEM\n%s", me, biffGetDone(NRRD));
  }
  _sampling[0] = _sampling[1] = _sampling[2] = AIR_NAN;
  this->sampling(0, 0.0);
  this->sampling(1, 0.0);
  this->sampling(2, 0.0);
  
  _anisoType = tenAnisoUnknown;
  this->anisoType(tenAniso_FA);
  _maskThresh = 0.0;
  this->maskThresh(0.5);
  _isovalue = AIR_NAN;
  this->isovalue(1.0);

  _sctx = seekContextNew();
  seekNormalsFindSet(_sctx, AIR_TRUE);

  /*
  _gctx = gageContextNew();
  double kparm[3];
  E = 0;
  if (!E) E |= !(_gpvl = gagePerVolumeNew(_gctx, vol->nrrd(), tenGageKind));
  if (!E) E |= gagePerVolumeAttach(_gctx, _gpvl);
  ELL_3V_SET(kparm, 1, 1.0, 0.0);
  if (!E) E |= gageKernelSet(_gctx, gageKernel00, nrrdKernelBCCubic, kparm);
  ELL_3V_SET(kparm, 1, 1.0, 0.0);
  if (!E) E |= gageKernelSet(_gctx, gageKernel11, nrrdKernelBCCubicD, kparm);
  ELL_3V_SET(kparm, 1, 1.0, 0.0);
  if (!E) E |= gageKernelSet(_gctx, gageKernel22, nrrdKernelBCCubicDD, kparm);
  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, tenGageFAGradVec);
  if (!E) E |= gageQueryItemOn(_gctx, _gpvl, tenGageFAHessianEvec2);
  if (!E) E |= gageUpdate(_gctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(GAGE));
  }
  if (!E) E |= seekDataSet(_sctx, NULL, _gctx, 0);
  if (!E) E |= seekTypeSet(_sctx, seekTypeRidgeSurface);
  if (!E) E |= seekItemGradientSet(_sctx, tenGageFAGradVec);
  if (!E) E |= seekItemEigenvectorSet(_sctx, 0,tenGageFAHessianEvec2);
  if (!E) E |= seekUpdate(_sctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGetDone(LIMN));
  }
  */

  _ksp = nrrdKernelSpecNew();
  nrrdKernelSpecParse(_ksp, "tent");
  this->kernel(_ksp);
  _flag[flagKernel] = true;           // HEY Clumsy

  dynamic_cast<PolyProbe*>(this)->brightness(1.0);

  _ntenRsmp = nrrdNew();
  _naniso = nrrdNew();
}

Anisocontour::~Anisocontour() {

  _rsmc = nrrdResampleContextNix(_rsmc);
  _sctx = seekContextNix(_sctx);
  _ksp = nrrdKernelSpecNix(_ksp);
  _ntenRsmp = nrrdNuke(_ntenRsmp);
  _naniso = nrrdNuke(_naniso);
}

void
Anisocontour::sampling(unsigned int axisIdx, double smpl) {

  axisIdx = AIR_MIN(2, axisIdx);
  if (_sampling[axisIdx] != smpl) {
    _sampling[axisIdx] = smpl;
    _flag[flagSamplings] = true;
  }
}

void
Anisocontour::kernel(const NrrdKernelSpec *ksp) {

  // HEY: good example of how container structs complicate
  // knowledge about state change (we're punting)
  if (ksp) {
    _ksp->kernel = ksp->kernel;
    for (unsigned int kpi=0; kpi<NRRD_KERNEL_PARMS_NUM; kpi++) {
      _ksp->parm[kpi] = ksp->parm[kpi];
    }
    _flag[flagKernel] = true;
  }
}

void
Anisocontour::anisoType(int aniso) {

  if (_anisoType != aniso) {
    _anisoType = aniso;
    _flag[flagAnisoType] = true;
  }
}

void
Anisocontour::maskThresh(double thresh) {

  if (_maskThresh != thresh) {
    _maskThresh = thresh;
    _flag[flagMaskThresh] = true;
  }
}

void
Anisocontour::isovalue(double ival) {
  
  if (_isovalue != ival) {
    _isovalue = ival;
    _flag[flagIsovalue] = true;
  }
}

void
Anisocontour::glyphsDo(bool doit) {

}

void
Anisocontour::tractsDo(bool doit) {

}

void
Anisocontour::update() {
  char me[]="Anisocontour::update";
  int E = 0;

  if (_flag[flagSamplings]
      || _flag[flagKernel]) {

    if (_flag[flagSamplings]) {
      for (unsigned int axi=0; axi<3; axi++) {
        size_t size = AIR_CAST(size_t, (pow(2.0, _sampling[axi]) 
                                        * _volume->nrrd()->axis[1+axi].size));
        if (!E) E |= nrrdResampleSamplesSet(_rsmc, 1+axi, size);
        if (!E) E |= nrrdResampleRangeFullSet(_rsmc, 1+axi);
      }
    }
    if (_flag[flagKernel]) {
      if (!E) E |= nrrdResampleKernelSet(_rsmc, 1, _ksp->kernel, _ksp->parm);
      if (!E) E |= nrrdResampleKernelSet(_rsmc, 2, _ksp->kernel, _ksp->parm);
      if (!E) E |= nrrdResampleKernelSet(_rsmc, 3, _ksp->kernel, _ksp->parm);
    }
    if (!E) E |= nrrdResampleExecute(_rsmc, _ntenRsmp);
    if (E) {
      fprintf(stderr, "%s: PROBLEM:\n%s", me, biffGetDone(NRRD));
    }

    // Also have to update kernel of PolyProbe
    dynamic_cast<PolyProbe*>(this)->kernel(gageKernel00, _ksp);
    
    _flag[flagSamplings] = false;
    _flag[flagKernel] = false;
    _flag[flagVolumeRsmp] = true;
  }
  if (_flag[flagAnisoType]
      || _flag[flagMaskThresh]
      || _flag[flagVolumeRsmp]) {

    if (tenAnisoVolume(_naniso, _ntenRsmp, _anisoType, _maskThresh)) {
      fprintf(stderr, "%s: PROBLEM:\n%s", me, biffGetDone(TEN));
    }
    /* */
    if (seekDataSet(_sctx, _naniso, NULL, 0)
        || seekTypeSet(_sctx, seekTypeIsocontour)
        || seekUpdate(_sctx)) {
      fprintf(stderr, "%s: PROBLEM:\n%s", me, biffGetDone(LIMN));
    }
    /* */
    _flag[flagAnisoType] = false;
    _flag[flagMaskThresh] = false;
    _flag[flagVolumeRsmp] = false;
    _flag[flagAnisoVolume] = true;
  }
  if (_flag[flagIsovalue]
      || _flag[flagAnisoVolume]) {

    /* NOTE: the limnPolyDataAlloc() is needed because seekExtract()
       currently does not allocate rgba data, so we have to do it here.
       But we also have to ask for norm data, so that limnPolyDataAlloc()
       doesn't blow it away */
    if (seekIsovalueSet(_sctx, _isovalue)
        || seekUpdate(_sctx)
        || seekExtract(_sctx, _lpldOwn)
        || limnPolyDataAlloc(_lpldOwn,
                             (1 << limnPolyDataInfoRGBA)
                             | (1 << limnPolyDataInfoNorm),
                             _lpldOwn->vertNum,
                             _lpldOwn->indxNum,
                             _lpldOwn->primNum)) {
      fprintf(stderr, "%s: PROBLEM:\n%s", me, biffGetDone(LIMN));
    }

    _flag[flagIsovalue] = false;
    _flag[flagAnisoVolume] = false;
    _flag[flagContour] = true;
  }
  if (_flag[flagContour]) {
    this->changed();
    _flag[flagContour] = false;
  }
}

} /* namespace Deft */
