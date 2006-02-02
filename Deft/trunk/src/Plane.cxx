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

#include "Plane.h"

enum {
  flagUnknown,
  flagResolution,
  flagOrigin,
  flagEdge,
  flagLast
};

namespace Deft {

Plane::Plane(unsigned int resU, unsigned int resV) { // sets _lpldOwn

  for (unsigned int fi=0; fi<DEFT_PLANE_FLAG_NUM; fi++) {
    _flag[fi] = false;
  }

  // bogus initialization to avoid unitialized value (valgrind) warnings
  _resolutionU = 0;
  _resolutionV = 0;
  ELL_3V_SET(_origin, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(_edgeU, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(_edgeV, AIR_NAN, AIR_NAN, AIR_NAN);

  resolution(resU, resV);
  origin(-1, -1, 0);
  edgeU(2, 0, 0);
  edgeV(0, 2, 0);
  _twoSided = true;
}

Plane::~Plane() {
  
}

void
Plane::resolution(unsigned int resU, unsigned int resV) {

  if (_resolutionU != resU) {
    _resolutionU = AIR_MAX(2, resU);
    _flag[flagResolution] = true;
  }
  if (_resolutionV != resV) {
    _resolutionV = AIR_MAX(2, resV);
    _flag[flagResolution] = true;
  }
}

unsigned int
Plane::resolutionU() {
  
  return _resolutionU;
}

unsigned int
Plane::resolutionV() {
  
  return _resolutionV;
}

void
Plane::origin(float X, float Y, float Z) {

  /*
  ** This, ultimately was the trick to make the following sequence work:
  ** 1) turn on probe plane
  ** 2) turn off plane
  ** 3) move plane
  ** 4) turn on plane
  ** The problem was that vertex colors were wrong, because probing had
  ** not been done with the move (because its faster to not do the 
  ** probing/coloring when the plane is not visible).  So, the solution
  ** was to restate the slice position when its made visible, to trigger
  ** probe and color, but that only works when flagOrigin is always turned
  ** on, without the cleverness that detects when the origin is unchanged.
  if (_origin[0] != X
      || _origin[1] != Y
      || _origin[2] != Z) {
  */
  ELL_3V_SET(_origin, X, Y, Z);
  _flag[flagOrigin] = true;
  /*
  }
  */
}

void
Plane::originSet(const float vec[3]) {

  origin(vec[0], vec[1], vec[2]);
  _flag[flagOrigin] = true;
}

void
Plane::originGet(float vec[3]) {

  ELL_3V_COPY(vec, _origin);
}

void
Plane::edgeU(float X, float Y, float Z) {
  
  if (_edgeU[0] != X
      || _edgeU[1] != Y
      || _edgeU[2] != Z) {
    ELL_3V_SET(_edgeU, X, Y, Z);
    _flag[flagEdge] = true;
  }
}

void
Plane::edgeUSet(const float vec[3]) {

  edgeU(vec[0], vec[1], vec[2]);
}

void
Plane::edgeUGet(float vec[3]) {

  ELL_3V_COPY(vec, _edgeU);
}

void
Plane::edgeV(float X, float Y, float Z) {
  
  if (_edgeV[0] != X
      || _edgeV[1] != Y
      || _edgeV[2] != Z) {
    ELL_3V_SET(_edgeV, X, Y, Z);
    _flag[flagEdge] = true;
  }
}

void
Plane::edgeVSet(const float vec[3]) {

  edgeV(vec[0], vec[1], vec[2]);
}

void
Plane::edgeVGet(float vec[3]) {

  ELL_3V_COPY(vec, _edgeV);
}

bool
Plane::updateGeometry() {
  // char me[]="Plane::updateGeometry";
  bool ret = false;

  /*
  fprintf(stderr, "%s: flag: res=%s (%u %u), edge=%s, orig=%s\n", me,
          _flag[flagResolution] ? "true" : "false",
          _resolutionU, _resolutionV,
          _flag[flagEdge] ? "true" : "false",
          _flag[flagOrigin] ? "true" : "false");
  fprintf(stderr, "%s: _edgeU = %g %g %g\n", me,
          _edgeU[0], _edgeU[1], _edgeU[2]);
  fprintf(stderr, "%s: _edgeV = %g %g %g\n", me,
          _edgeV[0], _edgeV[1], _edgeV[2]);
  fprintf(stderr, "%s: _origin = %g %g %g\n", me,
          _origin[0], _origin[1], _origin[2]);
  */
  if (_flag[flagResolution]) {
    ret = true;
    // polygonalization changes
    // these actually re-use the _lpld, but clipping will change that...
    limnPolyDataPlane(_lpldOwn, _resolutionU, _resolutionV);

    _flag[flagResolution] = false;
    // HEY: not true, but need to somehow force vertex update below ...
    _flag[flagEdge] = true;   
  }
  if (_flag[flagOrigin] || _flag[flagEdge]) {
    ret = true;
    float norm[3], tmp;
    ELL_3V_CROSS(norm, _edgeU, _edgeV);
    ELL_3V_NORM_TT(norm, float, norm, tmp);
    unsigned int vertIdx = 0;
    for (unsigned int vIdx=0; vIdx<_resolutionV; vIdx++) {
      float vv = static_cast<float>(AIR_AFFINE(0, vIdx, _resolutionV-1,
                                               0.0, 1.0));
      for (unsigned int uIdx=0; uIdx<_resolutionU; uIdx++) {
        float uu = static_cast<float>(AIR_AFFINE(0, uIdx, _resolutionU-1,
                                                 0.0, 1.0));
        ELL_3V_SCALE_ADD3(_lpldOwn->vert[vertIdx].xyzw,
                          1.0f, _origin, uu, _edgeU, vv, _edgeV);
        _lpldOwn->vert[vertIdx].xyzw[3] = 1.0f;
        ELL_3V_COPY(_lpldOwn->vert[vertIdx].norm, norm);
        ++vertIdx;
      }
    }
    _flag[flagOrigin] = false;
    _flag[flagEdge] = false;
  }
  if (ret) {
    changed(); // have to explicitly indicate vertex data change
  }
  return ret;
}

bool
Plane::update() {
  char me[]="Plane::update";

  bool ret = this->updateGeometry();
  fprintf(stderr, "!%s: updating, ret %s\n", me, ret ? "true" : "false");
  ret |= dynamic_cast<PolyProbe*>(this)->update(ret);
  if (ret) {
    changed(); // have to explicitly indicate vertex data change
  }
  return ret;
}

} /* namespace Deft */

