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

#include "Deft.h"
#include "Actor.h"
#include "PolyData.h"
#include "Viewer.h"
#include "Plane.h"

int
main(int, char **argv) {
  char *me;

  me = argv[0];

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
  viewer->helpPrint();

  limnPolyData *pld = limnPolyDataNew();
  limnPolyDataSpiralSuperquadric(pld, 0.5f, 0.5f, 41, 22);
  Deft::PolyData *surf = new Deft::PolyData(pld, true);
  surf->wireframe(true);
  surf->visible(true);
  scene->objectAdd(surf);

  /*
  Deft::Plane *plane = new Deft::Plane(10, 30);
  plane->wireframe(true);
  plane->twoSided(true);
  scene->objectAdd(plane);
  */

  // this returns when the user quits
  int ret = fltk::run();

  return ret;
}
