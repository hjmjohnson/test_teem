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

#include "Deft.h"
#include "Contour.h"

namespace Deft {

Contour::Contour() {

  lctx = limnContour3DContextNew();
  lctx->findNormals = AIR_TRUE;
}

Contour::~Contour() {

  lctx = limnContour3DContextNix(lctx);
}

void
Contour::volumeSet(const Nrrd *nvol) {
  char me[]="Contour::volumeSet", *err;
  const char *key;
  gageShape *shape;
  airArray *mop;

  mop = airMopNew();
  shape = gageShapeNew();
  airMopAdd(mop, shape, (airMopper)gageShapeNix, airMopAlways);
  if (!(key = GAGE)
      || gageShapeSet(shape, nvol, 0)
      || !(key = LIMN)
      || limnContour3DVolumeSet(lctx, nvol)
      || limnContour3DTransformSet(lctx, shape->ItoW)) {
    airMopAdd(mop, err=biffGetDone(key), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop); return;
  }
  airMopOkay(mop);
  return;
}

void
Contour::lowerInsideSet(int val) {
  char me[]="Contour::lowerInsideSet", *err;

  if (limnContour3DLowerInsideSet(lctx, val)) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    free(err); return;
  }
  return;
}

void
Contour::extract(limnObject *obj, double isovalue) {
  char me[]="Contour::extract", *err;
  double time0, time1;

  time0 = airTime();
  limnObjectEmpty(obj);
  /*
  lctx->findNormals = AIR_FALSE;
  if (limnContour3DExtract(lctx, obj, isovalue)
      || limnObjectFaceNormals(obj, limnSpaceWorld)
      || limnObjectVertexNormals(obj)) {
  */
  if (limnContour3DExtract(lctx, obj, isovalue)) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble getting isosurface:\n%s", me, err);
    free(err); 
    limnObjectEmpty(obj);
    return;
  }
  time1 = airTime();
  /*
  fprintf(stderr, "%s: %d tris, %g|%g sec: %g|%g Ktris/sec\n", me,
          obj->faceNum, lctx->time, time1-time0,
          (double)(obj->faceNum)/(1000.0*lctx->time),
          (double)(obj->faceNum)/(1000.0*(time1 - time0)));
  */
  return;
}

double
Contour::minimum() {
  return lctx->range->min;
}

double
Contour::maximum() {
  return lctx->range->max;
}

} /* namespace Deft */
