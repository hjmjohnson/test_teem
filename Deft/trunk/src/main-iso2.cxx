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
#include "Viewer.h"
#include "ViewerUI.h"
#include "Slider.H"

char *info = ("this might come in handy.");

typedef struct {
  Deft::Slider *slider;
  Deft::Contour *contour;
  Deft::Scene *scene;
  Deft::Viewer *viewer;
} isoBag;

void
isovalue_cb(fltk::Widget *widget, void *data) {
  Deft::Slider *slider;
  isoBag *bag;

  slider = (Deft::Slider *)widget;
  bag = (isoBag *)data;
  bag->contour->extract(slider->value());
  bag->viewer->redraw();
}

int
main(int argc, char **argv) {
  hestOpt *hopt=NULL;
  airArray *mop;
  char *me;

  char *inS, *err;
  float fr[3], at[3], up[3], fovy, neer, faar, dist, bg[3];
  int size[2], ortho, rght, atrel, camkeep;
  Nrrd *nin;
  isoBag bag;

  mop = airMopNew();

  me = argv[0];
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
             "input volume to surface",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "fr", "from point", airTypeFloat, 3, 3, fr, "3 4 5",
             "position of camera, used to determine view vector");
  hestOptAdd(&hopt, "at", "at point", airTypeFloat, 3, 3, at, "0 0 0",
             "camera look-at point, used to determine view vector");
  hestOptAdd(&hopt, "up", "up vector", airTypeFloat, 3, 3, up, "0 0 1",
             "camera pseudo-up vector, used to determine view coordinates");
  hestOptAdd(&hopt, "rh", NULL, airTypeInt, 0, 0, &rght, NULL,
             "normally, use a right-handed UVN frame (V points down), "
             "but in Deft this is always true");
  hestOptAdd(&hopt, "fv", "fov", airTypeFloat, 1, 1, &fovy, "20",
             "vertical field-of-view, in degrees");
  hestOptAdd(&hopt, "or", NULL, airTypeInt, 0, 0, &ortho, NULL,
             "use orthogonal projection instead of perspective");
  hestOptAdd(&hopt, "dn", "near clip", airTypeFloat, 1, 1, &neer, "-2",
             "position of near clipping plane, relative to look-at point");
  hestOptAdd(&hopt, "di", "image", airTypeFloat, 1, 1, &dist, "0.0",
             "normally, distance to image plane, "
             "but in Deft this is always 0.0");
  hestOptAdd(&hopt, "df", "far clip", airTypeFloat, 1, 1, &faar, "2",
             "position of far clipping plane, relative to look-at point");
  hestOptAdd(&hopt, "ar", NULL, airTypeInt, 0, 0, &atrel, NULL,
             "normally: near, image, and far plane distances are relative to "
             "the *at* point, instead of the eye point, "
             "but for Deft, this is always true");
  hestOptAdd(&hopt, "bg", "R G B", airTypeFloat, 3, 3, bg, "0.2 0.3 0.4",
             "background color");
  hestOptAdd(&hopt, "is", "su sv", airTypeInt, 2, 2, size, "640 480",
             "initial window size");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  bag.scene = new Deft::Scene();
  bag.scene->bgColor(bg[0], bg[1], bg[2]);
  bag.viewer = new Deft::Viewer(bag.scene, size[0], size[1]);
  if (camkeep) {
    bag.viewer->camera(fr[0], fr[1], fr[2],
                       at[0], at[1], at[2],
                       up[0], up[1], up[2],
                       fovy, neer, faar);
  }
  bag.viewer->resizable(bag.viewer);
  bag.viewer->end();
  char *fakeArgv[2] = {"Deft_iso", NULL};
  bag.viewer->show(1, fakeArgv);
  if (ortho) {
    // total hack
    bag.viewer->keyboard('p', 0, 0);
  }
  // bag.viewer->helpPrint();
  Deft::ViewerUI *viewerUI = new Deft::ViewerUI(bag.viewer);
  viewerUI->show();

  bag.contour = new Deft::Contour();
  bag.contour->volumeSet(nin);
  bag.contour->extract(0);

  bag.scene->objectAdd(bag.contour);

  fltk::Window *window = new fltk::Window(size[0]+20, 0, 400, 90);
  window->begin();
  window->resizable(window);

  int incy, winy = 0;
  bag.slider = new Deft::Slider(0, winy, window->w(), incy=55, "isovalue");
  winy += incy;
  bag.slider->align(fltk::ALIGN_LEFT);
  bag.slider->range(bag.contour->minimum(), bag.contour->maximum());
  bag.slider->value(bag.contour->maximum());
  bag.slider->fastUpdate(1);
  bag.slider->callback(isovalue_cb, &bag);

  window->end();
  window->show(argc,argv);


  // this returns when the user quits
  int ret = fltk::run();

  airMopOkay(mop);
  return ret;
}
