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

#ifndef DEFT_TRI_PLANE_INCLUDED
#define DEFT_TRI_PLANE_INCLUDED

#include "Deft.h"
#include "Group.h"
#include "Plane.h"
#include "TensorGlyph.h"
#include "HyperStreamline.h"

namespace Deft {

// this is another experiment in containing vs. inheriting stuff:
// it worked out well for Plane to inherit from PolyData; but in
// this case we want to maintain 3 separate limnPolyDatas, which
// may be clipped or drawing independently.
//
// HEY: what is the best way to avoid manually re-implementing
// all the Object methods by calling them on all contained Objects?

class DEFT_EXPORT TriPlane : public Group {
public: 
  explicit TriPlane(const Volume *vol);

  // exposing const pointer to useful info in gageShape
  const gageShape *shape() const;

  // log_2 of sampling factor for each axis, which means that
  // for plane 0 you have to modify samplines 1 and 2
  void sampling(unsigned int axisIdx, double smpl);
  double sampling(unsigned int axisIdx) const;

  // sampling for seed planes
  void seedSampling(unsigned int axisIdx, double smpl);
  double seedSampling(unsigned int axisIdx) const;

  // conveniences to *set* all planes' (regular and seed) Gage
  void kernel(int which, const NrrdKernelSpec *ksp);
  void colorQuantity(int quantity);
  void alphaMaskQuantity(int quantity);
  void alphaMaskThreshold(double thresh);
  void update();

  void glyphsDo(unsigned int axisIdx, bool doit);
  bool glyphsDo(unsigned int axisIdx) const;
  
  void tractsDo(unsigned int axisIdx, bool doit);
  bool tractsDo(unsigned int axisIdx) const;
  
  // conveniences for operating on all planes' Cmap
  void brightness(float);

  // set/get plane position in index space
  void position(unsigned int planeIdx, float);
  float position(unsigned int planeIdx) const;

  // HEY: this is a symptom of some stupidity
  void glyphsUpdate(unsigned int planeIdx);
  void tractsUpdate(unsigned int planeIdx);

  void seedAnisoThresh(double aniso);
  double seedAnisoThresh() const { return glyph[0]->anisoThresh(); }
  
  Plane *plane[3];       /* the planes we draw */
  Plane *seedPlane[3];   /* the planes used for seed points */
  TensorGlyph *glyph[3];
  HyperStreamline *hsline[3];

  ~TriPlane();
private:
  gageShape *_shape;
  float _origW[3], _interW[3][3], _edgeW[3][3], _posI[3];
  double _sampling[3], _seedSampling[3];
  bool _glyphsDo[3], _tractsDo[3];
  Nrrd *_seedTenFloat[3], *_seedPosFloat[3];
  unsigned int _size[3], _seedSize[3];  // # samples along each axis
};

} /* namespace Deft */

#endif /* DEFT_TRI_PLANE_INCLUDED */
