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
#include "Object.h"
#include "Contour.h"
#include "Viewer.h"
#include "ViewerUI.h"
#include "TensorGlyph.h"
#include "TensorGlyphUI.h"
#include "TriPlane.h"
#include "TriPlaneUI.h"
#include "HyperStreamline.h"
#include "HyperStreamlineUI.h"
#include "Gage.h"
#include "Cmap.h"

#include <fltk/Group.h>
#include <fltk/Menu.h>
#include <fltk/PopupMenu.h>
#include <fltk/draw.h>
#include <fltk/Font.h>
#include <fltk/IntInput.h>
#include <fltk/FloatInput.h>

#include <teem/push.h>

char *info = ("Test program for push library.");

typedef struct {
  int running;
  Deft::Viewer *viewer;
  Deft::TensorGlyph *glyph;
  pushContext *pctx;
  Nrrd *nPosOut, *nTenOut, *nEnrOut;
  fltk::IntInput *iters;
  fltk::FloatInput *scale;
  Deft::Slider *step;
  Deft::Slider *gravScl;
  fltk::CheckButton *midPntSmp;
} pushBag;

pushBag *theBag;

void
step_cb(fltk::Widget *, pushBag *bag) {
  /*  static double lastthresh = -42; */
  char me[]="step_cb", *err;
  static unsigned int itersTotal=0;

  unsigned int iters = bag->iters->ivalue();
  bag->pctx->scale = bag->scale->fvalue();
  bag->pctx->step = bag->step->value();
  if (tenGageUnknown != bag->pctx->gravItem) {
    bag->pctx->gravScl = bag->gravScl->value();
  }
  bag->pctx->midPntSmp = bag->midPntSmp->value();
  if (iters > 5) {
    fprintf(stderr, "!%s: ", me); fflush(stderr);
  }
  bag->pctx->maxIter = iters;
  pushRun(bag->pctx);
  itersTotal += iters;
  fprintf(stderr, "!%s: energy = %g; time = %g sec; %u iters (%g iters/sec)\n",
          me, bag->pctx->energySum, bag->pctx->timeRun, itersTotal,
          itersTotal/bag->pctx->timeRun);
  if (pushOutputGet(bag->nPosOut, bag->nTenOut, bag->nEnrOut, bag->pctx)) {
    err = biffGetDone(PUSH);
    fprintf(stderr, "%s: error getting push output:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  bag->step->value(bag->pctx->step);
  nrrdSave("nPosOut.nrrd", bag->nPosOut, NULL);
  nrrdSave("nTenOut.nrrd", bag->nTenOut, NULL);
  nrrdSave("nEnrOut.nrrd", bag->nEnrOut, NULL);
  bag->glyph->dataSet(bag->nPosOut->axis[1].size,
                      (float*)bag->nTenOut->data, 7,
                      (float*)bag->nPosOut->data, 3,
                      NULL);
  bag->glyph->update();
  bag->viewer->redraw();
  fltk::flush();
}

void
theStepper(pushBag *bag) {

  step_cb(NULL, bag);
}

/*
void
start_cb(fltk::Widget *, pushBag *bag) {

  bag->running = AIR_TRUE;
  fltk::repeat_timeout(0.01,(fltk::TimeoutHandler)theStepper, bag);
}

void
stop_cb(fltk::Widget *, pushBag *bag) {

  bag->running = AIR_FALSE;
  fltk::remove_timeout((fltk::TimeoutHandler)theStepper, bag);
}
*/

int
main(int argc, char *argv[]) {
  char *me, *err;
  hestOpt *hopt=NULL;
  airArray *mop;
  
  char *gravStr, *gravGradStr, *seedStr, *zcStr, *gvStr;
  int camkeep;
  pushContext *pctx;
  Nrrd *_nin, *nin, *nPosIn=NULL;
  NrrdKernelSpec *ksp00, *ksp11, *ksp22;
  pushEnergySpec *ensp;
  
  float fr[3], at[3], up[3], fovy, neer, faar, dist, bg[3],
    anisoThresh, anisoThreshMin, glyphScale, sqdSharp;
  int size[2], ortho, rght, atrel, glyphType, glyphFacetRes;
  int aniso;

  me = argv[0];

  mop = airMopNew();
  pctx = pushContextNew();
  airMopAdd(mop, pctx, (airMopper)pushContextNix, airMopAlways);

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &_nin, NULL,
             "input volume to filter", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "np", "# points", airTypeUInt, 1, 1,
             &(pctx->pointNum), "1000",
             "number of points to use in simulation");
  hestOptAdd(&hopt, "pi", "npos", airTypeOther, 1, 1, &nPosIn, "",
             "positions to start at (overrides \"-np\")",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "step", "step", airTypeDouble, 1, 1,
             &(pctx->stepInitial), "1",
             "step size for gradient descent");
  hestOptAdd(&hopt, "scl", "scale", airTypeDouble, 1, 1,
             &(pctx->scale), "1500",
             "scaling from tensor size to glyph size");
  hestOptAdd(&hopt, "wall", "wall", airTypeDouble, 1, 1,
             &(pctx->wall), "0.0",
             "spring constant of containing walls");
  hestOptAdd(&hopt, "cntscl", "scale", airTypeDouble, 1, 1, 
             &(pctx->cntScl), "0.0",
             "scaling of containment force");
  hestOptAdd(&hopt, "limit", "frac", airTypeDouble, 1, 1, 
             &(pctx->deltaLimit), "0.3",
             "speed limit on particles' motion");
  hestOptAdd(&hopt, "dfmin", "frac", airTypeDouble, 1, 1, 
             &(pctx->deltaFracMin), "0.2",
             "decrease step size if deltaFrac goes below this");

  hestOptAdd(&hopt, "esf", "frac", airTypeDouble, 1, 1, 
             &(pctx->energyStepFrac), "0.9",
             "when energy goes up instead of down, fraction by "
             "which to scale step size");
  hestOptAdd(&hopt, "dfsf", "frac", airTypeDouble, 1, 1, 
             &(pctx->deltaFracStepFrac), "0.5",
             "when deltaFrac goes below deltaFracMin, fraction by "
             "which to scale step size");
  hestOptAdd(&hopt, "eimin", "frac", airTypeDouble, 1, 1, 
             &(pctx->energyImprovMin), "0.01",
             "convergence threshold: stop when fracional improvement "
             "(decrease) in energy dips below this");

  hestOptAdd(&hopt, "detr", NULL, airTypeBool, 0, 0, 
             &(pctx->detReject), NULL,
             "do determinant-based rejection of initial sample locations");
  hestOptAdd(&hopt, "rng", "seed", airTypeUInt, 1, 1, 
             &(pctx->seedRNG), "42",
             "seed value for RNG which determines initial point locations");
  hestOptAdd(&hopt, "nt", "# threads", airTypeUInt, 1, 1,
             &(pctx->threadNum), "1",
             "number of threads to run");
  hestOptAdd(&hopt, "nprob", "# iters", airTypeDouble, 1, 1,
             &(pctx->neighborTrueProb), "1.0",
             "do full neighbor traversal with this probability");
  hestOptAdd(&hopt, "pprob", "# iters", airTypeDouble, 1, 1,
             &(pctx->probeProb), "1.0",
             "do field probing with this probability");
  hestOptAdd(&hopt, "maxi", "# iters", airTypeUInt, 1, 1,
             &(pctx->maxIter), "0",
             "if non-zero, max # iterations to run");
  hestOptAdd(&hopt, "snap", "iters", airTypeUInt, 1, 1, &(pctx->snap), "0",
             "if non-zero, # iterations between which a snapshot "
             "is saved");

  hestOptAdd(&hopt, "grv", "item", airTypeString, 1, 1, &gravStr, "none",
             "item to act as gravity");
  hestOptAdd(&hopt, "grvgv", "item", airTypeString, 1, 1, &gravGradStr, "none",
             "item to act as gravity gradient");
  hestOptAdd(&hopt, "grvs", "scale", airTypeDouble, 1, 1, &(pctx->gravScl),
             "nan", "magnitude and scaling of gravity vector");
  hestOptAdd(&hopt, "grvz", "scale", airTypeDouble, 1, 1, &(pctx->gravZero),
             "nan", "height (WRT gravity) of zero potential energy");

  hestOptAdd(&hopt, "seed", "item", airTypeString, 1, 1, &seedStr, "none",
             "item to act as seed threshold");
  hestOptAdd(&hopt, "seedth", "thresh", airTypeDouble, 1, 1,
             &(pctx->seedThresh), "nan",
             "seed threshold threshold");

  hestOptAdd(&hopt, "energy", "spec", airTypeOther, 1, 1, &ensp, "cotan",
             "specification of energy function to use",
             NULL, NULL, pushHestEnergySpec);

  hestOptAdd(&hopt, "nobin", NULL, airTypeBool, 0, 0,
             &(pctx->binSingle), NULL,
             "turn off spatial binning (which prevents multi-threading "
             "from being useful), for debugging or speed-up measurement");

  hestOptAdd(&hopt, "k00", "kernel", airTypeOther, 1, 1, &ksp00,
             "tent", "kernel for tensor field sampling",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kernel", airTypeOther, 1, 1, &ksp11,
             "fordif", "kernel for finding containment gradient from mask",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kernel", airTypeOther, 1, 1, &ksp22,
             "cubicdd:1,0", "kernel for 2nd derivatives",
             NULL, NULL, nrrdHestKernelSpec);

  hestOptAdd(&hopt, "zc", "item", airTypeString, 1, 1, &zcStr, "omlapl",
             "item for zero-crossing surface (some 2nd deriv of a scalar)");
  hestOptAdd(&hopt, "gv", "item", airTypeString, 1, 1, &gvStr, "omgv",
             "item for gradient vector of the underlying scalar");

  hestOptAdd(&hopt, "a", "aniso", airTypeEnum, 1, 1, &aniso, NULL,
             "anisotropy metric to make volume of",
             NULL, tenAniso);
  hestOptAdd(&hopt, "atr", "aniso thresh", airTypeFloat, 1, 1,
             &anisoThresh, "0.85",
             "Glyphs will be drawn only for tensors with anisotropy "
             "greater than this threshold");
  hestOptAdd(&hopt, "atrm", "aniso thresh min", airTypeFloat, 1, 1,
             &anisoThreshMin, "0.4",
             "lower bound on aniso thresh");
  hestOptAdd(&hopt, "g", "glyph shape", airTypeEnum, 1, 1, &glyphType, "sqd",
             "shape of glyph to use for display.  Possibilities "
             "include \"box\", \"sphere\"=\"sph\", \"cylinder\"=\"cyl\", and "
             "\"superquad\"=\"sqd\"", NULL, tenGlyphType);
  hestOptAdd(&hopt, "gsc", "scale", airTypeFloat, 1, 1, &glyphScale,
             "0.25", "over-all glyph size");
  hestOptAdd(&hopt, "gr", "glyph res", airTypeInt, 1, 1, &glyphFacetRes, "7",
             "resolution of polygonalization of glyphs (other than box)");
  hestOptAdd(&hopt, "sh", "sharpness", airTypeFloat, 1, 1, &sqdSharp, "2.5",
             "for superquadric glyphs, how much to sharp edges form as a "
             "function of differences between eigenvalues.  Higher values "
             "mean that edges form more easily");
  hestOptAdd(&hopt, "fr", "from point", airTypeFloat, 3, 3, fr, "3 4 5",
             "position of camera, used to determine view vector");
  hestOptAdd(&hopt, "at", "at point", airTypeFloat, 3, 3, at, "0 0 0",
             "camera look-at point, used to determine view vector");
  hestOptAdd(&hopt, "up", "up vector", airTypeFloat, 3, 3, up, "0 0 1",
             "camera pseudo-up vector, used to determine view coordinates");
  hestOptAdd(&hopt, "rh", NULL, airTypeInt, 0, 0, &rght, NULL,
             "normally, use a right-handed UVN frame (V points down), "
             "but in stark this is always true");
  hestOptAdd(&hopt, "fv", "fov", airTypeFloat, 1, 1, &fovy, "20",
             "vertical field-of-view, in degrees");
  hestOptAdd(&hopt, "or", NULL, airTypeInt, 0, 0, &ortho, NULL,
             "use orthogonal projection instead of perspective");
  hestOptAdd(&hopt, "dn", "near clip", airTypeFloat, 1, 1, &neer, "-2",
             "position of near clipping plane, relative to look-at point");
  hestOptAdd(&hopt, "di", "image", airTypeFloat, 1, 1, &dist, "0.0",
             "normally, distance to image plane, "
             "but in stark this is always 0.0");
  hestOptAdd(&hopt, "df", "far clip", airTypeFloat, 1, 1, &faar, "2",
             "position of far clipping plane, relative to look-at point");
  hestOptAdd(&hopt, "ar", NULL, airTypeInt, 0, 0, &atrel, NULL,
             "normally: near, image, and far plane distances are relative to "
             "the *at* point, instead of the eye point, "
             "but for stark, this is always true");
  hestOptAdd(&hopt, "usecam", NULL, airTypeInt, 0, 0, &camkeep, NULL,
             "hack: by default, a camera reset is done to put the volume "
             "in view. Use this to say that the camera specified by the "
             "flags above should be preserved and used");
  hestOptAdd(&hopt, "bg", "R G B", airTypeFloat, 3, 3, bg, "0.1 0.15 0.2",
             "background color");
  hestOptAdd(&hopt, "is", "su sv", airTypeInt, 2, 2, size, "640 480",
             "initial window size");

  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  fprintf(stderr, "%s: %p %p\n", me, pctx, pctx->ensp);
  
  if (3 == _nin->spaceDim && AIR_EXISTS(_nin->measurementFrame[0][0])) {
    nin = nrrdNew();
    airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
    if (tenMeasurementFrameReduce(nin, _nin)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble undoing measurement frame:\n%s", me, err);
      airMopError(mop);
      exit(1);
    }
  } else {
    nin = _nin;
  }

  pctx->nin = nin;
  pctx->npos = nPosIn;
  pctx->verbose = 0;
  pctx->binIncr = 84;  /* random small-ish value */
  pushEnergySpecSet(pctx->ensp, ensp->energy, ensp->parm);
  nrrdKernelSpecSet(pctx->ksp00, ksp00->kernel, ksp00->parm);
  nrrdKernelSpecSet(pctx->ksp11, ksp11->kernel, ksp11->parm);
  nrrdKernelSpecSet(pctx->ksp22, ksp22->kernel, ksp22->parm);
  if (strcmp("none", gravStr)) {
    pctx->gravItem = airEnumVal(tenGage, gravStr);
    if (tenGageUnknown == pctx->gravItem) {
      fprintf(stderr, "%s: couldn't parse \"%s\" as a %s (gravity)\n", me,
              gravStr, tenGage->name);
      airMopError(mop);
      return 1;
    }
    pctx->gravGradItem = airEnumVal(tenGage, gravGradStr);
    if (tenGageUnknown == pctx->gravGradItem) {
      fprintf(stderr, "%s: couldn't parse \"%s\" as a %s (gravity grad)\n",
              me, gravGradStr, tenGage->name);
      airMopError(mop);
      return 1;
    }
  } else {
    pctx->gravItem = tenGageUnknown;
    pctx->gravGradItem = tenGageUnknown;
    pctx->gravZero = AIR_NAN;
    pctx->gravScl = AIR_NAN;
  }

  if (strcmp("none", seedStr)) {
    pctx->seedThreshItem = airEnumVal(tenGage, seedStr);
    if (tenGageUnknown == pctx->seedThreshItem) {
      fprintf(stderr, "%s: couldn't parse \"%s\" as a %s (seedthresh)\n", me,
              seedStr, tenGage->name);
      airMopError(mop);
      return 1;
    }
  } else {
    pctx->seedThreshItem = 0;
    pctx->seedThresh = AIR_NAN;
  }

  if (pushStart(pctx)) {
    airMopAdd(mop, err = biffGetDone(PUSH), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble starting push:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }
  fprintf(stderr, "!%s: pushStart finished\n", me);

  Deft::Scene *scene = new Deft::Scene();
  scene->bgColor(bg[0], bg[1], bg[2]);
  // Deft::Viewer *viewer = new Deft::Viewer(scene, 10, 10, size[0], size[1]);
  Deft::Viewer *viewer = new Deft::Viewer(scene, size[0], size[1]);
  if (camkeep) {
    viewer->camera(fr[0], fr[1], fr[2],
                   at[0], at[1], at[2],
                   up[0], up[1], up[2],
                   fovy, neer, faar);
  }
  viewer->resizable(viewer);
  viewer->end();
  char *fakeArgv[2] = {"Deft", NULL};
  viewer->show(1, fakeArgv);
  if (ortho) {
    // total hack
    viewer->keyboard('p', 0, 0);
  }

  Deft::TensorGlyph *glyph = new Deft::TensorGlyph();
  glyph->dynamic(false);
  glyph->anisoType(aniso);
  glyph->anisoThreshMin(anisoThreshMin);
  glyph->anisoThresh(anisoThresh);
  glyph->glyphType(glyphType);
  glyph->superquadSharpness(sqdSharp);
  glyph->glyphResolution(glyphFacetRes);
  glyph->glyphScale(glyphScale);
  glyph->rgbEvecParmSet(tenAniso_Cl2, 0, 0.7, 1.0, 2.3, 1.0);
  glyph->maskThresh(0.0);
  /*
  void rgbEvecParmSet(int aniso, unsigned int evec,
                      double maxSat, double isoGray,
                      double gamma, double modulate);
  */

  scene->objectAdd(glyph);

  theBag = (pushBag*)calloc(1, sizeof(pushBag));
  theBag->running = AIR_FALSE;
  theBag->glyph = glyph;
  theBag->pctx = pctx;
  theBag->nPosOut = nrrdNew();
  theBag->nTenOut = nrrdNew();
  theBag->nEnrOut = nrrdNew();
  theBag->viewer = viewer;

  int winy = 10;
  fltk::Window *win = new fltk::Window(400, 400, "push UI");
  win->begin();
  fltk::Button *stepButton = new fltk::Button(10, winy, 50, 20, "step");
  stepButton->callback((fltk::Callback*)step_cb, theBag);

  theBag->iters = new fltk::IntInput(200, winy, 50, 20, "# iters");
  theBag->iters->value(1);

  theBag->scale = new fltk::FloatInput(300, winy, 50, 20, "scale");
  theBag->scale->value(pctx->scale);

  winy += 30;
  theBag->step = new Deft::Slider(0, winy, 400, 40, "time step");
  theBag->step->range(0, 2*pctx->step);
  theBag->step->value(pctx->step);
  
  winy += 30;
  theBag->midPntSmp = new fltk::CheckButton(85, winy, 30, 40, "midPntSmp");
  theBag->midPntSmp->value(pctx->midPntSmp);

  if (tenGageUnknown != pctx->gravItem) {
    winy += 50;
    theBag->gravScl = new Deft::Slider(0, winy, 400, 40, "gravity scale");
    theBag->gravScl->range(0, 2*pctx->gravScl);
    theBag->gravScl->value(pctx->gravScl);
  }

  /*
  fltk::Button *startButton = new fltk::Button(10, 40, 100, 20, "start");
  startButton->callback((fltk::Callback*)start_cb, theBag);
  fltk::Button *stopButton = new fltk::Button(10, 70, 100, 20, "stop");
  stopButton->callback((fltk::Callback*)stop_cb, theBag);
  */

  win->end();
  win->show();

  Deft::ViewerUI *viewerUI = new Deft::ViewerUI(viewer);
  viewerUI->show();
  
  Deft::TensorGlyphUI *glyphUI = new Deft::TensorGlyphUI(glyph, viewer);
  glyphUI->show();

  // --------------------------------------------------

  Deft::Volume *vol = new Deft::Volume(tenGageKind, nin);
  Deft::TriPlane *triplane = new Deft::TriPlane(vol);
  
  NrrdKernelSpec *ksp00_tmp = nrrdKernelSpecNew();
  double kparm[10] = {1,1,0};
  nrrdKernelSpecSet(ksp00_tmp, nrrdKernelTent, kparm);
  triplane->kernel(gageKernel00, ksp00_tmp);
  triplane->colorQuantity(Deft::colorTenQuantityRgbLinear);
  // HEY: you can eventually segfault if this isn't set here
  // shouldn't doing so be optional?
  triplane->alphaMaskQuantity(Deft::alphaMaskTenQuantityConfidence);
  triplane->visible(false);

  Deft::TriPlaneUI *planeUI = new Deft::TriPlaneUI(triplane, viewer);
  planeUI->show();
  scene->groupAdd(triplane);

  fltk::flush();
  fltk::redraw();
  // this returns when the user quits
  /*int ret = */ fltk::run();

  /*
  fprintf(stderr, "%s: time for %d iterations= %g secs; meanVel = %g\n",
          me, pctx->iter, pctx->time, pctx->meanVel);
  if (nrrdSave(outS[0], nPosOut, NULL)
      || nrrdSave(outS[1], nTenOut, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: couldn't save output:\n%s\n", me, err);
    airMopError(mop); 
    return 1;
  }
  */

  airMopOkay(mop);
  return 0;
}
