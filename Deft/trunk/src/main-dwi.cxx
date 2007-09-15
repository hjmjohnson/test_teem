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
#include "Viewer.h"
#include "ViewerUI.h"
#include "TriPlane.h"
#include "TriPlaneUI.h"
#include "SeedPoint.h"
#include "SeedPointUI.h"
#include "Hyperstreamline.h"
#include "HyperstreamlineUI.h"
#include "Gage.h"
#include "Cmap.h"

#include <fltk/Group.h>
#include <fltk/Menu.h>
#include <fltk/PopupMenu.h>
#include <fltk/draw.h>
#include <fltk/Font.h>
#include <fltk/Choice.h>

char *info = ("this might come in handy.");

/*
 inputs:

 dwi volume (for 2-ten tracking)
 list of seedpoints for 2-ten tracts
 1-ten volume (for RGB planes, 1-ten tracts)
 list of seedpoints for 1-ten tracts

*/

int
main(int argc, char **argv) {
  hestOpt *hopt=NULL;
  airArray *mop;
  char *me, *err;

  float fr[3], at[3], up[3], fovy, neer, faar, dist, bg[3], mmdwi;
  int size[2], ortho, rght, atrel;
  Nrrd *ninDwi;
  int camkeep;
  limnPolyData *baseGlyph;

  mop = airMopNew();

  me = argv[0];
  hestOptAdd(&hopt, "idwi", "nin", airTypeOther, 1, 1, &ninDwi, NULL,
             "input DWI volume",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "mmi", "val", airTypeFloat, 1, 1, &mmdwi, "40",
             "min mean dwi inside conf region");
  hestOptAdd(&hopt, "ibg", "glyph", airTypeOther, 1, 1, &baseGlyph, NULL,
             "base geometry for DWI glyph",
             NULL, NULL, limnHestPolyDataOFF);

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

  if (AIR_EXISTS(ninDwi->measurementFrame[0][0])) {
    float mf[9], tmp[3];

    mf[0] = static_cast<float>(ninDwi->measurementFrame[0][0]);
    mf[1] = static_cast<float>(ninDwi->measurementFrame[1][0]);
    mf[2] = static_cast<float>(ninDwi->measurementFrame[2][0]);
    mf[3] = static_cast<float>(ninDwi->measurementFrame[0][1]);
    mf[4] = static_cast<float>(ninDwi->measurementFrame[1][1]);
    mf[5] = static_cast<float>(ninDwi->measurementFrame[2][1]);
    mf[6] = static_cast<float>(ninDwi->measurementFrame[0][2]);
    mf[7] = static_cast<float>(ninDwi->measurementFrame[1][2]);
    mf[8] = static_cast<float>(ninDwi->measurementFrame[2][2]);
    for (unsigned int vi=0; vi<baseGlyph->xyzwNum; vi++) {
      ELL_3MV_MUL(tmp, mf, baseGlyph->xyzw + 4*vi);
      ELL_3V_COPY(baseGlyph->xyzw + 4*vi, tmp);
    }
  }

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
  
  // --------------------------------------------------

  Nrrd *ngrad, *nbmat;
  double bval;
  unsigned int *skip, skipNum;
  if (tenDWMRIKeyValueParse(&ngrad, &nbmat, &bval, &skip, &skipNum, ninDwi)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble parsing DWI info:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (skipNum) {
    fprintf(stderr, "%s: sorry, can't do DWI skipping in tenDwiGage", me);
    airMopError(mop); return 1;
  }
  gageKind *kindDwi = tenDwiGageKindNew();
  if (tenDwiGageKindSet(kindDwi, mmdwi, 1, bval, 10, ngrad, nbmat,
                        tenEstimate1MethodLLS,
                        tenEstimate2MethodPeled, 42)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble parsing DWI info:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  Deft::Volume *volDwi = new Deft::Volume(kindDwi, ninDwi);
  Deft::TriPlane *triplane = new Deft::TriPlane(volDwi);
  limnPolyDataVertexWindingFlip(baseGlyph);
  triplane->baseGlyph(baseGlyph);
  
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
  triplane->colorQuantity(Deft::colorDwiQuantityRgbLinear);
  triplane->colorQuantity(Deft::colorDwiQuantityB0);
  // HEY: you can eventually segfault if this isn't set here
  // shouldn't doing so be optional?
  triplane->alphaMaskQuantity(Deft::alphaMaskDwiQuantityConfidence);
  triplane->visible(false);
  scene->groupAdd(triplane);

  // HEY, WRONG: totally wrong place to be doing this
  /*
  triplane->glyph[0]->parmCopy(glyph);
  triplane0->glyph[0]->parmCopy(glyph);
  triplane1->glyph[0]->parmCopy(glyph);
  triplane->glyph[1]->parmCopy(glyph);
  triplane0->glyph[1]->parmCopy(glyph);
  triplane1->glyph[1]->parmCopy(glyph);
  triplane->glyph[2]->parmCopy(glyph);
  triplane0->glyph[2]->parmCopy(glyph);
  triplane1->glyph[2]->parmCopy(glyph);
  glyphUI->add(triplane->glyph[0]);
  glyphUI->add(triplane0->glyph[0]);
  glyphUI->add(triplane1->glyph[0]);
  glyphUI->add(triplane->glyph[1]);
  glyphUI->add(triplane0->glyph[1]);
  glyphUI->add(triplane1->glyph[1]);
  glyphUI->add(triplane->glyph[2]);
  glyphUI->add(triplane0->glyph[2]);
  glyphUI->add(triplane1->glyph[2]);
  */

  Deft::TriPlaneUI *planeUI = new Deft::TriPlaneUI(triplane, viewer);
  planeUI->show();

  // --------------------------------------------------

  Deft::HyperStreamline *hsline = new Deft::HyperStreamline(volDwi);
  hsline->lightingUse(false);
  hsline->colorQuantity(Deft::colorDwiQuantityRgbLinear);
  // hsline->alphaMaskQuantity(Deft::alphaMaskTenQuantityConfidence);
  hsline->alphaMask(false);
  hsline->twoSided(true);
  scene->objectAdd(hsline);
  Deft::HyperStreamlineUI *hslineUI = 
    new Deft::HyperStreamlineUI(hsline, NULL, viewer);

  // HEY, WRONG: totally wrong place to be doing this
  triplane->hsline[0]->parmCopy(hsline);
  triplane->hsline[1]->parmCopy(hsline);
  triplane->hsline[2]->parmCopy(hsline);
  hslineUI->add(triplane->hsline[0]);
  hslineUI->add(triplane->hsline[1]);
  hslineUI->add(triplane->hsline[2]);

  Deft::SeedPoint *seedpoint = new Deft::SeedPoint(volDwi, baseGlyph);
  seedpoint->positionSet(10, 32, 15.5);
  
  kparm[0] = 1.0;
  nrrdKernelSpecSet(ksp, nrrdKernelTent, kparm);
  seedpoint->kernel(gageKernel00, ksp);
  seedpoint->visible(true);
  scene->groupAdd(seedpoint);
  Deft::SeedPointUI *seedpointUI = new Deft::SeedPointUI(seedpoint, viewer);
  seedpoint->hsline->parmCopy(hsline);
  hslineUI->add(seedpoint->hsline);
  seedpointUI->show();

  hslineUI->show();

  // --------------------------------------------------

  fltk::flush();
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
