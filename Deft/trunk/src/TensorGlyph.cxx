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

#include "TensorGlyph.h"

/*
** NOTE: the logic of flags and updates is based entirely on a
** drawing of the dependency graph, on paper, which Gordon used
** during implementation.  Unless you're looking at this paper,
** you might not want to meddle with this logic
*/

namespace Deft {

enum {
  flagUnknown,
  flagPosData,
  flagTenData,
  flagRgbUse,
  flagRgbData,
  flagMaskThresh,
  flagClampEval,
  flagClampEvalUse,
  flagSkipNegEval,
  flagAnisoType,
  flagAnisoCache,
  flagAnisoThreshMin,
  flagMaxNum,
  flagDataAllocated,
  flagDataBasic,
  flagDataSorted,
  flagAnisoThresh,
  flagActiveNum,
  flagRGBEvecParm,
  flagDataRGB,
  flagBaryRes,
  flagDataBaryIdx,
  flagActiveSet,
  flagGlyphRes,
  flagGlyphType,
  flagCleanEdge,
  flagSuperquadSharpness,
  flagGlyphPalette,
  flagGlyphScale,
  flagLast
};

TensorGlyph::TensorGlyph() {
  // char me[]="TensorGlyph::TensorGlyph";

  // initialize all the flags
  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    flag[fi] = false;
  }
  _savingIV = false;

  dynamic(false);
  _cleanEdge = false;
  cleanEdge(true);
  _rgbUse = false;
  rgbUse(true);
  _glyphType = tenGlyphTypeUnknown;
  glyphType(tenGlyphTypeBox);
  _anisoType = tenAnisoUnknown;
  anisoType(tenAniso_FA);
  _glyphRes = 0;
  glyphResolution(10);
  _baryRes = 0;
  barycentricResolution(12);
  _rgbAnisoType = tenAnisoUnknown;
  _rgbEvec = 3;
  _rgbMaxSat = -1;
  _rgbIsoGray = -1;
  _rgbGamma = -1;
  _rgbModulate = -1;
  rgbEvecParmSet(tenAniso_Cl1, 0, 1, 1.0, 1.5, 0.8);
  _glyphScale = -1;
  glyphScale(1.0);
  _anisoThreshMin = -1;
  anisoThreshMin(0.0);
  _anisoThresh = -1;
  anisoThresh(0.5);
  _maskThresh = -1;
  maskThresh(0.5);
  _clampEvalUse = true;
  clampEvalUse(false);
  _clampEval = -1;
  clampEval(0.0);
  _superquadSharpness = -1;
  superquadSharpness(3.0);
  _skipNegEval = true;
  skipNegativeEigenvalues(false);
  
  ELL_3M_IDENTITY_SET(_measrFrame);
  _gshape = NULL;
  _tenData = NULL;
  _posData = NULL;
  _rgbData = NULL;
  _inDataNum = 0;
  _inTenDataStride = 0;
  _inPosDataStride = 0;
  _inRgbDataStride = 0;
  _maxNum = 0;
  _activeNum = 0;
  _glyphsDrawnNum = 0;
  _polygonsPerGlyph = 0;
  _nAnisoCache = nrrdNew();
  _nDataCache = nrrdNew();
  _nList = nrrdNew();
}

void
TensorGlyph::nListEmpty() {
  // char me[]="TensorGlyph::nListEmpty";

  unsigned int *list = (unsigned int *)(_nList->data);
  if (list) {
    unsigned int nn=nrrdElementNumber(_nList);
    for (unsigned int ii=0; ii<nn; ii++) {
      if (list[ii]) {
        glDeleteLists(list[ii], 1);
      }
    }
  }
  return;
}

bool
TensorGlyph::nListIsEmpty() {
  bool ret;

  ret = true;
  unsigned int *list = (unsigned int *)(_nList->data);
  if (list) {
    unsigned int nn=nrrdElementNumber(_nList);
    for (unsigned int ii=0; ii<nn; ii++) {
      ret &= !list[ii];
    }
  }
  return ret;
}

TensorGlyph::~TensorGlyph() {

  _gshape = gageShapeNix(_gshape);
  _tenData = NULL;
  _posData = NULL;
  _rgbData = NULL;
  _nAnisoCache = nrrdNuke(_nAnisoCache);
  _nDataCache = nrrdNuke(_nDataCache);
  nListEmpty();
  _nList = nrrdNuke(_nList);
}

void
TensorGlyph::volumeSet(const Nrrd *nten) {
  char me[]="TensorGlyph::volumeSet", *err;

  if (tenTensorCheck(nten, nrrdTypeFloat, AIR_TRUE, AIR_TRUE)) {
    err = biffGetDone(TEN);
    fprintf(stderr, "%s: problem with given tensor volume:\n%s\n", me, err);
    err = (char*)airFree(err);
    exit(1); // return;
  }

  _gshape = gageShapeNix(_gshape);
  _gshape = gageShapeNew();
  _gshape->defaultCenter = nrrdCenterCell;
  if (gageShapeSet(_gshape, nten, 1)) {
    err = biffGetDone(GAGE);
    fprintf(stderr, "%s: problem setting gageShape:\n%s\n", me, err);
    err = (char*)airFree(err);
    exit(1); return;
  }

  /*
   [0]  [1]  [2]     [0][0]   [1][0]   [2][0]
   [3]  [4]  [5]  =  [0][1]   [1][1]   [2][1]
   [6]  [7]  [8]     [0][2]   [1][2]   [2][2]
  */
  if (AIR_EXISTS(nten->measurementFrame[0][0])) {
    _measrFrame[0] = static_cast<float>(nten->measurementFrame[0][0]);
    _measrFrame[1] = static_cast<float>(nten->measurementFrame[1][0]);
    _measrFrame[2] = static_cast<float>(nten->measurementFrame[2][0]);
    _measrFrame[3] = static_cast<float>(nten->measurementFrame[0][1]);
    _measrFrame[4] = static_cast<float>(nten->measurementFrame[1][1]);
    _measrFrame[5] = static_cast<float>(nten->measurementFrame[2][1]);
    _measrFrame[6] = static_cast<float>(nten->measurementFrame[0][2]);
    _measrFrame[7] = static_cast<float>(nten->measurementFrame[1][2]);
    _measrFrame[8] = static_cast<float>(nten->measurementFrame[2][2]);
  } else {
    ELL_3M_IDENTITY_SET(_measrFrame);
  }
  fprintf(stderr, "!%s: _measrFrame = \n", me);
  ell_3m_print_f(stderr, _measrFrame);
  _tenData = (float*)nten->data;
  _posData = NULL;
  _rgbData = NULL;
  _inDataNum = nrrdElementNumber(nten)/nrrdKindSize(nrrdKind3DMaskedSymMatrix);
  _inTenDataStride = nrrdKindSize(nrrdKind3DMaskedSymMatrix);
  fprintf(stderr, "!%s: _inDataNum = %u, _inTenDataStride = %u\n", me, 
          _inDataNum, _inTenDataStride);
  _inPosDataStride = 0;

  flag[flagPosData] = true;
  flag[flagTenData] = true;
  flag[flagRgbData] = true;
  return;
}

void
TensorGlyph::dataSet(unsigned int num,
                     const float *tenData, unsigned int tenStride,
                     const float *posData, unsigned int posStride,
                     const float *rgbData, unsigned int rgbStride,
                     float measurementFrame[9]) {
  char me[]="TensorGlyph::dataSet";

  if (!(tenData && posData)) {
    fprintf(stderr, "!%s: got NULL pointer (%p,%p)\n", me,
            tenData, posData);
    return;
  }
  if (measurementFrame) {
    ELL_3M_COPY(_measrFrame, measurementFrame);
  } else {
    ELL_3M_IDENTITY_SET(_measrFrame);
  }
  _gshape = gageShapeNix(_gshape);
  _inDataNum = num;
  _tenData = tenData;
  _posData = posData;
  _rgbData = rgbData;
  _inTenDataStride = tenStride;
  _inPosDataStride = posStride;
  _inRgbDataStride = rgbStride;
  flag[flagPosData] = true;
  flag[flagTenData] = true;
  flag[flagRgbData] = true;
  return;
}

void
TensorGlyph::dataSet(unsigned int num,
                     const float *tenData, unsigned int tenStride,
                     const float *posData, unsigned int posStride,
                     float measurementFrame[9]) {

  this->dataSet(num, tenData, tenStride,
                posData, posStride,
                NULL, 0,
                measurementFrame);
}

void
TensorGlyph::dynamic(bool dyn) {
  
  _dynamic = dyn;
  if (_dynamic) {
    compilingUse(false);
  }
}

void
TensorGlyph::anisoType(int aniso) {

  if (_anisoType != aniso) {
    _anisoType = aniso;
    flag[flagAnisoType] = true;
  }
  return;
}

void
TensorGlyph::maskThresh(double maskThr) {

  if (_maskThresh != maskThr) {
    _maskThresh = maskThr;
    flag[flagMaskThresh] = true;
  }
  return;
}

void
TensorGlyph::skipNegativeEigenvalues(bool skip) {

  if (_skipNegEval != skip) {
    _skipNegEval = skip;
    flag[flagSkipNegEval] = true;
  }
  return;
}

void
TensorGlyph::clampEval(double clamp) {

  if (_clampEval != clamp) {
    _clampEval = clamp;
    flag[flagClampEval] = true;
  }
  return;
}

void
TensorGlyph::clampEvalUse(bool clampUse) {

  if (_clampEvalUse != clampUse) {
    _clampEvalUse = clampUse;
    flag[flagClampEvalUse] = true;
  }
  return;
}

void
TensorGlyph::anisoThreshMin(double thrMin) {

  if (_anisoThreshMin != thrMin) {
    _anisoThreshMin = thrMin;
    flag[flagAnisoThreshMin] = true;
  }
  return;
}

void
TensorGlyph::anisoThresh(double thr) {
  char me[]="TensorGlyph::anisoThreshSet";

  if (!_dynamic && thr < _anisoThreshMin) {
    fprintf(stderr, "%s: clamping aniso thresh %g to thresh min %g\n", me,
            thr, _anisoThreshMin);
    thr = _anisoThreshMin;
  }
  if (_anisoThresh != thr) {
    _anisoThresh = thr;
    flag[flagAnisoThresh] = true;
  }
  return;
}

void
TensorGlyph::rgbEvecParmSet(int aniso, unsigned int evec,
                            double maxSat, double isoGray,
                            double gamma, double modulate) {

  if (!( _rgbAnisoType == aniso
         && _rgbEvec == evec
         && _rgbMaxSat == maxSat
         && _rgbIsoGray == isoGray
         && _rgbGamma == gamma
         && _rgbModulate == modulate )) {
    _rgbAnisoType = AIR_CLAMP(tenAnisoUnknown+1, aniso, tenAnisoLast-1);
    _rgbEvec = AIR_MIN(evec, 2);
    _rgbMaxSat = AIR_CLAMP(0.0, maxSat, 1.0);
    _rgbIsoGray = AIR_CLAMP(0.0, isoGray, 1.0);
    _rgbGamma = AIR_MAX(0.000001, gamma);
    _rgbModulate = AIR_CLAMP(0.0, modulate, 1.0);
    flag[flagRGBEvecParm] = true;
  }
  return;
}

void
TensorGlyph::glyphType(int gtype) {

  if (_glyphType != gtype) {
    _glyphType = gtype;
    flag[flagGlyphType] = true;
  }
  return;
}

void
TensorGlyph::superquadSharpness(double sharpness) {

  if (_superquadSharpness != sharpness) {
    _superquadSharpness = sharpness;
    flag[flagSuperquadSharpness] = true;
  }
  return;
}

void
TensorGlyph::glyphResolution(unsigned int res) {

  if (_glyphRes != res) {
    _glyphRes = res;
    _baryEps = 1.0/(10*_baryRes);
    flag[flagGlyphRes] = true;
  }
  return;
}

void
TensorGlyph::barycentricResolution(unsigned int res) {

  if (_baryRes != res) {
    _baryRes = res;
    flag[flagBaryRes] = true;
  }
  return;
}

void
TensorGlyph::cleanEdge(bool clean) {

  if (_cleanEdge != clean) {
    _cleanEdge = clean;
    flag[flagCleanEdge] = true;
  }
  return;
}

void
TensorGlyph::rgbUse(bool rgbu) {

  if (_rgbUse != rgbu) {
    _rgbUse = rgbu;
    flag[flagRgbUse] = true;
  }
  return;
}

void
TensorGlyph::glyphScale(double scale) {

  if (_glyphScale != scale) {
    _glyphScale = scale;
    flag[flagGlyphScale] = true;
  }
  return;
}

void
TensorGlyph::parmCopy(TensorGlyph *src) {

  // this->dynamic(src->dynamic());
  this->anisoType(src->anisoType());
  this->maskThresh(src->maskThresh());
  this->clampEval(src->clampEval());
  this->clampEvalUse(src->clampEvalUse());
  this->skipNegativeEigenvalues(src->skipNegativeEigenvalues());
  this->anisoThreshMin(src->anisoThreshMin());
  this->anisoThresh(src->anisoThresh()); // this is okay for initial set-up
  /*  ???
  void rgbEvecParmSet(int aniso, unsigned int evec,
                      double maxSat, double isoGray,
                      double gamma, double modulate);
  */
  this->glyphType(src->glyphType());
  this->cleanEdge(src->cleanEdge());
  this->rgbUse(src->rgbUse());
  this->superquadSharpness(src->superquadSharpness());
  this->glyphResolution(src->glyphResolution());
  this->barycentricResolution(src->barycentricResolution());
  this->glyphScale(src->glyphScale());
}

void
TensorGlyph::anisoCacheUpdate() {
  // char me[]="TensorGlyph::anisoCacheUpdate";
  float *aniso;
  size_t ii;

  if (flag[flagAnisoType]
      || flag[flagTenData]) {
    if (_dynamic || 3 == _inTenDataStride) {  /* hedge */
      _nAnisoCache = nrrdNuke(_nAnisoCache);
    } else {
      nrrdMaybeAlloc_va(_nAnisoCache, nrrdTypeFloat, 1,
                        AIR_CAST(size_t, _inDataNum));
      aniso = (float *)_nAnisoCache->data;
      /*
      fprintf(stderr, "!%s: computing %s anisotropy (%u tensors) ... ", 
              "TensorGlyph::anisoCacheUpdate", 
              airEnumStr(tenAniso, _anisoType), _inDataNum);
      */
      for (ii=0; ii<_inDataNum; ii++) {
        const float *tenData = _tenData + ii*_inTenDataStride;
        if (tenData[0]) {
          aniso[ii] = tenAnisoTen_f(tenData, _anisoType);
        } else {
          aniso[ii] = 0;
        }
      }
      /*
      fprintf(stderr, "done\n");
      */
    }
    flag[flagAnisoType] = false;
    flag[flagAnisoCache] = true;
  }
  return;
}

void
TensorGlyph::maxNumUpdate() {
  // char me[]="TensorGlyph::maxNumUpdate";
  unsigned int newMaxNum;

  this->anisoCacheUpdate();

  if (flag[flagAnisoThreshMin]
      || flag[flagAnisoCache]
      || flag[flagTenData]
      || flag[flagMaskThresh]
      || flag[flagSkipNegEval]) {
    // fprintf(stderr, "%s: _dynamic = %s, _inTenDataStride = %u\n", me,
    //        _dynamic ? "true" : "false", _inTenDataStride);
    if (_dynamic || 3 == _inTenDataStride) {  /* hedge */
      newMaxNum = _inDataNum;
    } else {
      size_t ii, nn;
      float *aniso;
      const float *tdata;
      float eval[3];
      
      // fprintf(stderr, "!%s: finding new _maxNum = %u\n", me, _maxNum);
      newMaxNum = 0;
      nn = nrrdElementNumber(_nAnisoCache);
      aniso = (float*)_nAnisoCache->data;
      for (ii=0; ii<nn; ii++) {
        tdata = _tenData + ii*_inTenDataStride;
        if (!( tdata[0] >= _maskThresh )) {
          continue;
        }
        if (!( aniso[ii] >= _anisoThreshMin )) {
          continue;
        }
        if (_skipNegEval) {
          tenEigensolve_f(eval, NULL, tdata);
          if (!( eval[2] >= 0 )) {
            continue;
          }
        }
        // else it passes all criteria
        ++newMaxNum;
      }
    }
    flag[flagAnisoThreshMin] = false;
    flag[flagMaskThresh] = false;
    flag[flagSkipNegEval] = false;
    if (newMaxNum != _maxNum) {
      _maxNum = newMaxNum;
      flag[flagMaxNum] = true;
    }
    // fprintf(stderr, "%s: _maxNum = %u, %s\n", me, 
    //        _maxNum, flag[flagMaxNum] ? "CHANGED" : "(unchanged)");
  }
  return;
}

/* 
** layout of data in nDataCache:
** 0        1       2      3 4 5   6 7 8   9 10 11 12   13     14 15 16 (17)
** |        |       |      |       |       |            |      |
** aniso    mask    idx    pos     eval    quat         idx    rgb
**                  data                                bary
*/

#define ANISO_DATA_IDX    0
#define MASK_DATA_IDX     1
#define DATAIDX_DATA_IDX  2
#define POS_DATA_IDX      3
#define EVAL_DATA_IDX     6
#define QUAT_DATA_IDX     9
#define BARYIDX_DATA_IDX  13
#define RGB_DATA_IDX      14
#define DATA_IDX_NUM      17

void
TensorGlyph::dataAllocatedUpdate() {
  char me[]="TensorGlyph::dataAllocatedUpdate", *err;

  this->maxNumUpdate();
  if (flag[flagMaxNum]) {
    if (nrrdMaybeAlloc_va(_nDataCache, nrrdTypeFloat, 2,
                          AIR_CAST(size_t, DATA_IDX_NUM),
                          AIR_CAST(size_t, _maxNum))) {
      err = biffGetDone(NRRD);
      fprintf(stderr, "%s: couldn't allocate raw data:\n%s", me, err);
      free(err); return;
    }
    /*
    fprintf(stderr, "!%s: allocated datacache %u x %u of %s\n", me,
            DATA_IDX_NUM, _maxNum,
            airEnumStr(nrrdType, nrrdTypeFloat));
    */
    flag[flagMaxNum] = false;
    flag[flagDataAllocated] = true;
  }
  return;
}

void
TensorGlyph::dataBasicUpdate() {
  char me[]="TensorGlyph::dataBasicUpdate";

  this->dataAllocatedUpdate();

  if (flag[flagDataAllocated]
      || flag[flagClampEval]
      || flag[flagClampEvalUse]
      || flag[flagAnisoCache]
      || flag[flagTenData]
      || flag[flagPosData]
      || flag[flagRgbUse]
      || flag[flagRgbData]) {

    unsigned int idx, num, nn;
    float eval[3], evec[9], evecT[9], quat[4];
    const float *tenMeasr;
    float *dataCache = (float*)(_nDataCache->data);
    float *aniso = ((_dynamic || 3 == _inTenDataStride)  /* hedge */ 
                    ? NULL
                    : (float*)(_nAnisoCache->data));
    float measrFrameT[9], matMeasr[9], matWorld[9], tenWorld[7];

    ELL_3M_TRANSPOSE(measrFrameT, _measrFrame);
    nn = _inDataNum;
    num = 0;
    if (!_maxNum && nn) {
      fprintf(stderr, "!%s: probably a bug!!\n", me);
      fprintf(stderr, "!%s: _maxNum = %u, nn = %u, dataCache = %p\n",
              me, _maxNum, nn, dataCache);
    } else {
      for (idx=0; idx<nn; idx++) {
        float *dataLine = dataCache + num*DATA_IDX_NUM;
        dataLine[DATAIDX_DATA_IDX] = static_cast<float>(idx);
        if (3 == _inTenDataStride) { /* hedge */
          const float *vec;
          double nvec[3], len, rot[9], zee[3], quat[4];
          vec = _tenData + idx*_inTenDataStride;
          len = ELL_3V_LEN(vec);
          if (len) {
            ELL_3V_SCALE(nvec, 1/len, vec);
          } else {
            ELL_3V_SET(nvec, 0, 0, 1);
          }
          dataLine[ANISO_DATA_IDX] = 1;
          dataLine[MASK_DATA_IDX] = 1;
          ELL_3V_COPY(dataLine + POS_DATA_IDX,
                      _posData + idx*_inPosDataStride);
          ELL_3V_SET(dataLine + EVAL_DATA_IDX,
                     0.07*len, 0.07*len, len/2);
          if (_rgbData) {
            ELL_3V_COPY(dataLine + RGB_DATA_IDX,
                        _rgbData + idx*_inRgbDataStride);
          } else {
            ELL_3V_SET(dataLine + RGB_DATA_IDX, 1, 1, 1);
          }
          ELL_3V_SET(zee, 0, 0, 1);
          ell_3m_rotate_between_d(rot, zee, nvec);
          ell_3m_to_q_d(quat, rot);
          ELL_4V_COPY(dataLine + QUAT_DATA_IDX, quat);
        } else {
          tenMeasr = _tenData + idx*_inTenDataStride;
          if (!_dynamic) {
            if (!( tenMeasr[0] >= _maskThresh )) {
              continue;
            }
            if (!( aniso[idx] >= _anisoThreshMin )) {
              continue;
            }
          }
          TEN_T2M(matMeasr, tenMeasr);
          ell_3m_mul_f(matWorld, _measrFrame, matMeasr);
          ell_3m_mul_f(matWorld, matWorld, measrFrameT);
          tenWorld[0] = tenMeasr[0];
          TEN_M2T(tenWorld, matWorld);
          tenEigensolve_f(eval, evec, tenWorld);
          if (!_dynamic && _skipNegEval && eval[2] < 0) {
            continue;
          }
          if (_dynamic) {
            dataLine[ANISO_DATA_IDX] = tenAnisoEval_f(eval, _anisoType);
          } else {
            dataLine[ANISO_DATA_IDX] = aniso[idx];
          }
          dataLine[MASK_DATA_IDX] = tenWorld[0];
          if (_posData) {
            ELL_3V_COPY(dataLine + POS_DATA_IDX,
                        _posData + idx*_inPosDataStride);
          } else {
            double pI[3], pW[3];
            unsigned int tmp = idx;
            unsigned int d;
            for (d=0; d<=(3)-1; d++) {
              (pI)[d] = tmp % (_gshape->size)[d];
              tmp /= (_gshape->size)[d];
            }
            gageShapeItoW(_gshape, pW, pI);
            ELL_3V_COPY_TT(dataLine + POS_DATA_IDX, float, pW);
          }
          if (_clampEvalUse) {
            ELL_3V_SET_TT(dataLine + EVAL_DATA_IDX, float,
                          AIR_MAX(_clampEval, eval[0]),
                          AIR_MAX(_clampEval, eval[1]),
                          AIR_MAX(_clampEval, eval[2]));
          } else {
            ELL_3V_COPY(dataLine + EVAL_DATA_IDX, eval);
          }
          if (_rgbUse) {
            if (_rgbData) {
              ELL_3V_COPY(dataLine + RGB_DATA_IDX,
                          _rgbData + idx*_inRgbDataStride);
            }
          } else {
            ELL_3V_SET(dataLine + RGB_DATA_IDX, 1, 1, 1);
          }
          ELL_3M_TRANSPOSE(evecT, evec);
          ell_3m_to_q_f(quat, evecT);
          ELL_4V_COPY(dataLine + QUAT_DATA_IDX, quat);
        }
        num++;
      }
    }
    flag[flagDataAllocated] = false;
    flag[flagClampEval] = false;
    flag[flagClampEvalUse] = false;
    flag[flagAnisoCache] = false;
    flag[flagTenData] = false;
    flag[flagPosData] = false;
    flag[flagRgbUse] = false;
    flag[flagRgbData] = false;
    flag[flagDataBasic] = true;
    if (_rgbData) {
      flag[flagDataRGB] = true;
    }
  }
  return;
}

int
_DeftTensorGlyphAnisoCompare(const void *_dataA, const void *_dataB) {
  float A, B;

  A = ((float *)_dataA)[ANISO_DATA_IDX];
  B = ((float *)_dataB)[ANISO_DATA_IDX];
  // I want descending order of anisotropy
  return (A < B
          ? 1
          : (A > B
             ? -1
             : 0));
}

void
TensorGlyph::dataSortedUpdate() {

  this->dataBasicUpdate();

  if (flag[flagDataBasic]) {
    if (_dynamic || 3 == _inTenDataStride) {  /* hedge */
      /* for now a no-op */
    } else {
      qsort(_nDataCache->data, _maxNum, DATA_IDX_NUM*sizeof(float),
            _DeftTensorGlyphAnisoCompare);
    }
    flag[flagDataBasic] = false;
    flag[flagDataSorted] = true;
  }
  return;
}

void
TensorGlyph::activeNumUpdate() {
  // char me[]="TensorGlyph::activeNumUpdate";

  this->dataSortedUpdate();

  if (flag[flagAnisoThresh]
      || flag[flagDataSorted]) {
    if (_dynamic || 3 == _inTenDataStride) { /* hedge */
      _activeNum = _maxNum;
    } else {
      float *dataCache = (float*)(_nDataCache->data);

      // HEY, this should be a binary search!!!
      unsigned int ii;
      for (ii=0; ii<_maxNum; ii++) {
        if ((dataCache + ii*DATA_IDX_NUM)[ANISO_DATA_IDX] < _anisoThresh) {
          break;
        }
      }
      _activeNum = ii;
      /*
      fprintf(stderr, "!%s: aniso[%u] = %g; aniso[%u] = %g; thresh = %g\n", me,
              _activeNum-1,
              (dataCache + (_activeNum-1)*DATA_IDX_NUM)[ANISO_DATA_IDX],
              _activeNum,
              (dataCache + _activeNum*DATA_IDX_NUM)[ANISO_DATA_IDX],
              anisoThresh);
      */
    }
    flag[flagAnisoThresh] = false;
    flag[flagActiveNum] = true;
  }
  return;
}

void
TensorGlyph::dataBaryIdxUpdate() {
  // char me[]="TensorGlyph::dataBaryIdxUpdate";

  this->dataSortedUpdate();

  if (flag[flagBaryRes]
      || flag[flagGlyphType]
      || flag[flagDataSorted]) {
    float *dataCache = (float*)(_nDataCache->data);
    float *eval, cl1, cp1, cl2, cp2;
    unsigned int cpIdx, clIdx, uIdx, vIdx;
    double deval[3], uv[2];

    FILE *fout;
    fout = fopen("uv.txt", "w");

    for (unsigned int ii=0; ii<_maxNum; ii++) {
      float *dataLine = dataCache + ii*DATA_IDX_NUM;
      if (3 == _inTenDataStride) {  /* hedge */
        dataLine[BARYIDX_DATA_IDX] = 0;
      } else {
        eval = dataLine + EVAL_DATA_IDX;
        cp1 = tenAnisoEval_f(eval, tenAniso_Cp1);
        cl1 = tenAnisoEval_f(eval, tenAniso_Cl1);
        cp2 = tenAnisoEval_f(eval, tenAniso_Cp2);
        cl2 = tenAnisoEval_f(eval, tenAniso_Cl2);
        switch(_glyphType) {
        case tenGlyphTypeBox:
          dataLine[BARYIDX_DATA_IDX] = 0;
          break;
        case tenGlyphTypeCylinder:
        case tenGlyphTypeSphere:
          if (cp2 >= cl2) {
            dataLine[BARYIDX_DATA_IDX] = static_cast<float>(1);
          } else {
            dataLine[BARYIDX_DATA_IDX] = static_cast<float>(_baryRes);
          }
          break;
        case tenGlyphTypeSuperquad:
          cpIdx = airIndexClamp(0.0, cp1, 1.0, _baryRes);
          clIdx = airIndexClamp(0.0, cl1, 1.0, _baryRes);
          dataLine[BARYIDX_DATA_IDX] =
            static_cast<float>(cpIdx + _baryRes*clIdx);
          break;
        case tenGlyphTypeBetterquad:
          ELL_3V_COPY(deval, eval);
          tenGlyphBqdUvEval(uv, deval);
          fprintf(fout, "%g %g\n", uv[0], uv[1]);
          /* don't use  "+ _baryEps" on lookup */
          uIdx = airIndexClamp(0.0, uv[0], 1.0, _baryRes);
          vIdx = airIndexClamp(0.0, uv[1], 1.0, _baryRes);
          dataLine[BARYIDX_DATA_IDX] =
            static_cast<float>(uIdx + _baryRes*vIdx);
          break;
        }
      }
    }
    fclose(fout);
    flag[flagBaryRes] = false;
    flag[flagGlyphType] = false;
    flag[flagDataBaryIdx] = true;
  }  
  return;
}

void
TensorGlyph::dataRGBUpdate() {
  // char me[]="TensorGlyph::dataRGBUpdate";

  this->dataSortedUpdate();
  if (flag[flagDataSorted]
      || flag[flagRGBEvecParm]
      || flag[flagRgbUse]) {
    float *dataCache = (float*)(_nDataCache->data);
    float evecPre[3], evec[3];

    if (!_rgbData) {
      ELL_3V_SET(evecPre, 0, 0, 0);
      evecPre[_rgbEvec] = 1.0;
      for (unsigned int ii=0; ii<_maxNum; ii++) {
        float *dataLine = dataCache + ii*DATA_IDX_NUM;
        if (!_rgbUse) {
          ELL_3V_SET_TT(dataLine + RGB_DATA_IDX, float, 1, 1, 1);
          continue;
        }
        float *eval = dataLine + EVAL_DATA_IDX;
        ell_q_3v_rotate_f(evec, dataLine + QUAT_DATA_IDX, evecPre);
        double R = AIR_ABS(evec[0]);
        double G = AIR_ABS(evec[1]);
        double B = AIR_ABS(evec[2]);
        /* desaturate by rgbMaxSat */
        R = AIR_AFFINE(0.0, _rgbMaxSat, 1.0, _rgbIsoGray, R);
        G = AIR_AFFINE(0.0, _rgbMaxSat, 1.0, _rgbIsoGray, G);
        B = AIR_AFFINE(0.0, _rgbMaxSat, 1.0, _rgbIsoGray, B);
        /* desaturate some by rgbAniso */
        double rgbAniso = tenAnisoEval_f(eval, _rgbAnisoType);
        double tmp = AIR_AFFINE(0.0, rgbAniso, 1.0, _rgbIsoGray, R);
        R = AIR_AFFINE(0.0, _rgbModulate, 1.0, R, tmp);
        tmp = AIR_AFFINE(0.0, rgbAniso, 1.0, _rgbIsoGray, G);
        G = AIR_AFFINE(0.0, _rgbModulate, 1.0, G, tmp);
        tmp = AIR_AFFINE(0.0, rgbAniso, 1.0, _rgbIsoGray, B);
        B = AIR_AFFINE(0.0, _rgbModulate, 1.0, B, tmp);
        /* clamp and do gamma */
        R = AIR_CLAMP(0.0, R, 1.0);
        G = AIR_CLAMP(0.0, G, 1.0);
        B = AIR_CLAMP(0.0, B, 1.0);
        R = pow(R, _rgbGamma);
        G = pow(G, _rgbGamma);
        B = pow(B, _rgbGamma);
        ELL_3V_SET_TT(dataLine + RGB_DATA_IDX, float, R, G, B);
      }
    }
    flag[flagDataSorted] = false;
    flag[flagRGBEvecParm] = false;
    flag[flagRgbUse] = false;
    flag[flagDataRGB] = true;
  }
  return;
}

void
TensorGlyph::activeSetUpdate() {

  this->activeNumUpdate();
  this->dataBaryIdxUpdate();
  this->dataRGBUpdate();

  if (flag[flagActiveNum]
      || flag[flagDataBaryIdx]
      || flag[flagDataRGB]) {
    flag[flagActiveNum] = false;
    flag[flagDataBaryIdx] = false;
    flag[flagDataRGB] = false;
    flag[flagActiveSet] = true;
  }
  return;
}

void
baryFind(double bcoord[3], const double uvp[2],
         const double uv0[2],
         const double uv1[2],
         const double uv2[2]) {
  double mat[9], a, a01, a02, a12;
  
  ELL_3M_SET(mat,
             uv0[0], uv0[1], 1,
             uv1[0], uv1[1], 1,
             uvp[0], uvp[1], 1);
  a01 = ELL_3M_DET(mat); a01 = AIR_ABS(a01);
  
  ELL_3M_SET(mat,
             uv0[0], uv0[1], 1,
             uv2[0], uv2[1], 1,
             uvp[0], uvp[1], 1);
  a02 = ELL_3M_DET(mat); a02 = AIR_ABS(a02);
  
  ELL_3M_SET(mat,
             uv1[0], uv1[1], 1,
             uv2[0], uv2[1], 1,
             uvp[0], uvp[1], 1);
  a12 = ELL_3M_DET(mat); a12 = AIR_ABS(a12);

  a = a01 + a02 + a12;
  ELL_3V_SET(bcoord, a12/a, a02/a, a01/a);
  return;
}

void
baryBlend(double abc[3], const double co[3],
          const double abc0[3],
          const double abc1[3],
          const double abc2[3]) {
  unsigned int ii;

  for (ii=0; ii<3; ii++) {
    abc[ii] = co[0]*abc0[ii] + co[1]*abc1[ii] + co[2]*abc2[ii];
  }
  return;
}

void
glyphBqdPaint(limnPolyData *pld, const double uv[2], float gray) {
  double eval[3];
  unsigned int vi, zone, gr;
  int allpos, allneg;

  tenGlyphBqdEvalUv(eval, uv);
  zone = tenGlyphBqdZoneUv(uv);
  if ((allpos = (0 == zone || 1 == zone)) ||
      (allneg = (8 == zone || 9 == zone))) {
    gr = (allpos ? 255 : airIndexClamp(0.0, gray, 1.0, 256));
    for (vi=0; vi<pld->xyzwNum; vi++) {
      ELL_4V_SET(pld->rgba + 4*vi, gr, gr, gr, 255);
    }
  } else {
    double aval[3], sum, ss, tt, svec[3], tvec[3], vv, hh, pp, aa, bb, dd, xx, zz;
    unsigned char cout;
    ELL_3V_ABS(aval, eval);
    sum = aval[0] + aval[1] + aval[2];
    /* project eval onto octahedron */
    ELL_3V_SCALE(eval, 1/sum, eval);
    if (eval[1] > 0) {
      /* y positive */
      ELL_3V_SET(svec, -1.0/sqrt(2), 1.0/sqrt(2), 0);
      ELL_3V_SET(tvec, 1.0/sqrt(2), 1.0/sqrt(2), 0);
      ss = ELL_3V_DOT(eval, svec)*sqrt(2) + 1.0;
      tt = ELL_3V_DOT(eval, tvec)*sqrt(2);
    } else {
      /* y negative */
      ELL_3V_SET(svec, 0, 1.0/sqrt(2), -1.0/sqrt(2));
      ELL_3V_SET(tvec, 0, 1.0/sqrt(2), 1.0/sqrt(2));
      ss = ELL_3V_DOT(eval, svec)*sqrt(2);
      tt = ELL_3V_DOT(eval, tvec)*sqrt(2) + 1;
    }
    ss = AIR_CLAMP(0, ss, 1);
    tt = AIR_CLAMP(0, tt, 1);
    pp = tt;
    if (ss + tt > 1) {
      vv = ss + tt - 1;
      hh = 0;
      aa = vv*vv;
      bb = (pp*pp - vv*vv)/((pp-0.9999)*(pp-0.9999));
      dd = -1;
    } else {
      vv = 0;
      hh = 1 - (ss + tt);
      aa = -hh*hh;
      bb = 1;
      dd = -(1 - hh*hh - 2*pp + pp*pp)/(pp*pp + 0.00001);
    }
    for (vi=0; vi<pld->xyzwNum; vi++) {
      xx = pld->xyzw[0 + 4*vi];
      zz = pld->xyzw[2 + 4*vi];
      cout = (aa + bb*xx*xx + dd*zz*zz > 0
              ? 255
              : airIndexClamp(0.0, gray, 1.0, 256));
      ELL_3V_SET(pld->rgba + 4*vi, cout, cout, cout);
    }
  }
  return;
}

void
TensorGlyph::glyphPaletteUpdate() {
  char me[]="TensorGlyph::glyphPaletteUpdate";
  float ZtoX[16] = {0, 0, 1, 0,
                    0, 1, 0, 0,
                    -1, 0, 0, 0,
                    0, 0, 0, 1};
  float ident[16] = {1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 1, 0,
                     0, 0, 0, 1};
  float *trnsf;
  double abcBall[3]={1,1,1};
  double abcCyli[3]={1,0,0};
  double abcFunk[3]={0,3,2}; // only one with c != b 
  double abcThrn[3]={1,3,3};
  double abcOcta[3]={0,2,2};
  double abcHalf[3]={0.5,0.5,0.5}; /* alpha is half-way between alpha of octa and cone
                                      and beta has to be the same as alpha at for the
                                      seam to be shape-continuous */
  double abcCone[3]={1,2,2};
  
  unsigned int zone,
    vertsZone[10][3] = {{0, 1, 2},   /* 0 */
                        {0, 2, 3},   /* 1 */
                        {1, 3, 4},   /* 2 */
                        {1, 4, 5},   /* 3 */
                        {4, 5, 9},   /* 4 */
                        {1, 5, 6},   /* 5 */
                        {5, 6, 9},   /* 6 */
                        {6, 7, 9},   /* 7 */
                        {7, 8, 10},  /* 8 */
                        {8, 9, 10}}; /* 9 */
  double *abcAll[10][11] = {
    /* zone \ vert 0      1        2        3        4        5        6        7        8        9       10    */
    /*  0 */ {abcBall, abcCyli, abcHalf,  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL   },
    /*  1 */ {abcBall,  NULL,   abcHalf, abcCyli,  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL   },
    /*  2 */ { NULL,   abcOcta,  NULL,   abcCone, abcThrn,  NULL,    NULL,    NULL,    NULL,    NULL,    NULL   },
    /*  3 */ { NULL,   abcOcta,  NULL,    NULL,   abcThrn, abcFunk,  NULL,    NULL,    NULL,    NULL,    NULL   },
    /*  4 */ { NULL,    NULL,    NULL,    NULL,   abcThrn, abcFunk,  NULL,    NULL,    NULL,   abcCone,  NULL   },
    /*  5 */ { NULL,   abcCone,  NULL,    NULL,    NULL,   abcFunk, abcThrn,  NULL,    NULL,    NULL,    NULL   },
    /*  6 */ { NULL,    NULL,    NULL,    NULL,    NULL,   abcFunk, abcThrn,  NULL,    NULL,   abcOcta,  NULL   },
    /*  7 */ { NULL,    NULL,    NULL,    NULL,    NULL,    NULL,   abcThrn, abcCone,  NULL,   abcOcta,  NULL   },
    /*  8 */ { NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,   abcCyli, abcHalf,  NULL,   abcBall },
    /*  9 */ { NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,   abcHalf, abcCyli, abcBall }};
  double uvVert[11][2] = {{1.00, 1.00},   /* 0 */
                          {0.50, 1.00},   /* 1 */
                          {0.75, 0.75},   /* 2 */
                          {1.00, 0.50},   /* 3 */
                          {1.00, 0.00},   /* 4 */
                          {0.50, 0.50},   /* 5 */
                          {0.00, 1.00},   /* 6 */
                          {0.00, 0.50},   /* 7 */
                          {0.25, 0.25},   /* 8 */
                          {0.50, 0.00},   /* 9 */
                          {0.00, 0.00}};  /* 10 */
  double abc[3], uv[2], bcoord[3];
  unsigned int pvi[3];
  
  if (flag[flagBaryRes]
      || flag[flagGlyphRes]
      || flag[flagGlyphType]
      || flag[flagCleanEdge]
      || flag[flagSuperquadSharpness]
      || nListIsEmpty()) { /* this is required because initial palette
                              creation may have been futile if there was no
                              gl drawing context created at the time */

    // create surface and (temporary) actor around it
    limnPolyData *lpld = limnPolyDataNew();
    Deft::PolyData *surf = new Deft::PolyData(lpld, true);
    surf->wireframe(this->wireframe());
    surf->twoSided(this->twoSided());
    surf->colorUse(false);
    surf->normalizeUse(false);
    surf->transformUse(false);

    nListEmpty();
    nrrdMaybeAlloc_va(_nList, nrrdTypeUInt, 2,
                      AIR_CAST(size_t, _baryRes),
                      AIR_CAST(size_t, _baryRes));
    unsigned int *list = (unsigned int *)(_nList->data);

    if (_savingIV && _glyphType != tenGlyphTypeSuperquad) {
      fprintf(stderr, "%s: WARNING: savingIV only works with %s\n", me,
              airEnumStr(tenGlyphType, tenGlyphTypeSuperquad));
    }

    if (3 == _inTenDataStride) {  /* hedge */
      double mat[16];
      limnPolyDataCone(lpld, 1 << limnPolyDataInfoNorm,
                       _glyphRes, AIR_TRUE);
      ELL_4M_TRANSLATE_SET(mat, 0, 0, 1);
      limnPolyDataTransform_d(lpld, mat);
      list[0] = surf->compileDisplayList();
    } else {
      switch(_glyphType) {
      case tenGlyphTypeBox:
        limnPolyDataCube(lpld, 1 << limnPolyDataInfoNorm, _cleanEdge);
        list[0] = surf->compileDisplayList();
        break;
      case tenGlyphTypeCylinder:
      case tenGlyphTypeSphere:
        if (tenGlyphTypeCylinder == _glyphType) {
          limnPolyDataCylinder(lpld, 1 << limnPolyDataInfoNorm,
                               _glyphRes, _cleanEdge);
        } else {
          limnPolyDataSpiralSphere(lpld, 1 << limnPolyDataInfoNorm,
                                   2*_glyphRes, _glyphRes);
        }
        list[1] = surf->compileDisplayList();
        limnPolyDataTransform_f(lpld, ZtoX);
        list[_baryRes] = surf->compileDisplayList();
        break;
      case tenGlyphTypeSuperquad:
        float alpha, beta;
        for (unsigned int cpIdx=0; cpIdx<_baryRes; cpIdx++) {
          double cp = NRRD_CELL_POS(0.0, 1.0, _baryRes, cpIdx);
          for (unsigned int clIdx=0; clIdx<_baryRes; clIdx++) {
            double cl = NRRD_CELL_POS(0.0, 1.0, _baryRes, clIdx);
            if (cpIdx + clIdx > _baryRes-1) {
              continue;
            }
            if (cl > cp) {
              trnsf = ZtoX;
              alpha = AIR_CAST(float, pow(1-cp, _superquadSharpness));
              beta = AIR_CAST(float, pow(1-cl, _superquadSharpness));
            } else {
              trnsf = ident;
              alpha = AIR_CAST(float, pow(1-cl, _superquadSharpness));
              beta = AIR_CAST(float, pow(1-cp, _superquadSharpness));
            }
            limnPolyDataSpiralSuperquadric(lpld,
                                           1 << limnPolyDataInfoNorm,
                                           alpha, beta,
                                           2*_glyphRes, _glyphRes);
            limnPolyDataTransform_f(lpld, trnsf);
            if (_savingIV) {
              FILE *file;
              char filename[128];
              sprintf(filename, "glyph-%03u.iv", cpIdx + _baryRes*clIdx);
              file = fopen(filename, "w");
              if (limnPolyDataWriteIV(file, lpld)) {
                char *err = biffGetDone(LIMN);
                fprintf(stderr, "%s: couldn't save %s:\n%s", me, filename, err);
                free(err);
              }
              fclose(file);
            }
            list[cpIdx + _baryRes*clIdx] = surf->compileDisplayList();
            /*
              fprintf(stderr, "!%s: list[cpIdx + _baryRes*clIdx] = "
              "list[%u + _%u*%u] = list[%u] = %u\n", me,
              cpIdx, _baryRes, clIdx, cpIdx + _baryRes*clIdx, 
              list[cpIdx + _baryRes*clIdx]);
            */
          }
        }
        break;
      case tenGlyphTypeBetterquad:
        surf->colorUse(true);
        for (unsigned int vIdx=0; vIdx<_baryRes; vIdx++) {
          uv[1] = NRRD_CELL_POS(0.0, 1.0, _baryRes, vIdx);
          for (unsigned int uIdx=0; uIdx<_baryRes; uIdx++) {
            uv[0] = NRRD_CELL_POS(0.0 + _baryEps, 1.0 + _baryEps, _baryRes, uIdx);
            zone = tenGlyphBqdZoneUv(uv);
            if (0 == zone || 5 == zone || 6 == zone || 7 == zone || 8 == zone) {
              trnsf = ZtoX;
            } else {              
              trnsf = ident;
            }
            ELL_3V_COPY(pvi, vertsZone[zone]);
            baryFind(bcoord, uv, uvVert[pvi[0]], uvVert[pvi[1]], uvVert[pvi[2]]);
            baryBlend(abc, bcoord,
                      abcAll[zone][pvi[0]],
                      abcAll[zone][pvi[1]],
                      abcAll[zone][pvi[2]]);
            abc[0] = pow(abc[0], _superquadSharpness);
            for (unsigned int bci=1; bci<=2; bci++) {
              if (abc[bci] < 1) {
                abc[bci] = pow(abc[bci], _superquadSharpness);
              } else if (abc[bci] > 2) {
                abc[bci] = (abc[bci] - 2)*_superquadSharpness + 2;
              }
            }
            limnPolyDataSpiralBetterquadric(lpld,
                                            (1 << limnPolyDataInfoNorm
                                             | 1 << limnPolyDataInfoRGBA),
                                            abc[0], abc[1], abc[2], 0.0,
                                            2*_glyphRes, _glyphRes);
            limnPolyDataTransform_f(lpld, trnsf);
            limnPolyDataVertexNormals(lpld);
            glyphBqdPaint(lpld, uv, 0.2);
            surf->wireframe(true);
            list[uIdx + _baryRes*vIdx] = surf->compileDisplayList();
          }
        }
        break;
      } /* switch */
    }
    // learn number of polygons per glyph, (NOTE) assuming that its the 
    // same for all entries of the palette
    _polygonsPerGlyph = limnPolyDataPolygonNumber(lpld);

    // lose the actor
    delete surf;
    
    flag[flagGlyphRes] = false;
    flag[flagCleanEdge] = false;
    flag[flagSuperquadSharpness] = false;
    flag[flagGlyphPalette] = true;
  }
  return;
}

void
TensorGlyph::update() {
  char me[]="TensorGlyph::update";

  if (!_tenData) {
    fprintf(stderr, "%s: never got data!\n", me);
    // exit(1); 
    return;
  }
 
  // this will recursively the rest of the updates
  // double time0 = airTime();
  this->glyphPaletteUpdate();
  this->activeSetUpdate();
  // double time1 = airTime();

  if (flag[flagActiveSet] 
      || flag[flagGlyphPalette]) {
    // fprintf(stderr, "%s: permits %g fps\n", me, 1.0/(time1 - time0));
  }

  if (flag[flagGlyphScale]
      || flag[flagGlyphPalette]
      || flag[flagActiveSet]) {
    this->changed();
    flag[flagGlyphScale] = false;
    flag[flagGlyphPalette] = false;
    flag[flagActiveSet] = false;
  }

  return;
}

void
TensorGlyph::drawImmediate() {
  // char me[]="TensorGlyph::drawImmediate";
  unsigned int *list, baryIdx;
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  
  _glyphsDrawnNum = 0;
  if (!( _nList && _nList->data && _nDataCache && _nDataCache->data )) {
    return;
  }

  if (nListIsEmpty()) {
    // the initial call to glyphPaletteUpdate() may have been for naught,
    // if there was no working GL context at the time.  This fixes that.
    this->update();
  }
  list = (unsigned int *)(_nList->data);
  if (_normalizeUse) {
    glEnable(GL_NORMALIZE);
  }
  if (_colorUse) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  } else {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
  }
  float glyphScale = static_cast<float>(_glyphScale);
  FILE *ivFile = NULL;
  if (_savingIV) {
    ivFile = fopen("datalist.txt", "w");
  }
  for (unsigned int ii=0; ii<_activeNum; ii++) {
    float *dataLine = (float*)(_nDataCache->data) + ii*DATA_IDX_NUM;
    float *rgb = dataLine + RGB_DATA_IDX;
    float *eval = dataLine + EVAL_DATA_IDX;
    float *pos = dataLine + POS_DATA_IDX;
    float *quat = dataLine + QUAT_DATA_IDX;
    if (_dynamic || 3 == _inTenDataStride) { /* hedge */
      if (!( dataLine[MASK_DATA_IDX] >= _maskThresh )) {
        continue;
      }
      if (!( dataLine[ANISO_DATA_IDX] >= _anisoThresh )) {
        continue;
      }
      if (_skipNegEval && eval[2] < 0) {
        continue;
      }
    }
    float axis[3];
    float angle = ell_q_to_aa_f(axis, quat);  // not currently a bottle-neck
    baryIdx = (unsigned int)(dataLine[BARYIDX_DATA_IDX]);

    if (_colorUse) {
      glColor3fv(rgb);
    }
    glPushMatrix();
    /*
    fprintf(stderr, "!%s[%u]: pos(%g,%g,%g) rot(%g,%g,%g,%g) scl(%g,%g,%g)\n",
            "bingo",  ii,
            pos[0], pos[1], pos[2],
            AIR_CAST(float, 180*angle/AIR_PI), axis[0], axis[1], axis[2],
            glyphScale*eval[0], glyphScale*eval[1], glyphScale*eval[2]);
    */
    glTranslatef(pos[0], pos[1], pos[2]);
    glRotatef(AIR_CAST(float, 180*angle/AIR_PI), axis[0], axis[1], axis[2]);
    if (tenGlyphTypeBetterquad == _glyphType) {

      glScalef(glyphScale*AIR_ABS(eval[0]),
               glyphScale*AIR_ABS(eval[1]),
               glyphScale*AIR_ABS(eval[2]));
      
      /* hack; don't scale by eigenvalues
      glScalef(glyphScale,
               glyphScale,
               glyphScale);
      */
    } else {
      glScalef(glyphScale*eval[0], glyphScale*eval[1], glyphScale*eval[2]);
    }
    /*
    fprintf(stderr, "!%s: glCallList(list[%u] = %u)\n", me,
	    baryIdx, list[baryIdx]);
    */
    glCallList(list[baryIdx]);
    if (_savingIV) {
      fprintf(ivFile, "%g %g %g   %g %g %g   %g %g %g %g   %g %g %g    %u\n", 
              rgb[0], rgb[1], rgb[2],
              pos[0], pos[1], pos[2],
              AIR_CAST(float, 180*angle/AIR_PI), axis[0], axis[1], axis[2],
              glyphScale*eval[0], glyphScale*eval[1], glyphScale*eval[2],
              baryIdx);
    }
    glPopMatrix();
    _glyphsDrawnNum++;
  }
  if (_colorUse) {
    glDisable(GL_COLOR_MATERIAL);
  }
  if (_normalizeUse) {
    glDisable(GL_NORMALIZE);
  }
  if (_savingIV) {
    fclose(ivFile);
  }

  flag[flagGlyphScale] = false;
  flag[flagGlyphPalette] = false;
  flag[flagActiveSet] = false;
  /*
  fprintf(stderr, "!%s: done (_colorUse = %s)\n", me, 
          _colorUse ? "true" : "false");
  */
}

unsigned int
TensorGlyph::verticesGet(Nrrd *npos) {
  char me[]="TensorGlyph::verticesGet", *err;
  
  if (0 == _activeNum) {
    return 0;
  }
  if (nrrdMaybeAlloc_va(npos, nrrdTypeFloat, 2,
                        AIR_CAST(size_t, 3),
                        AIR_CAST(size_t, _activeNum))) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: couldn't allocate output:\n%s", me, err);
    free(err); exit(1);
  }
  float *pos = static_cast<float *>(npos->data);
  for (unsigned int ii=0; ii<_activeNum; ii++) {
    float *dataLine = static_cast<float*>(_nDataCache->data) + ii*DATA_IDX_NUM;
    ELL_3V_COPY(pos + 3*ii, dataLine + POS_DATA_IDX);
  }
  return _activeNum;
}

void
TensorGlyph::boundsGet(float min[3], float max[3]) const {
  // char me[]="TensorGlyph::boundsGet";

  ELL_3V_SET(min, 0, 0, 0);
  ELL_3V_SET(max, 0, 0, 0);

  for (unsigned int ii=0; ii<_activeNum; ii++) {
    float *dataLine = (float*)(_nDataCache->data) + ii*DATA_IDX_NUM;
    float *pos = dataLine + POS_DATA_IDX;

    if (!ii) {
      ELL_3V_COPY(min, pos);
      ELL_3V_COPY(max, pos);
    } else {
      ELL_3V_MIN(min, min, pos);
      ELL_3V_MAX(max, max, pos);
    }
  }
  /*
  fprintf(stderr, "!%s: min=(%g,%g,%g), max=(%g,%g,%g)\n", me, 
          min[0], min[1], min[2], max[0], max[1], max[2]);
  */
}

void
TensorGlyph::saveInventor() {
  
  unsigned int bres = this->barycentricResolution();
  this->barycentricResolution(bres+1);
  _savingIV = true;
  this->barycentricResolution(bres);
  this->update();
  this->drawImmediate();
  _savingIV = false;
  return;
}

} /* namespace Deft */

