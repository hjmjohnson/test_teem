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

  _anisoThresh = thr;
  _flag[flagAnisoThresh] = true;
}

void
StarGlyph::scale(double scl) {

  _scale = scl;
  _flag[flagScale] = true;
}

void
StarGlyph::offsetSet(const float off[3]) {

  ELL_3V_COPY(_offset, off);
  _flag[flagOffset] = true;
}

/* HEY: measurement frame has to be identity?? */
void
StarGlyph::dataSet(unsigned int num,
                   const float *radData, unsigned int radLen,
                   unsigned int radStride,
                   const float *posData, unsigned int posStride) {
  char me[]="StarGlyph::dataSet";

  if (!_glyphBase) {
    fprintf(stderr, "!%s: don't yet have base glyph!\n", me);
    return;
  }
  if (!( radLen != _glyphBase->xyzwNum/2 )) {
    fprintf(stderr, "!%s: (base glyph # vert)/2 %u != radLen %u\n", me,
            _glyphBase->xyzwNum/2, radLen);
    return;
  }

  _pointsNum = num;
  _radData = radData;
  _radLen = radLen;
  _radStride = radStride;
  _posData = posData;
  _posStride = posStride;

  _flag[flagData] = true;
  return;
}

void
StarGlyph::parmCopy(StarGlyph *src) {
  char me[]="StarGlyph::parmCopy";

  fprintf(stderr, "!%s: dummy\n", me);
}

void
StarGlyph::geomAlloc() {
  char me[]="StarGlyph::geomAlloc";

  if (limnPolyDataCopyN(_lpldOwn, _glyphBase, _pointsNum)) {
    fprintf(stderr, "!%s: problem:\n%s", me, biffGetDone(LIMN));
    return;
  }
  if (1) {
    FILE *file;
    file = fopen("new.lmpd", "wb");
    limnPolyDataLMPDWrite(file, _lpldOwn);
    fclose(file);
  }
  return;
}

void
StarGlyph::geomSet() {
  char me[]="StarGlyph::geomSet";
  unsigned int pidx, ridx, hlf;
  const float *rad, *pos;
  float *xyzw;

  rad = _radData;
  pos = _posData;
  xyzw = _lpldOwn->xyzw;
  for (pidx=0; pidx<_pointsNum; pidx++) {
    for (hlf=0; hlf<2; hlf++) {
      for (ridx=0; ridx<_radLen; ridx++) {
        ELL_3V_SCALE_ADD3(xyzw, 1.0, _offset, 1.0, pos + 3*pidx,
                          _scale*rad[ridx], _glyphBase->xyzw + 4*ridx);
        xyzw[3] = 1;
        xyzw += 4;
      }
    }
    rad += _radStride;
    pos += _posStride;
  }
  limnPolyDataVertexNormals(_lpldOwn);
  return;
}

void
StarGlyph::update() {
  char me[]="StarGlyph::update";

  if (_flag[flagGlyphBase]
      || _flag[flagData]) {
    this->geomAlloc();
    _flag[flagGlyphBase] = false;
    _flag[flagGeomAlloc] = true;
  }
  if (_flag[flagGeomAlloc]
      || _flag[flagData]
      || _flag[flagScale]
      || _flag[flagOffset]
      || _flag[flagAnisoThresh]) {
    this->geomSet();
    _flag[flagData] = false;
    _flag[flagScale] = false;
    _flag[flagOffset] = false;
    _flag[flagAnisoThresh] = false;
    this->changed();
  }
}

void
StarGlyph::drawImmediate() {
  char me[]="StarGlyph::drawImmediate";
  
  fprintf(stderr, "!%s: dummy\n", me);
}

unsigned int
StarGlyph::verticesGet(Nrrd *npos) {
  char me[]="StarGlyph::verticesGet";
  
  fprintf(stderr, "!%s: dummy\n", me);
  return 0;
}

void
StarGlyph::boundsGet(float min[3], float max[3]) const {
  char me[]="StarGlyph::boundsGet";

  fprintf(stderr, "!%s: dummy\n", me);
  return;
}


} /* namespace Deft */
