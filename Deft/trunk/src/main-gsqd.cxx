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
#include "ViewerUI.h"
#include "Plane.h"
#include "TensorGlyph.h"
#include "TensorGlyphUI.h"

#include <teem/dye.h>

/* BAD Gordon */
extern "C" {
  extern int
  limnPolyDataEdgeHalve(limnPolyData *pldOut, 
                        const limnPolyData *pldIn);
}

char *info = ("this might come in handy.");

int
main(int argc, char **argv) {
  hestOpt *hopt=NULL;
  hestParm *hparm;
  airArray *mop;
  char *me, *err;

  me = argv[0];
  double matScl[16], matRot[16], matTrn[16];
  double *din;
  unsigned int dnum;

  mop = airMopNew();
  /*
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);

  hparm->respFileEnable = AIR_TRUE;
  me = argv[0];
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, -1, &_nin, NULL,
             "list of input triple lists",
             &ninNum, NULL, nrrdHestNrrd);
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
  */

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
  Deft::ViewerUI *viewerUI = new Deft::ViewerUI(viewer);
  viewerUI->show();

  ELL_4M_SCALE_SET(matScl, 0.005, 0.005, 1.05);
  limnPolyData *pld = limnPolyDataNew();
  limnPolyDataCylinder(pld, 
                       (1 << limnPolyDataInfoRGBA)
                       | (1 << limnPolyDataInfoNorm),
                       20, AIR_TRUE);
  limnPolyDataTransform_d(pld, matScl);
  ELL_4M_ROTATE_X_SET(matRot, AIR_PI/2);
  limnPolyDataTransform_d(pld, matRot);
  Deft::PolyData *surf = new Deft::PolyData(pld, true);
  surf->wireframe(false);
  surf->normalsUse(true);
  surf->visible(true);
  scene->objectAdd(surf);
  
  pld = limnPolyDataNew();
  limnPolyDataCylinder(pld, 
                       (1 << limnPolyDataInfoRGBA)
                       | (1 << limnPolyDataInfoNorm),
                       20, AIR_TRUE);
  limnPolyDataTransform_d(pld, matScl);
  ELL_4M_ROTATE_Y_SET(matRot, AIR_PI/2);
  limnPolyDataTransform_d(pld, matRot);
  surf = new Deft::PolyData(pld, true);
  surf->wireframe(false);
  surf->normalsUse(true);
  surf->visible(true);
  scene->objectAdd(surf);
  
  pld = limnPolyDataNew();
  limnPolyDataCylinder(pld, 
                       (1 << limnPolyDataInfoRGBA)
                       | (1 << limnPolyDataInfoNorm),
                       20, AIR_TRUE);
  limnPolyDataTransform_d(pld, matScl);
  surf = new Deft::PolyData(pld, true);
  surf->wireframe(false);
  surf->normalsUse(true);
  surf->visible(true);
  scene->objectAdd(surf);

  pld = limnPolyDataNew();
  limnPolyDataPolarSphere(pld, 
                          (1 << limnPolyDataInfoRGBA)
                          | (1 << limnPolyDataInfoNorm),
                          300, 300);
  ELL_4M_SCALE_SET(matScl, 0.85, 0.85, 0.85);
  limnPolyDataTransform_d(pld, matScl);
  surf = new Deft::PolyData(pld, true);
  surf->wireframe(false);
  surf->normalsUse(true);
  surf->visible(true);
  // surf->flatShading(true);
  scene->objectAdd(surf);
  for (unsigned int vi=0; vi<pld->xyzwNum; vi++) {
    float x, y, z;
    unsigned char *rgba, uc;
    x = pld->xyzw[0 + 4*vi];
    y = pld->xyzw[1 + 4*vi];
    z = pld->xyzw[2 + 4*vi];
    rgba = pld->rgba + 4*vi;
    if (x >= y && y >= z) {
      double mode = airMode3(x, y, z);
      uc = AIR_CAST(unsigned char, AIR_AFFINE(-1, mode, 1, 0, 255));
      ELL_4V_SET(rgba, uc, uc, uc, 255);
      ELL_4V_SET(rgba,
                 255*(x > 0.0),
                 255*(y > 0.0),
                 255*(z > 0.0), 255);
    } else {
      if (z > 0.82) {
        ELL_4V_SET(rgba, 0, 0, 255, 255);
      } else {
        ELL_4V_SET(rgba, 90, 50, 90, 255);
      }
    }
  }

  pld = limnPolyDataNew();
  if (limnPolyDataAlloc(pld,
                        (1 << limnPolyDataInfoRGBA)
                        | (1 << limnPolyDataInfoNorm),
                        11,   /* vertNum */
                        3*12, /* indxNum */
                        1)) { /* primNum */
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble:%s", me, err);
    free(err);
    exit(1);
  }
  ELL_4V_SET(pld->xyzw + 4*0,  1/sqrt(3), 1/sqrt(3), 1/sqrt(3), 1);
  ELL_4V_SET(pld->xyzw + 4*1,  1, 0, 0, 1);
  ELL_4V_SET(pld->xyzw + 4*3,  1/sqrt(2), 1/sqrt(2), 0, 1);
  ELL_4V_SET(pld->xyzw + 4*7,  0, -1/sqrt(2), -1/sqrt(2), 1);
  ELL_4V_SET(pld->xyzw + 4*9,  0, 0, -1, 1);
  ELL_4V_SET(pld->xyzw + 4*10, -1/sqrt(3), -1/sqrt(3), -1/sqrt(3), 1);
  ELL_4V_SET(pld->xyzw + 4*2, 2/sqrt(5), 1/sqrt(5), 0, 1);
  ELL_4V_LERP(pld->xyzw + 4*4, 0.5, pld->xyzw + 4*1, pld->xyzw + 4*7);
  ELL_4V_LERP(pld->xyzw + 4*5, 0.5, pld->xyzw + 4*1, pld->xyzw + 4*9);
  ELL_4V_LERP(pld->xyzw + 4*6, 0.5, pld->xyzw + 4*3, pld->xyzw + 4*9);
  ELL_4V_SET(pld->xyzw + 4*8, 0, -1/sqrt(5), -2/sqrt(5), 1);
  for (unsigned int vi=0; vi<pld->xyzwNum; vi++) {
    float tmp;
    ELL_3V_NORM(pld->xyzw + 4*vi, pld->xyzw + 4*vi, tmp);
  }
  ELL_3V_SET(pld->indx + 3*0,  0, 1, 2);
  ELL_3V_SET(pld->indx + 3*1,  0, 2, 3);
  ELL_3V_SET(pld->indx + 3*2,  2, 1, 5);
  ELL_3V_SET(pld->indx + 3*3,  2, 5, 6);
  ELL_3V_SET(pld->indx + 3*4,  3, 2, 6);
  ELL_3V_SET(pld->indx + 3*5,  5, 1, 4);
  ELL_3V_SET(pld->indx + 3*6,  5, 4, 8);
  ELL_3V_SET(pld->indx + 3*7,  9, 5, 8);
  ELL_3V_SET(pld->indx + 3*8,  6, 5, 9);
  ELL_3V_SET(pld->indx + 3*9,  4, 7, 8);
  ELL_3V_SET(pld->indx + 3*10, 8, 7, 10);
  ELL_3V_SET(pld->indx + 3*11, 8, 10, 9);
  limnPolyDataColorSet(pld, 255, 255, 255, 255);
  pld->type[0] = limnPrimitiveTriangles;
  pld->icnt[0] = 3*12;
  if (limnPolyDataVertexNormals(pld)) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble:%s", me, err);
    free(err);
    exit(1);
  }
  surf = new Deft::PolyData(pld, true);
  surf->wireframe(true);
  surf->normalsUse(true);
  surf->visible(true);
  // surf->flatShading(true);
  scene->objectAdd(surf);
  
  limnPolyData *pldSplit = limnPolyDataNew();
  limnPolyDataEdgeHalve(pldSplit, pld);
  for (unsigned int vi=0; vi<pldSplit->xyzwNum; vi++) {
    float tmp;
    ELL_3V_NORM(pldSplit->xyzw + 4*vi, pldSplit->xyzw + 4*vi, tmp);
  }

  pld = pldSplit;
  pldSplit = limnPolyDataNew();
  limnPolyDataEdgeHalve(pldSplit, pld);
  for (unsigned int vi=0; vi<pldSplit->xyzwNum; vi++) {
    float tmp;
    tmp = ELL_3V_LEN(pldSplit->xyzw + 4*vi);
    ELL_3V_SCALE(pldSplit->xyzw + 4*vi, 1.1/tmp, pldSplit->xyzw + 4*vi);
  }

  Nrrd *nten; float *ten;
  nten = nrrdNew();
  nrrdMaybeAlloc_va(nten, nrrdTypeFloat, 2, 
                    AIR_CAST(size_t, 7),
                    AIR_CAST(size_t, pldSplit->xyzwNum));
  ten = AIR_CAST(float *, nten->data);
  for (unsigned int vi=0; vi<pldSplit->xyzwNum; vi++) {
    TEN_T_SET(ten + 7*vi, 1,
              0.0001 + AIR_ABS((pldSplit->xyzw + 4*vi)[0]), 0, 0,
              0.0001 + AIR_ABS((pldSplit->xyzw + 4*vi)[1]), 0,
              0.0001 + AIR_ABS((pldSplit->xyzw + 4*vi)[2]));
  }
  
  Deft::TensorGlyph *glyph = new Deft::TensorGlyph();
  glyph->dataSet(nten->axis[1].size,
                 (float *)nten->data, 7,
                 pldSplit->xyzw, 4, NULL);
  glyph->dynamic(false);
  glyph->anisoType(tenAniso_FA);
  glyph->anisoThreshMin(0.0);
  glyph->anisoThresh(0.0);
  glyph->flatShading(false);
  // glyph->wireframe(false);
  glyph->cleanEdge(false);
  glyph->glyphType(tenGlyphTypeSuperquad);
  glyph->superquadSharpness(3.0);
  glyph->glyphResolution(10);
  glyph->skipNegativeEigenvalues(false);
  // glyph->barycentricResolution(12);
  glyph->glyphScale(0.09);
  glyph->rgbEvecParmSet(tenAniso_Cl2, 0, 0.0, 1.0, 1, 0.0);
  glyph->visible(true);
  glyph->update();
  scene->objectAdd(glyph);

  Deft::TensorGlyphUI *glyphUI = new Deft::TensorGlyphUI(glyph, viewer);
  glyphUI->show();

  // this returns when the user quits
  int ret = fltk::run();

  return ret;
}
