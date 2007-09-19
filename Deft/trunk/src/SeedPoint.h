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

#ifndef DEFT_SEEDPOINT_INCLUDED
#define DEFT_SEEDPOINT_INCLUDED

#include "Deft.h"
#include "Group.h"
#include "Plane.h"
#include "TensorGlyph.h"
#include "StarGlyph.h"
#include "TensorGlyph.h"
#include "HyperStreamline.h"
#include "Volume.h"

namespace Deft {

class DEFT_EXPORT SeedPoint : public Group {
public:
  explicit SeedPoint(const Volume *vol, const limnPolyData *bglyph);
  ~SeedPoint();

  const limnPolyData *baseGlyph() const { return _bglyph; }

  // exposing const pointer to useful info in gageShape
  const gageShape *shape() const { return _shape; }

  void kernel(int which, const NrrdKernelSpec *ksp);

  void visible(bool doit) { this->cursorShow(doit); }
  bool visible() const { return this->cursorShow(); };

  void cursorShow(bool doit) { _cursor->visible(doit); }
  bool cursorShow() const { return _cursor->visible(); }

  void starGlyphDo(bool doit);
  bool starGlyphDo() const { return _starGlyph->visible(); }
  
  void ten1GlyphDo(bool doit);
  bool ten1GlyphDo() const { return _ten1Glyph->visible(); }
  
  void ten2GlyphDo(bool doit);
  bool ten2GlyphDo() const { return _ten2Glyph->visible(); }
  
  void fiberDo(bool doit);
  bool fiberDo() const { return hsline->visible(); }
  
  void fiberType(int type) { hsline->fiberType(type); }
  int fiberType() const { return hsline->fiberType(); }
  
  void positionSet(double X, double Y, double Z);
  void positionGet(double *pos) const;

  // applies to ten1, ten2, and star
  void scale(double scl);
  const double scale() { return _scale; }
  
  void cursorScale(double scl);
  const double cursorScale() { return _cursScale; }

  void update();

  // HEY this is public so that the main_dwi program can set the 
  // default parameters, and add it to the hslineUI.  Obviously lame
  HyperStreamline *hsline;

  void seedAnisoThresh(double aniso);
  double seedAnisoThresh() const;
  
private:
  gageShape *_shape;
  const Volume *_vol;          // never owned
  const limnPolyData *_bglyph; // never owned
  double _pos[3], _scale, _cursScale;
  PolyData *_cursor;
  PolyProbe *_seedProbe, *_ten1Probe, *_ten2Probe;
  StarGlyph *_starGlyph;
  TensorGlyph *_ten2Glyph, *_ten1Glyph;
  Nrrd *_npos, *_ntmp, *_ntmp1, *_ntmp2;
  float _mframe[9];
  limnPolyData *_seedpld;
};

} /* namespace Deft */

#endif /* DEFT_SEEDPOINT_INCLUDED */
