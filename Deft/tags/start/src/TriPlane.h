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

#ifndef DEFT_TRI_PLANE_INCLUDED
#define DEFT_TRI_PLANE_INCLUDED

#include "Deft.H"
#include "Group.H"
#include "Plane.H"

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
  void sampling(unsigned int axisIdx, float smpl);
  float sampling(unsigned int axisIdx) const;

  // conveniences to *set* all planes' Gage
  void kernel(int which, const NrrdKernelSpec *ksp);
  void colorQuantity(int quantity);
  void alphaMaskQuantity(int quantity);
  void alphaMaskThreshold(double thresh);
  void update();
  
  // conveniences for operating on all planes' Cmap
  void brightness(float);

  // set/get plane position in index space
  void position(unsigned int planeIdx, float);
  float position(unsigned int planeIdx) const;
  
  Plane *plane[3];

  ~TriPlane();
private:
  gageShape *_shape;
  float _origW[3], _interW[3][3], _edgeW[3][3], _posI[3], _sampling[3];
  unsigned int _size[3];  // # samples along each axis
};

} /* namespace Deft */

#endif /* DEFT_TRI_PLANE_INCLUDED */
