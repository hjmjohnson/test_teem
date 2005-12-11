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

#include "TriPlane.h"

namespace Deft {

TriPlane::TriPlane(const Volume *vol) : Group(3) {
  char me[]="TriPlane::TriPlane", *err;

  fprintf(stderr, "%s(%u): kind = %p\n", me, __LINE__, vol->kind());
  _shape = gageShapeNew();
  if (gageShapeSet(_shape, vol->nrrd(), vol->kind()->baseDim)) {
    fprintf(stderr, "%s: trouble with volume:\n%s", me,
            err = biffGetDone(GAGE)); free(err);
    exit(1);
  }

  float tmpW[4], umpW[4], tmpI[4], umpI[4];
  ELL_4V_SET(tmpI, 0.0, 0.0, 0.0, 1.0);
  ELL_4MV_MUL_TT(tmpW, float, _shape->ItoW, tmpI);
  ELL_4V_HOMOG(_origW, tmpW);
  fprintf(stderr, "%s: tmpW=(%g,%g,%g,%g) --> _origW=(%g,%g,%g)\n", me,
          tmpW[0], tmpW[1], tmpW[2], tmpW[3],
          _origW[0], _origW[1], _origW[2]);

  ELL_4V_SET(umpI, 1, 0, 0, 1);
  ELL_4MV_MUL_TT(umpW, float, _shape->ItoW, umpI);
  ELL_4V_HOMOG(umpW, umpW);
  ELL_3V_SUB(_interW[0], umpW, _origW);
  ELL_3V_SCALE(_edgeW[0], _shape->size[0]-1, _interW[0]);

  ELL_4V_SET(umpI, 0, 1, 0, 1);
  ELL_4MV_MUL_TT(umpW, float, _shape->ItoW, umpI);
  ELL_4V_HOMOG(umpW, umpW);
  ELL_3V_SUB(_interW[1], umpW, _origW);
  ELL_3V_SCALE(_edgeW[1], _shape->size[1]-1, _interW[1]);

  ELL_4V_SET(umpI, 0, 0, 1, 1);
  ELL_4MV_MUL_TT(umpW, tmp, _shape->ItoW, umpI);
  ELL_4V_HOMOG(umpW, umpW);
  ELL_3V_SUB(_interW[2], umpW, _origW);
  ELL_3V_SCALE(_edgeW[2], _shape->size[2]-1, _interW[2]);

  _sampling[0] = 0.0; _size[0] = _shape->size[0];
  _sampling[1] = 0.0; _size[1] = _shape->size[1];
  _sampling[2] = 0.0; _size[2] = _shape->size[2];

  object[0] = plane[0] = new Plane(_size[1], _size[2]);
  plane[0]->volume(vol);
  plane[0]->edgeUSet(_edgeW[1]);
  plane[0]->edgeVSet(_edgeW[2]);
  position(0, static_cast<float>(_size[0]/2));

  object[1] = plane[1] = new Plane(_size[0], _size[2]);
  plane[1]->volume(vol);
  plane[1]->edgeUSet(_edgeW[0]);
  plane[1]->edgeVSet(_edgeW[2]);
  position(1, static_cast<float>(_size[1]/2));

  object[2] = plane[2] = new Plane(_size[0], _size[1]);
  plane[2]->volume(vol);
  plane[2]->edgeUSet(_edgeW[0]);
  plane[2]->edgeVSet(_edgeW[1]);
  position(2, static_cast<float>(_size[2]/2));

  plane[0]->lightingUse(false);
  plane[1]->lightingUse(false);
  plane[2]->lightingUse(false);

  plane[0]->brightness(1.2);
  plane[1]->brightness(1.2);
  plane[2]->brightness(1.2);

}

TriPlane::~TriPlane() {
  
}

void
TriPlane::sampling(unsigned int axisIdx, double smpl) {

  axisIdx = AIR_MIN(axisIdx, 2);
  _size[axisIdx] = AIR_CAST(unsigned int, pow(2.0, smpl)*_shape->size[0]);
  if (0 != axisIdx) {
    plane[0]->resolution(_size[1], _size[2]);
    plane[0]->update();
  }
  if (1 != axisIdx) {
    plane[1]->resolution(_size[0], _size[2]);
    plane[1]->update();
  }
  if (2 != axisIdx) {
    plane[2]->resolution(_size[0], _size[1]);
    plane[2]->update();
  }
  _sampling[axisIdx] = smpl;
}

const gageShape *TriPlane::shape() const { return _shape; }

double TriPlane::sampling(unsigned int ai) const { return _sampling[ai]; }

void
TriPlane::position(unsigned int planeIdx, float pos) {
  // char me[]="TriPlane::position";
  float vec[3];

  // fprintf(stderr, "%s(%u, %g): -------------------\n", me, planeIdx, pos);
  planeIdx = AIR_MIN(planeIdx, 2);
  _posI[planeIdx] = pos;
  ELL_3V_SCALE_ADD2(vec, 1.0f, _origW, _posI[planeIdx], _interW[planeIdx]);
  plane[planeIdx]->originSet(vec);
  // HEY: this is lame: we should be able to always do simply
  // plane[planeIdx]->update(), but there's a problem on the
  // first time through ...
  if (plane[planeIdx]->colorQuantity()) {
    // fprintf(stderr, "%s: colorQuantity = %d\n", me, plane[planeIdx]->colorQuantity());
    // do vert update, probe, cmap
    plane[planeIdx]->update();
    // fprintf(stderr, "%s(%u): vert,probe,cmap updated\n", me, planeIdx);
  } else {
    // fprintf(stderr, "%s: simple vert update\n", me);
    // just do vert update
    ((Plane*)plane[planeIdx])->update();
    // fprintf(stderr, "%s(%u): vert updated\n", me, planeIdx);
  }    
}

float TriPlane::position(unsigned int planeIdx) const { 
  return _posI[AIR_MIN(planeIdx, 2)]; 
}

void
TriPlane::kernel(int which, const NrrdKernelSpec *ksp) {
  // char me[]="TriPlane::kernel";

  plane[0]->kernel(which, ksp);
  plane[1]->kernel(which, ksp);
  plane[2]->kernel(which, ksp);
}

void
TriPlane::colorQuantity(int quantity) {

  plane[0]->colorQuantity(quantity);
  plane[1]->colorQuantity(quantity);
  plane[2]->colorQuantity(quantity);
}

void
TriPlane::alphaMaskQuantity(int quantity) {

  plane[0]->alphaMaskQuantity(quantity);
  plane[1]->alphaMaskQuantity(quantity);
  plane[2]->alphaMaskQuantity(quantity);
}

void
TriPlane::alphaMaskThreshold(double thresh) {

  plane[0]->alphaMaskThreshold(thresh);
  plane[1]->alphaMaskThreshold(thresh);
  plane[2]->alphaMaskThreshold(thresh);
}

void
TriPlane::brightness(float br) {

  plane[0]->brightness(br);
  plane[1]->brightness(br);
  plane[2]->brightness(br);
}

void
TriPlane::update() {

  plane[0]->update();
  plane[1]->update();
  plane[2]->update();
}


} /* namespace Deft */
