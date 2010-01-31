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

#include "StarGlyph.h"

namespace Deft {

enum {
  flagUnknown,
  flagGlyphBase,
  flagGeomAlloc,
  flagData,
  flagScale,
  flagErrThresh,
  flagOffset,
  flagAnisoThresh,
  flagGeomSet,
  flagLast
};

StarGlyph::StarGlyph() : PolyData(limnPolyDataNew(), true) {
  // char me[]="StarGlyph::StarGlyph";

  // initialize all the flags
  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = false;
  }

  ELL_3V_SET(_offset, 0, 0, 0);
  _scale = 1300; // HEY!
  _errThresh = 30; // HEY!
  _glyphBase = NULL;
}

StarGlyph::~StarGlyph() {

}

void
StarGlyph::baseGlyph(const limnPolyData *bglyph) {
  char me[]="StarGlyph::baseGlyph";

  fprintf(stderr, "!%s: got base glyph\n", me);
  _glyphBase = bglyph;
  _flag[flagGlyphBase] = true;
  return;
}

void
StarGlyph::anisoThresh(double thr) {
  // char me[]="StarGlyph::anisoThreshSet";

  if (_anisoThresh != thr) {
    _anisoThresh = thr;
    _flag[flagAnisoThresh] = true;
  }
}

void
StarGlyph::scale(double scl) {

  _scale = scl;
  _flag[flagScale] = true;
}

void
StarGlyph::errThresh(double thresh) {

  _errThresh = thresh;
  _flag[flagErrThresh] = true;
}

void
StarGlyph::offsetSet(const float off[3]) {

  ELL_3V_COPY(_offset, off);
  _flag[flagOffset] = true;
}

/* HEY: measurement frame has to be identity?? */
void
StarGlyph::dataSet(unsigned int num,
                   const double *radData,
                   unsigned int radLen, unsigned int radStride,
                   const float *posData, unsigned int posStride,
                   const double *anisoData, unsigned int anisoStride,
                   const double *errData, unsigned int errStride) {
  char me[]="StarGlyph::dataSet";

  if (!_glyphBase) {
    fprintf(stderr, "!%s: don't yet have base glyph!\n", me);
    return;
  }
  if (!( radLen == _glyphBase->xyzwNum/2 )) {
    fprintf(stderr, "!%s: (base glyph # vert)/2 %u != radLen %u\n", me,
            _glyphBase->xyzwNum/2, radLen);
    exit(0);
    return;
  }

  _pointsNum = num;
  _radData = radData;
  _radLen = radLen;
  _radStride = radStride;
  _posData = posData;
  _posStride = posStride;
  _anisoData = anisoData;
  _anisoStride = anisoStride;
  _errData = errData;
  _errStride = errStride;

  _flag[flagData] = true;
  return;
}

void
StarGlyph::parmCopy(StarGlyph *src) {
  char me[]="StarGlyph::parmCopy";

  AIR_UNUSED(src);
  fprintf(stderr, "!%s: dummy\n", me);
}

void
StarGlyph::geomAlloc() {
  char me[]="StarGlyph::geomAlloc";

  if (limnPolyDataCopyN(_lpldOwn, _glyphBase, _pointsNum)
      || limnPolyDataAlloc(_lpldOwn, 
                           limnPolyDataInfoBitFlag(_lpldOwn) 
                           | (1 << limnPolyDataInfoRGBA),
                           _lpldOwn->xyzwNum,
                           _lpldOwn->indxNum,
                           _lpldOwn->primNum)) {
    fprintf(stderr, "!%s: problem:\n%s", me, biffGetDone(LIMN));
    return;
  }
  if (0) {
    FILE *file;
    file = fopen("new.lmpd", "wb");
    limnPolyDataWriteLMPD(file, _lpldOwn);
    fclose(file);
  }
  return;
}

void
StarGlyph::geomSet() {
  char me[]="StarGlyph::geomSet";
  unsigned int pidx, ridx;
  const float *pos;
  const double *rad, *aniso, *err;
  float *xyzw;
  unsigned char *rgba;
  
  if (!AIR_EXISTS(_anisoThresh)) {
    fprintf(stderr, "%s: anisoThresh never set!!!\n", me);
  }

  rad = _radData;
  pos = _posData;
  aniso = _anisoData;
  err = _errData;
  xyzw = _lpldOwn->xyzw;
  rgba = _lpldOwn->rgba;

  unsigned char RGBzero[3] = {255,255,255};
  unsigned char RGBlo[3] = {255,  0,255};
  unsigned char RGBhi[3] = {  0,255,  0};
  unsigned char RGB[3];
  for (pidx=0; pidx<_pointsNum; pidx++) {
    if (aniso[0] > _anisoThresh) {
      _lpldOwn->type[pidx] = limnPrimitiveTriangles;
      for (ridx=0; ridx<2*_radLen; ridx++) {
        /*
          ELL_3V_SCALE_ADD3(xyzw, 1.0, _offset, 1.0, pos + 3*pidx,
          _scale*rad[ridx % _radLen], _glyphBase->xyzw + 4*ridx);
        */
        ELL_3V_SCALE_ADD2(xyzw, 1.0, pos,
                          _scale*rad[ridx % _radLen],
                          _glyphBase->xyzw + 4*ridx);
        xyzw[3] = 1;
        xyzw += 4;
        double eee = err[ridx % _radLen];
        if (eee > 0) {
          eee = AIR_MIN(eee, _errThresh);
          ELL_4V_COPY(RGB, RGBhi);
        } else {
          eee = AIR_MIN(-eee, _errThresh);
          ELL_4V_COPY(RGB, RGBlo);
        }
        ELL_3V_LERP(rgba, eee/_errThresh, RGBzero, RGB);
        rgba[3] = 255;
        rgba += 4;
      }
    } else {
      _lpldOwn->type[pidx] = limnPrimitiveNoop;
      xyzw += 4*2*_radLen;
      rgba += 4*2*_radLen;
    }
    rad += _radStride;
    pos += _posStride;
    aniso += _anisoStride;
    err += _errStride;
  }
  limnPolyDataVertexNormals(_lpldOwn);

  if (0) {
    FILE *file;
    file = fopen("all.lmpd", "wb");
    if (limnPolyDataWriteLMPD(file, _lpldOwn)) {
      fprintf(stderr, "!%s: problem:\n%s", me, biffGetDone(LIMN));
    }
    fclose(file);
  }

  return;
}

void
StarGlyph::update() {
  // char me[]="StarGlyph::update";

  if (_flag[flagGlyphBase]
      || _flag[flagData]) {
    this->geomAlloc();
    _flag[flagGlyphBase] = false;
    _flag[flagGeomAlloc] = true;
  }
  if (_flag[flagGeomAlloc]
      || _flag[flagData]
      || _flag[flagScale]
      || _flag[flagErrThresh]
      || _flag[flagOffset]
      || _flag[flagAnisoThresh]) {
    this->geomSet();
    _flag[flagData] = false;
    _flag[flagScale] = false;
    _flag[flagOffset] = false;
    _flag[flagAnisoThresh] = false;
    // fprintf(stderr, "!%s: changed\n", me);
    this->changed();
  }
}

unsigned int
StarGlyph::verticesGet(Nrrd *npos) {
  char me[]="StarGlyph::verticesGet";
  unsigned int pidx, totalNum;
  const float *pos;
  float *posOut;
  const double *aniso;


  totalNum = 0;
  aniso = _anisoData;
  for (pidx=0; pidx<_pointsNum; pidx++) {
    totalNum += (aniso[0] > _anisoThresh);
    aniso += _anisoStride;
  }
  if (totalNum) {
    if (nrrdMaybeAlloc_va(npos, nrrdTypeFloat, 2,
                          AIR_CAST(size_t, 3),
                          AIR_CAST(size_t, totalNum))) {
      char *err = biffGetDone(NRRD);
      fprintf(stderr, "%s: couldn't allocate output:\n%s", me, err);
      free(err); exit(1);
    }
    posOut = AIR_CAST(float *, npos->data);
    pos = _posData;
    aniso = _anisoData;
    for (pidx=0; pidx<_pointsNum; pidx++) {
      if (aniso[0] > _anisoThresh) {
        ELL_3V_COPY(posOut, pos);
        posOut += 3;
      }
      pos += _posStride;
      aniso += _anisoStride;
    }
  }
  return totalNum;
}

// void
// StarGlyph::drawImmediate() {
//   char me[]="StarGlyph::drawImmediate";
  
//   fprintf(stderr, "!%s: dummy\n", me);
// }


} /* namespace Deft */
