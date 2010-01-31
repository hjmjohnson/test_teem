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
#include "PolyData.h"
#include "Viewer.h"
#include "Plane.h"

int
main(int, char **argv) {
  char *me;

  me = argv[0];
  double matScl[16];

  Deft::Scene *scene = new Deft::Scene();
  scene->bgColor(0.1f, 0.15f, 0.2f);
  Deft::Viewer *viewer = new Deft::Viewer(scene, 640, 480);
  viewer->camera(3, 4, 5,
                 0, 0, 0,
                 0, 0, 1,
                 20, -2, 2);
  viewer->resizable(viewer);
  viewer->end();
  char *fakeArgv[2] = {"Deft", NULL};
  viewer->show(1, fakeArgv);

  limnPolyData *pld = limnPolyDataNew();
  limnPolyDataSpiralSuperquadric(pld, 
                                 (1 << limnPolyDataInfoRGBA)
                                 | (1 << limnPolyDataInfoNorm),
                                 1, 1,
                                 10, 10);
  ELL_4M_SCALE_SET(matScl, 0.5, 0.5, 0.5);
  limnPolyDataTransform_d(pld, matScl);
  Deft::PolyData *surf = new Deft::PolyData(pld, true);
  surf->wireframe(true);
  surf->normalsUse(true);
  surf->visible(true);
  scene->objectAdd(surf);

  /*
  limnPolyData *pld2 = limnPolyDataNew();
  limnPolyDataCone(pld2, 
                   (1 << limnPolyDataInfoRGBA)
                   | (1 << limnPolyDataInfoNorm),
                   30, AIR_TRUE);
  ELL_4M_SCALE_SET(matScl, 0.5, 0.5, 0.5);
  limnPolyDataTransform_d(pld2, matScl);
  ELL_4M_TRANSLATE_SET(matScl, 0.9, 0, 0);
  limnPolyDataTransform_d(pld2, matScl);
  surf = new Deft::PolyData(pld2, true);
  surf->wireframe(false);
  surf->normalsUse(true);
  surf->visible(true);
  scene->objectAdd(surf);
  */

  /*
  unsigned int N, P, pi;
  N = 30; P = 3;
  pld = limnPolyDataNew();
  limnPolyDataAlloc(pld, 
                    (1 << limnPolyDataInfoNorm),
                    P*N, P*N, P);
  for (pi=0; pi<P; pi++) {
    pld->type[pi] = limnPrimitiveLineStrip;
    pld->icnt[pi] = N;
    for (unsigned int ii=0; ii<N; ii++) {
      double th = AIR_AFFINE(0, ii, N, 0, 1.8*AIR_PI);
      ELL_4V_SET(pld->xyzw + 4*(ii + N*pi), cos(th), sin(th), pi/3.0, 1);
    }
    for (unsigned int ii=0; ii<N; ii++) {
      pld->indx[ii + pi*N] = ii + N*pi;
    }
  }

  limnPolyData *tubes = limnPolyDataNew();
  if (limnPolyDataSpiralTubeWrap(tubes, pld, 
                                 limnPolyDataInfoBitFlag(pld),
                                 NULL, 16, 4, 0.1)) {
    char *err;
    fprintf(stderr, "%s: trouble:\n%s\n", me, err = biffGetDone(LIMN));
    free(err);
  }

  surf = new Deft::PolyData(tubes, true);
  surf->wireframe(true);
  surf->normalsUse(true);
  surf->visible(true);
  scene->objectAdd(surf);
  */

  /*
  Deft::Plane *plane = new Deft::Plane(10, 30);
  plane->update();
  plane->wireframe(true);
  plane->twoSided(true);
  scene->objectAdd(plane);
  */

  // this returns when the user quits
  int ret = fltk::run();

  return ret;
}
