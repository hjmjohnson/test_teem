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
#include "Anisocontour.h"
#include "AnisocontourUI.h"
#include "Anisofeature.h"
#include "AnisofeatureUI.h"
#include "Gage.h"
#include "Cmap.h"

#include <fltk/Group.h>
#include <fltk/Menu.h>
#include <fltk/PopupMenu.h>
#include <fltk/draw.h>
#include <fltk/Font.h>
#include <fltk/Choice.h>

char *info = ("this might come in handy.");

int
main(int argc, char **argv) {
  hestOpt *hopt=NULL;
  airArray *mop;
  char *me, *err;

  float fr[3], at[3], up[3], fovy, neer, faar, dist, bg[3],
    anisoThresh, anisoThreshMin, glyphScale, sqdSharp;
  int size[2], ortho, rght, atrel, glyphType, glyphFacetRes;
  Nrrd *_nin=NULL, *nin=NULL, *nPos=NULL, *nseed=NULL;
#if 0*3
  Nrrd *_ninBlur=NULL, *ninBlur;
#endif
#if 0*4
  Nrrd *nb0;
#endif
#if 0*5
  Nrrd *ntumor;
#endif
  int aniso, camkeep;

  mop = airMopNew();

  me = argv[0];
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &_nin, NULL,
             "input tensor volume",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "seed", "nin", airTypeOther, 1, 1, &nseed, "",
             "tractography seeds for tensor volume",
             NULL, NULL, nrrdHestNrrd);
#if 0*3
  hestOptAdd(&hopt, "ib", "nin", airTypeOther, 1, 1, &_ninBlur, NULL,
             "input tensor volume, blurred a titch",
             NULL, NULL, nrrdHestNrrd);
#endif
#if 0*4
  hestOptAdd(&hopt, "b0", "nin", airTypeOther, 1, 1, &nb0, NULL,
             "B0 volume",
             NULL, NULL, nrrdHestNrrd);
#endif
#if 0*5
  hestOptAdd(&hopt, "it", "nin", airTypeOther, 1, 1, &ntumor, NULL,
             "tumor distance map",
             NULL, NULL, nrrdHestNrrd);
#endif
  hestOptAdd(&hopt, "pi", "pos", airTypeOther, 1, 1, &nPos, "",
             "positions of input samples",
             NULL, NULL, nrrdHestNrrd);
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
  hestOptAdd(&hopt, "usecam", NULL, airTypeInt, 0, 0, &camkeep, NULL,
             "hack: by default, a camera reset is done to put the volume "
             "in view. Use this to say that the camera specified by the "
             "flags above should be preserved and used");
  hestOptAdd(&hopt, "bg", "R G B", airTypeFloat, 3, 3, bg, "0.1 0.12 0.14",
             "background color");
  hestOptAdd(&hopt, "is", "su sv", airTypeInt, 2, 2, size, "640 480",
             "initial window size");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (const char *envS = getenv("DEFT_HOME")) {
    strcpy(Deft::homeDir, envS);
    strcat(Deft::homeDir, "/");
  } else {
    fprintf(stderr, "%s: WARNING: \"DEFT_HOME\" environment variable "
            "not set; assuming \".\"\n", me);
    strcpy(Deft::homeDir, "./");
  }

  if (nPos) {
    if (!(2 == _nin->dim
          && nrrdTypeFloat == _nin->type
          && 7 == _nin->axis[0].size
          && 2 == nPos->dim
          && nrrdTypeFloat == nPos->type
          && 3 == nPos->axis[0].size)) {
      fprintf(stderr, "%s: try harder\n", me);
      airMopError(mop);
      exit(1);
    }
  }
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

#if 0*3
  if (3 == _ninBlur->spaceDim 
      && AIR_EXISTS(_ninBlur->measurementFrame[0][0])) {
    ninBlur = nrrdNew();
    airMopAdd(mop, ninBlur, (airMopper)nrrdNuke, airMopAlways);
    if (tenMeasurementFrameReduce(ninBlur, _ninBlur)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble undoing measurement frame:\n%s", me, err);
      airMopError(mop);
      exit(1);
    }
  } else {
    ninBlur = _ninBlur;
  }
#endif

  Deft::Scene *scene = new Deft::Scene();
  scene->bgColor(bg[0], bg[1], bg[2]);
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
  // viewer->helpPrint();
  Deft::ViewerUI *viewerUI = new Deft::ViewerUI(viewer);
  viewerUI->show();
  
  Deft::TensorGlyph *glyph = new Deft::TensorGlyph();
  if (nPos) {
    glyph->dataSet(nin->axis[1].size,
                   (float *)nin->data, 7,
                   (float *)nPos->data, 3, NULL);
  } else {
    glyph->volumeSet(nin);
  }
  glyph->dynamic(false);
  glyph->anisoType(aniso);
  // glyph->maskThresh(0.5);
  glyph->anisoThreshMin(anisoThreshMin);
  glyph->anisoThresh(anisoThresh);
  // glyph->wireframe(false);
  // glyph->cleanEdge(true);
  glyph->glyphType(glyphType);
  glyph->superquadSharpness(sqdSharp);
  glyph->glyphResolution(glyphFacetRes);
  // glyph->barycentricResolution(12);
  glyph->glyphScale(glyphScale);
  glyph->rgbParmSet(tenAniso_Cl2, 0, 0.7, 1.0, 2.3, 1.0);
  glyph->visible(true);
  glyph->update();
  /*
  void rgbParmSet(int aniso, unsigned int evec,
                  double maxSat, double isoGray,
                  double gamma, double modulate);
  */

  scene->objectAdd(glyph);

  Deft::TensorGlyphUI *glyphUI = new Deft::TensorGlyphUI(glyph, viewer);
  glyphUI->show();

  // --------------------------------------------------

  Deft::Volume *vol = new Deft::Volume(tenGageKind, nin);
#if 0*3
  Deft::Volume *volBlur = new Deft::Volume(tenGageKind, ninBlur);
#endif
  Deft::TriPlane *triplane = new Deft::TriPlane(vol);
  
  NrrdKernelSpec *ksp = nrrdKernelSpecNew();
  double kparm[10];

  kparm[0] = 1.0;
  nrrdKernelSpecSet(ksp, nrrdKernelTent, kparm);
  triplane->kernel(gageKernel00, ksp);
  ELL_3V_SET(kparm, 1, 1, 0);
  nrrdKernelSpecSet(ksp, nrrdKernelBCCubicD, kparm);
  triplane->kernel(gageKernel11, ksp);
  nrrdKernelSpecSet(ksp, nrrdKernelBCCubicDD, kparm);
  triplane->kernel(gageKernel22, ksp);
  triplane->colorQuantity(Deft::colorQuantityRgbLinear);
  // HEY: you can eventually segfault if this isn't set here
  // shouldn't doing so be optional?
  triplane->alphaMaskQuantity(Deft::alphaMaskQuantityConfidence);
  triplane->visible(false);
  scene->groupAdd(triplane);

  // HEY, WRONG: totally wrong place to be doing this
  triplane->glyph[0]->parmCopy(glyph);
  triplane->glyph[1]->parmCopy(glyph);
  triplane->glyph[2]->parmCopy(glyph);
  glyphUI->add(triplane->glyph[0]);
  glyphUI->add(triplane->glyph[1]);
  glyphUI->add(triplane->glyph[2]);

  Deft::TriPlaneUI *planeUI = new Deft::TriPlaneUI(triplane, viewer);
  planeUI->show();

  // --------------------------------------------------

  Deft::HyperStreamline *hsline = new Deft::HyperStreamline(vol);
  hsline->lightingUse(false);
  hsline->colorQuantity(Deft::colorQuantityRgbLinear);
  hsline->alphaMask(false);
  if (nseed) {
    hsline->seedsSet(nseed);
  }
  Deft::HyperStreamlineUI *hslineUI = 
    new Deft::HyperStreamlineUI(hsline, glyph, viewer);
  scene->objectAdd(hsline);

  // HEY, WRONG: totally wrong place to be doing this
  triplane->hsline[0]->parmCopy(hsline);
  triplane->hsline[1]->parmCopy(hsline);
  triplane->hsline[2]->parmCopy(hsline);
  hslineUI->add(triplane->hsline[0]);
  hslineUI->add(triplane->hsline[1]);
  hslineUI->add(triplane->hsline[2]);

  hslineUI->show();

  // --------------------------------------------------

#if 1

  Deft::Anisocontour *anicont = new Deft::Anisocontour(vol);
  anicont->colorQuantity(Deft::colorQuantityRgbLinear);
  anicont->alphaMask(true);
  // HEY: without this, the UI menu would come up blank because no there
  // was no value
  anicont->alphaMaskQuantity(Deft::alphaMaskQuantityConfidence);
  Deft::AnisocontourUI *anisoUI = 
    new Deft::AnisocontourUI(anicont, viewer);
  fprintf(stderr, "!%s: anicont->alphaMaskQuantity() = %u\n", me, 
          anicont->alphaMaskQuantity());
  scene->objectAdd(anicont);
  anicont->update();
  anisoUI->show();

#endif

  // --------------------------------------------------

#if 0*3

  Deft::Anisofeature *anifeatR = new Deft::Anisofeature(volBlur);
  anifeatR->type(seekTypeRidgeSurface);
  anifeatR->itemStrength(tenGageFARidgeSurfaceStrength);
  anifeatR->itemGradient(tenGageFAGradVec);
  anifeatR->itemEigensystem(tenGageFAHessianEval, tenGageFAHessianEvec);
  anifeatR->colorQuantity(Deft::colorQuantityRgbLinear);
  anifeatR->alphaMaskQuantity(Deft::alphaMaskQuantityFARidgeSurfaceStrength);
  Deft::AnisofeatureUI *anifeUIR = 
    new Deft::AnisofeatureUI(anifeatR, viewer);
  scene->objectAdd(anifeatR);
  anifeUIR->show();

  // --------------------------------------------------


  Deft::Anisofeature *anifeatV = new Deft::Anisofeature(volBlur);
  anifeatV->type(seekTypeValleySurface);
  anifeatV->itemStrength(tenGageFAValleySurfaceStrength);
  anifeatV->itemGradient(tenGageFAGradVec);
  anifeatV->itemEigensystem(tenGageFAHessianEval, tenGageFAHessianEvec);
  anifeatV->colorQuantity(Deft::colorQuantityRgbLinear);
  anifeatV->alphaMaskQuantity(Deft::alphaMaskQuantityFAValleySurfaceStrength);
  Deft::AnisofeatureUI *anifeUIV = 
    new Deft::AnisofeatureUI(anifeatV, viewer);
  scene->objectAdd(anifeatV);
  anifeUIV->show();

  /*
  Deft::Anisofeature *anifeatV = new Deft::Anisofeature(volBlur);
  anifeatV->type(seekTypeIsoContour);
  anifeatV->itemStrength(tenGageFAGradMag);
  anifeatV->itemScalar(tenGageFA2n
  anifeatV->itemGradient(tenGageFAGradVec);
  anifeatV->itemEigensystem(tenGageFAHessianEval, tenGageFAHessianEvec);
  anifeatV->colorQuantity(Deft::colorQuantityRgbLinear);
  anifeatV->alphaMaskQuantity(Deft::alphaMaskQuantityFAValleySurfaceStrength);
  Deft::AnisofeatureUI *anifeUIV = 
    new Deft::AnisofeatureUI(anifeatV, viewer);
  scene->objectAdd(anifeatV);
  anifeUIV->show();

  ftype = seekTypeIsocontour;
  gageKind *kind = tenGageKind;
  int sclvItem = tenGageFA;
  int normItem = tenGageFANormal;
  int strengthItem = tenGageConfidence;
  int strengthSign = 1;
  double strength = 0.5;
  */


  // --------------------------------------------------

#endif

  fltk::flush();
  glyph->update();
  fltk::redraw();

  if (!camkeep) {
    viewer->cameraReset();
  }

  /*
  fprintf(stderr, "%s: glGetString(GL_VERSION) = %s\n",
          me, glGetString(GL_VERSION));
  */
  
  // this returns when the user quits
  int ret = fltk::run();
  airMopOkay(mop);
  return ret;

  // this doesn't finish when you close all windows!
  // for (;;) fltk::wait();
  // airMopOkay(mop);
  // return 0;
}
