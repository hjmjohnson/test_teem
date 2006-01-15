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

#ifndef DEFT_GLYPHOLD_INCLUDED
#define DEFT_GLYPHOLD_INCLUDED

#include "Deft.h"

#include <teem/ten.h>

namespace Deft {

class DEFT_EXPORT GlyphOld {
public:
  explicit GlyphOld();
  ~GlyphOld();
  void volumeSet(const Nrrd *nin);
  void anisoSet(int aniso);
  void anisoThreshSet(float anisoThresh);
  void typeSet(int glyphType);
  void resolutionSet(int res);
  void scaleSet(float scale);
  void generate(limnObject *obj);
  Nrrd *naniso;  // so that others can isosurface this; should be private?
private:
  const Nrrd *nten;
  tenGlyphParm *gparm;
  int glyphNum();
  int facePerGlyph();
  int vertPerGlyph();
};

}

#endif /* DEFT_GLYPHOLD_INCLUDED */
