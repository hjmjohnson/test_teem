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

#ifndef DEFT_STAR_GLYPH_INCLUDED
#define DEFT_STAR_GLYPH_INCLUDED

#include "Deft.h"
#include "Object.h"
#include "PolyData.h"

namespace Deft {

class DEFT_EXPORT StarGlyph : public PolyData  {
public: 
  explicit StarGlyph();
  ~StarGlyph();

  void parmCopy(StarGlyph *src);

  void baseGlyph(const limnPolyData *bglyph);
  // "rad" = vertex radius, "pos" = 3-vector of glyph center
  void dataSet(unsigned int num,
               const double *radData,
               unsigned int radLen, unsigned int radStride,
               const float *posData, unsigned int posStride,
               const double *anisoData, unsigned int anisoStride,
               const double *errData, unsigned int errStride);

  void anisoThresh(double thresh);
  double anisoThresh() const { return _anisoThresh; }
  void errThresh(double thresh);
  double errThresh() const { return _errThresh; }
  void scale(double scl);
  double scale() const { return _scale; }
  void offsetSet(const float off[3]);
  void offsetGet(float off[3]) const { ELL_3V_COPY(off, _offset); }

  void update();

  // over-rides standard PolyData verticesGet
  unsigned int verticesGet(Nrrd *npos);

private:

  void geomAlloc(void);
  void geomSet(void);

  bool _flag[DEFT_FLAG_NUM_MAX];
  const limnPolyData *_glyphBase;
  double _anisoThresh, _errThresh, _scale;
  unsigned int _pointsNum;
  float _offset[3];
  const double *_radData, *_anisoData, *_errData;
  unsigned int _radLen, _radStride, _posStride, _anisoStride, _errStride;
  const float *_posData;
};

}

#endif /* DEFT_STAR_GLYPH_INCLUDED */
