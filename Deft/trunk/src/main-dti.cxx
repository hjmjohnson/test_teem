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
#include "PolyData.H"

#include <fltk/Group.h>
#include <fltk/Menu.h>
#include <fltk/PopupMenu.h>
#include <fltk/draw.h>
#include <fltk/Font.h>
#include <fltk/Choice.h>

bool running = false;

#define PST 0
#define TUMOR 0
#define LPLD 1

#if LPLD
typedef struct {
  Deft::Slider *radSlider, *ccSlider, *vertSlider;
  limnPolyData *orig, *tube;
  Deft::Viewer *viewer;
  Deft::PolyData *poly;
} lpldBag;

void
tuberad_cb(fltk::Widget *widget, void *data) {
  Deft::Slider *slider;
  lpldBag *lbag;

  slider = (Deft::Slider *)widget;
  lbag = (lpldBag *)data;

  limnPolyDataSpiralTubeWrap(lbag->tube, lbag->orig, 
                             limnPolyDataInfoBitFlag(lbag->orig),
                             NULL,
                             8, 3, slider->value());
  lbag->poly->changed();
  lbag->viewer->redraw();
}

void
ccselect_cb(fltk::Widget *widget, void *data) {
  char me[]="ccselect_cb";
  lpldBag *lbag;
  unsigned int primNum, primIdx, keep, needVert, keepNum;

  lbag = (lpldBag *)data;

  keepNum = 0;
  keep = AIR_CAST(unsigned int, lbag->ccSlider->value());
  needVert = AIR_CAST(unsigned int, lbag->vertSlider->value());
  primNum = lbag->tube->primNum;
  if (keep) {
    for (primIdx=0; primIdx<primNum; primIdx++) {
      if (primIdx < keep && (!needVert 
                             || lbag->tube->icnt[primIdx] > needVert)) {
        lbag->tube->type[primIdx] = limnPrimitiveTriangleStrip;
        keepNum++;
      }  else {
        lbag->tube->type[primIdx] = limnPrimitiveNoop;
      }
    }
  }
  fprintf(stderr, "!%s: keepNum = %u\n", me, keepNum);

  lbag->poly->changed();
  lbag->viewer->redraw();
}

#endif

#if PST
#include <teem/pull.h>

typedef struct {
  fltk::CheckButton *tractvis;
  fltk::Button *stepButton;
  Deft::Viewer *viewer;
  Deft::TensorGlyph *glyph;
  Deft::Slider *slider;
  Deft::HyperStreamline *hsline;
  pullContext *pctx;
  Nrrd *nPosOut, *nTenOut, *ntmp, *nten, *npos;
} pullBag;

#if TUMOR
/* forward decls for sake of isobag callback */
void outputGet(pullBag *pbag);
void outputShow(pullBag *pbag);
#endif

#endif

char *info = ("this might come in handy.");

#if TUMOR
typedef struct {
  fltk::CheckButton *visible, *wireframe, *colorDo;
  Deft::Slider *slider;
  Deft::Contour *contour;
  Deft::Scene *scene;
  Deft::Viewer *viewer;
  fltk::Choice *color;
#if PST
  pullContext *pctx;
  Nrrd *nvert;
  pullBag *pbag;
#endif
} isoBag;

int grads; char *gradsStr;


void
isovalue_cb(fltk::Widget *widget, void *data) {
  char me[]="isovalue_cb", *err;
  Deft::Slider *slider;
  isoBag *ibag;
  const limnPolyData *lpld;

  slider = (Deft::Slider *)widget;
  ibag = (isoBag *)data;
  fprintf(stderr, "!%s: visible = %s ************* %p\n", me, 
          ibag->contour->visible() ? "true" : "false",
          dynamic_cast<Deft::PolyProbe*>(ibag->contour)->volume());
  ibag->contour->extract(slider->value());
  dynamic_cast<Deft::PolyProbe*>(ibag->contour)->update(true);
#if PST
  fprintf(stderr, "%s: grads = %s = %d\n", me, gradsStr, grads);
  if (running && !grads) {
    /* if (running && ibag->pbag->hsline->visible()) {*/
    lpld = ibag->contour->lpld();
    Nrrd *ntmp = nrrdNew();
    if (nrrdWrap_va(ntmp, AIR_CAST(void *, lpld->xyzw),
                    nrrdTypeFloat, 2, 4, lpld->xyzwNum)
        || nrrdCopy(ibag->nvert, ntmp)) {
      err = biffGetDone(NRRD);
      fprintf(stderr, "%s: trouble:%s", me, err);
      free(err);
      exit(1);
    }
    /* this is setting pctx->npos == ibag->nvert */
    float *xyzw = AIR_CAST(float *, ibag->nvert->data);
    for (unsigned int vi=0; vi<lpld->xyzwNum; vi++) {
      xyzw[3 + 4*vi] = 0;
    }
    ibag->pctx->ispec[pullInfoIsovalue]->zero = slider->value();
    if (pullStart(ibag->pctx)) {
      err = biffGetDone(PULL);
      fprintf(stderr, "%s: trouble:%s", me, err);
      free(err);
      exit(1);
    }
    outputGet(ibag->pbag);
    outputShow(ibag->pbag);
  }
#endif
  ibag->viewer->redraw();
}

void
isovisible_cb(fltk::Widget *widget, void *data) {
  char me[]="isovisible_cb";
  isoBag *bag;

  bag = (isoBag *)data;
  fprintf(stderr, "!%s: visible = %s\n", me, 
          bag->contour->visible() ? "true" : "false");
  bag->contour->visible(bag->visible->value());
  bag->viewer->redraw();
}

void
isowire_cb(fltk::Widget *widget, void *data) {
  // char me[]="isovisible_cb";
  isoBag *bag;

  bag = (isoBag *)data;
  bag->contour->wireframe(bag->wireframe->value());
  bag->viewer->redraw();
}

void
colordo_cb(fltk::Widget *, void *data) {
  // char me[]="colordo_cb";
  isoBag *bag;

  bag = (isoBag *)data;
  Deft::PolyProbe *cprobe = dynamic_cast<Deft::PolyProbe*>(bag->contour);
  cprobe->color(bag->colorDo->value());
  dynamic_cast<Deft::PolyProbe*>(bag->contour)->update(false);
  bag->viewer->redraw();
}

void
color_cb(fltk::Widget *, void *data) {
  char me[]="color_cb";
  isoBag *bag;
  unsigned int qi;

  bag = (isoBag *)data;
  Deft::PolyProbe *cprobe = dynamic_cast<Deft::PolyProbe*>(bag->contour);
  const airEnum *enm = cprobe->colorQuantityEnum();
  int itu = airEnumUnknown(enm);
  for (qi=itu+1;
       strcmp(bag->color->item()->label(), airEnumStr(enm, qi));
       qi++);
  fprintf(stderr, "!%s: got %u %s\n", me, qi, airEnumStr(enm, qi));
  cprobe->colorQuantity(qi);
  dynamic_cast<Deft::PolyProbe*>(bag->contour)->update(false);
  bag->viewer->redraw();
}
#endif

#if PST

void
pstgsc_cb(fltk::Widget *widget, void *data) {
  char me[]="pstgsc_cb";
  Deft::Slider *slider;
  pullBag *bag;

  slider = (Deft::Slider *)widget;
  bag = (pullBag *)data;
  bag->glyph->glyphScale(slider->value()*bag->pctx->radiusSpace);
  // bag->glyph->glyphScale(bag->pctx->radiusSpace);
  // bag->glyph->glyphScale(1.0);
  bag->glyph->update();
  bag->viewer->redraw();
}

void
tractvis_cb(fltk::Widget *, void *data) {
  char me[]="tractvis_cb";
  pullBag *bag;

  bag = (pullBag *)data;
  fprintf(stderr, "!%s: visible = %s\n", me, 
          bag->tractvis->visible() ? "true" : "false");
  bag->hsline->visible(bag->tractvis->value());
  bag->viewer->redraw();
}

void
outputGet(pullBag *pbag) {
  char me[]="outputGet", *err;
  size_t cropMin[2], cropMax[2];

  fprintf(stderr, "!%s: hello, running = %s\n", me,
          running ? "true" : "false");
  if (!running) {
    return;
  }
  if (pullOutputGet(pbag->nPosOut, pbag->nTenOut, pbag->pctx)) {
    err = biffGetDone(PULL);
    fprintf(stderr, "%s: error getting pull output:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  cropMin[0] = 0;
  cropMin[1] = 0;
  cropMax[0] = 2;
  cropMax[1] = pbag->nPosOut->axis[1].size-1;
  if (nrrdConvert(pbag->nten, pbag->nTenOut, nrrdTypeFloat)
      || nrrdCrop(pbag->ntmp, pbag->nPosOut, cropMin, cropMax)
      || nrrdConvert(pbag->npos, pbag->ntmp, nrrdTypeFloat)) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: another error 0\n%s\n", me, err);
    free(err);
    exit(1);
  }
  fprintf(stderr, "!%s: bye\n", me);
}

void
outputShow(pullBag *pbag) {
  char me[]="outputShow";

  fprintf(stderr, "!%s: hello, running = %s\n", me,
          running ? "true" : "false");
  if (!running) {
    return;
  }
  if (pbag->nPosOut->axis[1].size) {
    pbag->glyph->dataSet(pbag->nPosOut->axis[1].size,
                        (float*)pbag->nten->data, 7,
                        (float*)pbag->npos->data, 3, NULL);
    pbag->glyph->update();
    pbag->hsline->seedsSet(pbag->npos);
    pbag->hsline->update();
    pbag->viewer->redraw();
    fltk::flush();
  } else {
    fprintf(stderr, "!%s: got zero tensors out!\n", me);
  }
  return;
}

void
step_cb(fltk::Widget *, pullBag *pbag) {
  /*  static double lastthresh = -42; */
  char me[]="step_cb", *err;
  static unsigned int itersTotal=0;

  pbag->pctx->iterMax += 1;
  if (pullRun(pbag->pctx)) {
    err = biffGetDone(PULL);
    fprintf(stderr, "%s: error running pull:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  itersTotal += 1;
  fprintf(stderr, "!%s: enr = %g; time = %g sec; %u iters (%g iters/sec)\n",
          me, pbag->pctx->energy, pbag->pctx->timeRun, itersTotal,
          itersTotal/pbag->pctx->timeRun);
  outputGet(pbag);
  outputShow(pbag);
}

const gageKind *
parseKind(char *str) {
  const gageKind *ret = NULL;

  if (!str) {
    return NULL;
  }
  airToLower(str);
  if (!strcmp(gageKindScl->name, str)) {
    ret = gageKindScl;
  } else if (!strcmp(gageKindVec->name, str)) {
    ret = gageKindVec;
  } else if (!strcmp(tenGageKind->name, str)) {
    ret = tenGageKind;
  }
  return ret;
}
/* volume specifications ---------------------------------------------
** 
** format: <filename>:<kind>:<vname>[:<scalemax>:<scalesteps>]
*/
typedef struct {
  Nrrd *nin;
  const gageKind *kind;
  char *vname;
} pVolSpec;

int
pVolSpecParse(void *ptr, char *_str, char err[AIR_STRLEN_HUGE]) {
  char me[]="pVolSpecParse", *last=NULL, *tok, *lerr, *str;
  pVolSpec **vspecP, *vspec;
  airArray *mop;
  
  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  vspecP = (pVolSpec **)ptr;

  mop = airMopNew();
  *vspecP = vspec = AIR_CAST(pVolSpec *, calloc(1, sizeof(pVolSpec)));
  if (!vspec) {
    sprintf(err, "%s: couldn't alloc vspec", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, vspec, airFree, airMopOnError);
  if (!(str = airStrdup(_str))) {
    sprintf(err, "%s: couldn't strdup input", me);
    airMopError(mop); return 1; 
  }
  airMopAdd(mop, str, airFree, airMopOnError);
  if (3 != airStrntok(str, ":")) {
    sprintf(err, "%s: \"%s\" not of form <filename>:<kind>:<vname>", me, str);
    airMopError(mop); return 1;
  }
  vspec->nin = nrrdNew();
  airMopAdd(mop, vspec->nin, (airMopper)nrrdNuke, airMopOnError);
  
  tok = airStrtok(str, ":", &last);
  if (nrrdLoad(vspec->nin, tok, NULL)) {
    airMopAdd(mop, lerr = biffGetDone(NRRD), airFree, airMopAlways);
    sprintf(err, "%s: trouble loading seed volume:\n%s\n", me, lerr);
    airMopError(mop); return 1;
  }
  tok = airStrtok(NULL, ":", &last);
  if (!(vspec->kind = parseKind(tok))) {
    sprintf(err, "%s: couldn't parse \"%s\" as kind", me, tok);
    airMopError(mop); return 1;
  }
  tok = airStrtok(NULL, ":", &last);
  if (!(vspec->vname = airStrdup(tok))) {
    sprintf(err, "%s: couldn't strdup vname", me);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
void *
pVolSpecDestroy(void *ptr) {
  pVolSpec *vspec;
  
  if (ptr) {
    vspec = AIR_CAST(pVolSpec *, ptr);
    nrrdNuke(vspec->nin);
    free(vspec->vname);
    free(vspec);
  }
  return NULL;
}
hestCB pVolSpecHestCB = {
  sizeof(pVolSpec *),
  "vspec",
  pVolSpecParse,
  pVolSpecDestroy
}; 

/* info definitions ----------------------------------------------
**
** format: <info>:<vname>:<item>[:<zero>:<scale>]
*/
typedef struct {
  int info;
  char *vname, *itemStr;
  double zero, scale;
} pInfoDef;
int
pInfoDefParse(void *ptr, char *_str, char err[AIR_STRLEN_HUGE]) {
  char me[]="pInfoDefParse", *last=NULL, *tok, *str;
  airArray *mop;
  pInfoDef **idefP, *idef;

  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  idefP = (pInfoDef **)ptr;

  mop = airMopNew();
  *idefP = idef = AIR_CAST(pInfoDef *, calloc(1, sizeof(pInfoDef)));
  if (!idef) {
    sprintf(err, "%s: couldn't alloc idef", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, idef, airFree, airMopOnError);
  if (!(str = airStrdup(_str))) {
    sprintf(err, "%s: couldn't strdup input", me);
    airMopError(mop); return 1; 
  }
  airMopAdd(mop, str, airFree, airMopOnError);
  if (!( 3 == airStrntok(str, ":")
         || 5 == airStrntok(str, ":") )) {
    sprintf(err, "%s: \"%s\" not of form "
            "<info>:<vname>:<item>[:<zero>:<scale>]", me, str);
    airMopError(mop); return 1;
  }
  
  /* <info>:<vname>:<item>[:<zero>:<scale>] */
  tok = airStrtok(str, ":", &last);
  if (!(idef->info = airEnumVal(pullInfo, tok))) {
    sprintf(err, "%s: couldn't parse \"%s\" as %s", me, tok, pullInfo->name);
    airMopError(mop); return 1;
  }
  tok = airStrtok(NULL, ":", &last);
  idef->vname = airStrdup(tok);
  airMopAdd(mop, idef->vname, airFree, airMopOnError);
  tok = airStrtok(NULL, ":", &last);
  idef->itemStr = airStrdup(tok);
  airMopAdd(mop, idef->itemStr, airFree, airMopOnError);
  if (5 == airStrntok(_str, ":")) {
    tok = airStrtok(NULL, ":", &last);
    if (1 != sscanf(tok, "%lf", &(idef->zero))) {
      sprintf(err, "%s: couldn't parse %s as zero (double)", me, tok);
      airMopError(mop); return 1;
    }
    tok = airStrtok(NULL, ":", &last);
    if (1 != sscanf(tok, "%lf", &(idef->scale))) {
      sprintf(err, "%s: couldn't parse %s as scale (double)", me, tok);
      airMopError(mop); return 1;
    }
  } else {
    idef->zero = idef->scale = AIR_NAN;
  }
  
  mop = airMopNew();
  airMopOkay(mop);
  return 0;
}
void *
pInfoDefDestroy(void *ptr) {
  pInfoDef *idef;

  if (ptr) {
    idef = AIR_CAST(pInfoDef *, ptr);
    free(idef->vname);
    free(idef->itemStr);
    free(idef);
  }
  return NULL;
}
hestCB pInfoDefHestCB = {
  sizeof(pInfoDef *),
  "idef",
  pInfoDefParse,
  pInfoDefDestroy
}; 
#endif

int
main(int argc, char **argv) {
  hestOpt *hopt=NULL;
  airArray *mop;
  char *me, *err;

  float fr[3], at[3], up[3], fovy, neer, faar, dist, bg[3],
    anisoThresh, anisoThreshMin, glyphScale, sqdSharp;
  int sdaq, size[2], ortho, rght, atrel, glyphType, glyphFacetRes;
  unsigned int baryRes;
  Nrrd *_nin=NULL, *nin=NULL, *nPos=NULL, *nseed=NULL;
#if 0*3
  Nrrd *_ninBlur=NULL, *ninBlur;
  limnPolyData *pldRid, *pldVal;
#endif
#if 0*4
  Nrrd *nb0;
#endif
#if TUMOR
  isoBag ibag;
  Nrrd *ntumor;
#endif
#if LPLD
  lpldBag lbag;
#endif
#if PST
  pullBag pbag;
  pullContext *pctx;
  pullEnergySpec *ensp;
  NrrdKernelSpec *k00, *k11, *k22;
  pVolSpec **vspec;
  pInfoDef **idef;
  unsigned int vsi, vspecNum, idi, idefNum;
  int E, constrInfo;
#endif
#if LPLD
  char *lpldS;
  limnPolyData *lpld=NULL;
#endif
  int aniso, camkeep, ret;

  mop = airMopNew();

#if TUMOR
  nrrdGetenvBool(&grads, &gradsStr, "GRADS");
#endif

  me = argv[0];
  hestOptAdd(&hopt, "sdaq", NULL, airTypeBool, 0, 0, &sdaq, NULL,
             "screen dump and quit");
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &_nin, NULL,
             "input tensor volume",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "seed", "nin", airTypeOther, 1, 1, &nseed, "",
             "tractography seeds for tensor volume",
             NULL, NULL, nrrdHestNrrd);
#if PST
  pctx = pullContextNew();
  airMopAdd(mop, pctx, (airMopper)pullContextNix, airMopAlways);
  hestOptAdd(&hopt, "vol", "vol0 vol1", airTypeOther, 1, -1, &vspec, NULL,
             "input volumes, in format <filename>:<kind>:<vname>",
             &vspecNum, NULL, &pVolSpecHestCB);
  hestOptAdd(&hopt, "info", "info0 info1", airTypeOther, 1, -1, &idef, NULL,
             "info definitions, in format "
             "<info>:<vname>:<item>[:<zero>:<scale>]",
             &idefNum, NULL, &pInfoDefHestCB);
  hestOptAdd(&hopt, "constr", "info", airTypeEnum, 1, 1, &constrInfo, "sthr",
             "info to use as constraint, or use \"sthr\" as hack to "
             "indicate no constraint", NULL, pullInfo);
  hestOptAdd(&hopt, "ipe", "spec", airTypeOther, 1, 1, &ensp, "quartic",
             "specification of inter-particle energy function to use",
             NULL, NULL, pullHestEnergySpec);
  hestOptAdd(&hopt, "k00", "kern00", airTypeOther, 1, 1, &k00,
             "cubic:1,0", "kernel for gageKernel00",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kern11", airTypeOther, 1, 1, &k11,
             "cubicd:1,0", "kernel for gageKernel11",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kern22", airTypeOther, 1, 1, &k22,
             "cubicdd:1,0", "kernel for gageKernel22",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "np", "# points", airTypeUInt, 1, 1,
             &(pctx->pointNumInitial), "1000",
             "number of points to start in simulation");
  hestOptAdd(&hopt, "ppv", "# pnts/vox", airTypeUInt, 1, 1,
             &(pctx->pointPerVoxel), "0",
             "number of points per voxel to start in simulation "
             "(need to have a seed thresh vol, overrides \"-np\")");
  hestOptAdd(&hopt, "step", "step", airTypeDouble, 1, 1,
             &(pctx->stepInitial), "1",
             "initial step size for gradient descent");
  hestOptAdd(&hopt, "csm", "step", airTypeDouble, 1, 1,
             &(pctx->constraintStepMin), "0.0001",
             "convergence criterion for constraint satisfaction");
  hestOptAdd(&hopt, "maxi", "# iters", airTypeUInt, 1, 1,
             &(pctx->iterMax), "0",
             "if non-zero, max # iterations to run whole system");
  hestOptAdd(&hopt, "irad", "scale", airTypeDouble, 1, 1,
             &(pctx->radiusSpace), "1",
             "particle radius in spatial domain");
  hestOptAdd(&hopt, "alpha", "alpha", airTypeDouble, 1, 1,
             &(pctx->alpha), "0.5",
             "blend between particle-image (alpha=0) and "
             "inter-particle (alpha=1) energies");
  hestOptAdd(&hopt, "jitt", "jitter", airTypeDouble, 1, 1, 
             &(pctx->jitter), "1.0",
             "how much jittering to do for per-voxel seeding");
  hestOptAdd(&hopt, "wall", "k", airTypeDouble, 1, 1,
             &(pctx->wall), "1.0",
             "spring constant on walls");
  hestOptAdd(&hopt, "ess", "scl", airTypeDouble, 1, 1, 
             &(pctx->stepScale), "0.5",
             "when energy goes up instead of down, scale step "
             "size by this");
  hestOptAdd(&hopt, "oss", "scl", airTypeDouble, 1, 1, 
             &(pctx->opporStepScale), "1.0",
             "opportunistic scaling (hopefully up, >1) of step size "
             "on every iteration");
  hestOptAdd(&hopt, "rng", "seed", airTypeUInt, 1, 1, 
             &(pctx->rngSeed), "42",
             "base seed value for RNGs");
#endif
#if LPLD
  hestOptAdd(&hopt, "ipd", "lpld", airTypeString, 1, 1, &lpldS, "",
             "random polydata");
#endif
#if 0*3
  hestOptAdd(&hopt, "ib", "nin", airTypeOther, 1, 1, &_ninBlur, NULL,
             "input tensor volume, blurred a titch",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "lvi", "pld", airTypeOther,   1, 1, &pldVal, "",
             "input polydata",
             NULL, NULL, limnHestPolyDataLMPD);
  hestOptAdd(&hopt, "lri", "pld", airTypeOther,   1, 1, &pldRid, "",
             "input polydata",
             NULL, NULL, limnHestPolyDataLMPD);
#endif
#if 0*4
  hestOptAdd(&hopt, "b0", "nin", airTypeOther, 1, 1, &nb0, NULL,
             "B0 volume",
             NULL, NULL, nrrdHestNrrd);
#endif
#if TUMOR
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
  hestOptAdd(&hopt, "br", "barycentric res", airTypeInt, 1, 1, &baryRes, "50",
             "resolution of sampling of tensor shape palette");
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

  int incy, winy = 0;
  if (nPos) {
    if (!(2 == _nin->dim
          && nrrdTypeFloat == _nin->type
          && 7 == _nin->axis[0].size
          && 2 == nPos->dim
          && nrrdTypeFloat == nPos->type
          && 3 == nPos->axis[0].size)) {
      fprintf(stderr, "%s: try harder %d %d %d %d %d %d\n", me,
              !!(2 == _nin->dim),
              !!(nrrdTypeFloat == _nin->type),
              !!(7 == _nin->axis[0].size),
              !!(2 == nPos->dim),
              !!(nrrdTypeFloat == nPos->type),
              !!(3 == nPos->axis[0].size));
      airMopError(mop);
      exit(1);
    }
    fprintf(stderr, "!%s: npos okay!\n", me);
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

#if LPLD
  if (airStrlen(lpldS)) {
    FILE *file;
    if (!(file = airFopen(lpldS, stdin, "rb"))) {
      fprintf(stderr, "%s: couldn't open \"%s\" for reading\n", me, lpldS);
      airMopError(mop); return 1;
    }
    lpld = limnPolyDataNew();
    airMopAdd(mop, lpld, (airMopper)limnPolyDataNix, airMopAlways);
    if (!strcmp("-", lpldS) || airEndsWith(lpldS, ".lmpd")) {
      if (0) {
        ret = (limnPolyDataReadLMPD(lpld, file)
               || limnPolyDataVertexWindingFix(lpld, AIR_TRUE)
               || limnPolyDataVertexNormals(lpld));
      } else {
        ret = limnPolyDataReadLMPD(lpld, file);
      }
    } else {
      ret = (limnPolyDataReadOFF(lpld, file)
             || limnPolyDataVertexWindingFix(lpld, AIR_TRUE)
             || limnPolyDataVertexNormals(lpld));
    }
    if (ret) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble reading \"%s\":\n%s", me, lpldS, err);
      airMopError(mop); return 1;
    }

    if ((1 << limnPrimitiveLineStrip) == limnPolyDataPrimitiveTypes(lpld)) {
      fltk::Window *lwin = new fltk::Window(size[0]+20, 0, 400, 140);
      lwin->begin();
      lwin->resizable(lwin);

      lbag.orig = lpld;
      lbag.tube = limnPolyDataNew();
      lbag.viewer = viewer;
      
      winy = 0;
      lbag.radSlider = new Deft::Slider(0, winy, lwin->w(), incy=40,
                                        "Tube Radius");
      lbag.radSlider->align(fltk::ALIGN_LEFT);
      lbag.radSlider->range(0.0, 2.0);
      lbag.radSlider->fastUpdate(1);
      lbag.radSlider->value(0.3);
      limnPolyDataSpiralTubeWrap(lbag.tube, lbag.orig, 
                                 limnPolyDataInfoBitFlag(lpld),
                                 NULL,
                                 8, 3, lbag.radSlider->value());
      lbag.radSlider->callback(tuberad_cb, &lbag);

      winy += incy;
      lbag.ccSlider = new Deft::Slider(0, winy, lwin->w(), incy=40,
                                       "CC Select");
      lbag.ccSlider->align(fltk::ALIGN_LEFT);
      lbag.ccSlider->range(0, 1000);
      lbag.ccSlider->fastUpdate(1);
      lbag.ccSlider->value(0);
      lbag.ccSlider->step(1);
      lbag.ccSlider->callback(ccselect_cb, &lbag);

      winy += incy;
      lbag.vertSlider = new Deft::Slider(0, winy, lwin->w(), incy=40,
                                       "vert Select");
      lbag.vertSlider->align(fltk::ALIGN_LEFT);
      lbag.vertSlider->range(0, 1000);
      lbag.vertSlider->fastUpdate(1);
      lbag.vertSlider->value(0);
      lbag.vertSlider->step(1);
      lbag.vertSlider->callback(ccselect_cb, &lbag);

      lwin->end();
      lwin->show(argc,argv);

      lbag.poly = new Deft::PolyData(lbag.tube);
      lbag.poly->lightingUse(true);
      lbag.poly->flatShading(false);
      lbag.poly->twoSided(true);
      lbag.poly->normalsUse(true);
      lbag.poly->visible(true);
      scene->objectAdd(lbag.poly);
    }
  }
#endif

  Deft::Volume *vol;
  if (!nPos) {
    vol = new Deft::Volume(tenGageKind, nin);
    fprintf(stderr, "!%s: vol = %p *********************\n", me, vol);
  } else {
    vol = NULL;
  }

  Deft::TensorGlyph *glyph = new Deft::TensorGlyph();
  if (nPos) {
    glyph->dataSet(nin->axis[1].size,
                   (float *)nin->data, 7,
                   (float *)nPos->data, 3, NULL);
  } else {
    glyph->volumeSet(nin);
  }
  glyph->dynamic(false);
  glyph->twoSided(true);
  glyph->anisoType(aniso);
  // glyph->maskThresh(0.5);
  glyph->anisoThreshMin(anisoThreshMin);
  glyph->anisoThresh(anisoThresh);
  // glyph->wireframe(false);
  // glyph->cleanEdge(true);
  glyph->glyphType(glyphType);
  glyph->superquadSharpness(sqdSharp);
  glyph->glyphResolution(glyphFacetRes);
  if (tenGlyphTypeBetterquad) {
    glyph->barycentricResolution(baryRes);
  } else {
    glyph->barycentricResolution(20);
  }
  glyph->skipNegativeEigenvalues(false);
  glyph->glyphScale(glyphScale);
  glyph->rgbEvecParmSet(tenAniso_Cl2, 0, 0.7, 1.0, 2.3, 1.0);
  glyph->visible(false);
  glyph->rgbUse(false);
  glyph->update();
  /*
  void rgbEvecParmSet(int aniso, unsigned int evec,
                      double maxSat, double isoGray,
                      double gamma, double modulate);
  */

  scene->objectAdd(glyph);

  Deft::TensorGlyphUI *glyphUI = new Deft::TensorGlyphUI(glyph, viewer);
  glyphUI->show();

  // --------------------------------------------------

  if (vol) {
    Deft::TriPlane *triplane = new Deft::TriPlane(vol);
    // HEY: you can eventually segfault if this isn't set here
    // shouldn't doing so be optional?
    triplane->alphaMaskQuantity(Deft::alphaMaskTenQuantityConfidence);
    fprintf(stderr, "!%s: setting %s %s = %u\n", me, 
            Deft::colorTenQuantity->name,
            airEnumStr(Deft::colorTenQuantity, Deft::colorTenQuantityRgbLinear),
            Deft::colorTenQuantityRgbLinear);
    triplane->colorQuantity(Deft::colorTenQuantityRgbLinear);
    fprintf(stderr, "!%s: done\n", me);
    
    NrrdKernelSpec *ksp = nrrdKernelSpecNew();
    double kparm[10];
    
    kparm[0] = 1.0;
    nrrdKernelSpecSet(ksp, nrrdKernelTent, kparm);
    nrrdKernelSpecSet(ksp, nrrdKernelBox, kparm); // Brett
    triplane->kernel(gageKernel00, ksp);
    ELL_3V_SET(kparm, 1, 1, 0);
    nrrdKernelSpecSet(ksp, nrrdKernelBCCubicD, kparm);
    triplane->kernel(gageKernel11, ksp);
    nrrdKernelSpecSet(ksp, nrrdKernelBCCubicDD, kparm);
    triplane->kernel(gageKernel22, ksp);
    triplane->visible(false);
    
    scene->groupAdd(triplane);
    
    // HEY, WRONG: totally wrong place to be doing this
    if (1) {
      Deft::TensorGlyph *tgl[3];
      tgl[0] = static_cast<Deft::TensorGlyph*>(triplane->glyph[0]);
      tgl[1] = static_cast<Deft::TensorGlyph*>(triplane->glyph[1]);
      tgl[2] = static_cast<Deft::TensorGlyph*>(triplane->glyph[2]);
      tgl[0]->parmCopy(glyph);
      tgl[1]->parmCopy(glyph);
      tgl[2]->parmCopy(glyph);
      glyphUI->add(tgl[0]);
      glyphUI->add(tgl[1]);
      glyphUI->add(tgl[2]);
    }

#if 0
    triplane->alphaMaskThreshold(-1); // Brett
    triplane->sampling(0, 2); // Brett
    triplane->sampling(1, 2); // Brett
    triplane->sampling(2, 2); // Brett
    // triplane->plane[0]->visible(true); // Brett cor
    // triplane->position(0, 44.3); // Brett cor
#endif
    
    Deft::TriPlaneUI *planeUI = new Deft::TriPlaneUI(triplane, viewer);
    planeUI->show();

    // --------------------------------------------------
    Deft::HyperStreamline *hsline = new Deft::HyperStreamline(vol);
    hsline->lightingUse(false);
    hsline->colorQuantity(Deft::colorTenQuantityRgbLinear);
    hsline->alphaMaskQuantity(Deft::alphaMaskTenQuantityConfidence);
    hsline->alphaMask(false);
    hsline->twoSided(true);
    if (nseed) {
      hsline->seedsSet(nseed);
    }
    scene->objectAdd(hsline);
    Deft::HyperStreamlineUI *hslineUI = 
      new Deft::HyperStreamlineUI(hsline, glyph, viewer);
    
    // HEY, WRONG: totally wrong place to be doing this
    triplane->hsline[0]->parmCopy(hsline);
    triplane->hsline[1]->parmCopy(hsline);
    triplane->hsline[2]->parmCopy(hsline);
    hslineUI->add(triplane->hsline[0]);
    hslineUI->add(triplane->hsline[1]);
    hslineUI->add(triplane->hsline[2]);

    hslineUI->show();
  }

    // --------------------------------------------------

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

  Deft::Volume *volBlur = new Deft::Volume(tenGageKind, ninBlur);

  Deft::Anisofeature *anifeatR;
  if (pldRid) {
    anifeatR = new Deft::Anisofeature(volBlur, pldRid);
    fprintf(stderr, "!%s: hacking data pldRid %p\n", me, pldRid);
  } else {
    anifeatR = new Deft::Anisofeature(volBlur, NULL);
    anifeatR->type(seekTypeRidgeSurface);
    anifeatR->itemStrength(tenGageFARidgeSurfaceStrength);
    anifeatR->itemGradient(tenGageFAGradVec);
    anifeatR->itemEigensystem(tenGageFAHessianEval, tenGageFAHessianEvec);
  }
  anifeatR->colorQuantity(Deft::colorTenQuantityRgbLinear);
  anifeatR->alphaMaskQuantity(Deft::alphaMaskTenQuantityFARidgeSurfaceStrength);
  Deft::AnisofeatureUI *anifeUIR = 
    new Deft::AnisofeatureUI(anifeatR, viewer);
  scene->objectAdd(anifeatR);
  anifeUIR->show();

  // --------------------------------------------------

  Deft::Anisofeature *anifeatV;
  if (pldVal) {
    anifeatV = new Deft::Anisofeature(volBlur, pldVal);
    fprintf(stderr, "!%s: hacking data pldVal %p\n", me, pldVal);
  } else {
    anifeatV = new Deft::Anisofeature(volBlur, NULL);
    anifeatV->type(seekTypeValleySurface);
    anifeatV->itemStrength(tenGageFAValleySurfaceStrength);
    anifeatV->itemGradient(tenGageFAGradVec);
    anifeatV->itemEigensystem(tenGageFAHessianEval, tenGageFAHessianEvec);
  }
  anifeatV->colorQuantity(Deft::colorTenQuantityRgbLinear);
  anifeatV->alphaMaskQuantity(Deft::alphaMaskTenQuantityFAValleySurfaceStrength);
  Deft::AnisofeatureUI *anifeUIV = 
    new Deft::AnisofeatureUI(anifeatV, viewer);
  scene->objectAdd(anifeatV);
  anifeUIV->show();

#endif /* 0*3 */

  // --------------------------------------------------

#if 0*8
  if (vol) {
    Deft::Anisocontour *anicont = new Deft::Anisocontour(vol);
    anicont->colorQuantity(Deft::colorTenQuantityRgbLinear);
    anicont->alphaMask(true);
    // HEY: without this, the UI menu would come up blank because no there
    // was no value
    anicont->alphaMaskQuantity(Deft::alphaMaskTenQuantityConfidence);
    anicont->visible(false);
    fprintf(stderr, "!%s: anicont->alphaMaskQuantity() = %u\n", me, 
            anicont->alphaMaskQuantity());
    anicont->update();
    
    Deft::AnisocontourUI *anisoUI = 
      new Deft::AnisocontourUI(anicont, viewer);
    scene->objectAdd(anicont);
    anisoUI->show();
  }
#endif

  // --------------------------------------------------

#if TUMOR
  ibag.scene = scene;
  ibag.viewer = viewer;
  ibag.contour = new Deft::Contour();
  ibag.contour->volumeSet(ntumor);
  ibag.contour->visible(true);
  ibag.contour->wireframe(true);
  ibag.contour->twoSided(true);
  ibag.contour->brightness(1.2);
  ibag.scene->objectAdd(ibag.contour);
#if PST
  ibag.pctx = pctx;
  ibag.nvert = nrrdNew();
  if (grads) {
    ibag.pctx->npos = NULL;
  } else {    
    ibag.pctx->npos = ibag.nvert;
  }
  ibag.pbag = &pbag;
#endif

  hsline->probeVol(ntumor, gageKindScl);
  hsline->probeItem(gageSclValue);
  hsline->probeMeasr(nrrdMeasureMin);

  Deft::PolyProbe *cprobe = dynamic_cast<Deft::PolyProbe*>(ibag.contour);
  cprobe->volume(vol);
  cprobe->color(true);
  cprobe->colorQuantity(Deft::colorTenQuantityConf);
  cprobe->alphaMask(false);
  cprobe->alphaMaskQuantity(Deft::alphaMaskTenQuantityConfidence);
  ELL_3V_SET(kparm, 1, 0.5, 0);
  nrrdKernelSpecSet(ksp, nrrdKernelBCCubic, kparm);
  cprobe->kernel(gageKernel00, ksp);
  ELL_3V_SET(kparm, 1, 1, 0);
  nrrdKernelSpecSet(ksp, nrrdKernelBCCubicD, kparm);
  cprobe->kernel(gageKernel11, ksp);
  nrrdKernelSpecSet(ksp, nrrdKernelBCCubicDD, kparm);
  cprobe->kernel(gageKernel22, ksp);

  fltk::Window *iwin = new fltk::Window(size[0]+20, 0, 400, 140);
  iwin->begin();
  iwin->resizable(iwin);

  winy = 0;
  ibag.visible = new fltk::CheckButton(0, winy, 50, incy=20, "Show");
  ibag.visible->value(ibag.contour->visible());
  ibag.visible->callback(isovisible_cb, &ibag);

  ibag.wireframe = new fltk::CheckButton(50, winy, 50, incy=20, "Wire");
  ibag.wireframe->value(ibag.contour->wireframe());
  ibag.wireframe->callback(isowire_cb, &ibag);

  ibag.colorDo = new fltk::CheckButton(100, winy, 60, incy, "Color");
  ibag.colorDo->callback((fltk::Callback*)colordo_cb, &ibag);
  ibag.colorDo->value(true);

  ibag.color = new fltk::Choice(120, winy, 120, incy);
  const airEnum *enm = dynamic_cast<Deft::PolyProbe*>(ibag.contour)->colorQuantityEnum();
  int itu = airEnumUnknown(enm);
  int itl = airEnumLast(enm);
  for (int qi=itu+1; qi<=itl; qi++) {
    ibag.color->add(airEnumStr(enm, qi), &ibag);
  }
  ibag.color->callback((fltk::Callback*)(color_cb), &ibag);
  const char *def = airEnumStr(enm, cprobe->colorQuantity());
  ibag.color->value(((fltk::Group*)ibag.color)
                   ->find(ibag.color->find(def)));

  winy += incy;
  ibag.slider = new Deft::Slider(0, winy, iwin->w(), incy=40,
                                 "Tumor Isovalue");
  ibag.slider->align(fltk::ALIGN_LEFT);
  ibag.slider->range(AIR_MAX(ibag.contour->minimum(),
                             -2*ibag.contour->maximum()),
                     ibag.contour->maximum());
  ibag.slider->fastUpdate(1);
  ibag.slider->callback(isovalue_cb, &ibag);
  ibag.slider->value(0);

  iwin->end();
  iwin->show(argc,argv);
#endif

#if PST
  pctx->verbose = 0;
  E = 0;
  for (vsi=0; vsi<vspecNum; vsi++) {
    fprintf(stderr, "%s: vspec[%u]: (%u) %s \"%s\"\n", me, 
            vsi, AIR_CAST(unsigned int, nrrdElementNumber(vspec[vsi]->nin)),
            vspec[vsi]->kind->name, vspec[vsi]->vname);
    if (!E) E |= pullVolumeSingleAdd(pctx, vspec[vsi]->vname,
                                     vspec[vsi]->nin, vspec[vsi]->kind,
                                     k00, k11, k22);
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble A:\n%s", me, err);
    airMopError(mop); return 1;
  }
  /* <info>:<vname>:<item>[:<zero>:<scale>] */
  /* this functionality copied from pullInfoSpecAdd(), 
     probably should be a stand-alone function */
  for (idi=0; idi<idefNum; idi++) {
    unsigned int vi;
    pullVolume *vol;
    int item;
    pullInfoSpec *ispec;
    fprintf(stderr, "%s: idef[%u]: %s \"%s\" \n", me, idi, 
            airEnumStr(pullInfo, idef[idi]->info),
            idef[idi]->vname);
    for (vi=0; vi<pctx->volNum; vi++) {
      if (!strcmp(pctx->vol[vi]->name, idef[idi]->vname)) {
        break;
      }
    }
    if (vi == pctx->volNum) {
      fprintf(stderr, "%s: no volume has name \"%s\" (idef[%u])\n", me,
              idef[idi]->vname, idi);
      airMopError(mop); return 1;
    }
    vol = pctx->vol[vi];
    if (!(item = airEnumVal(vol->kind->enm, idef[idi]->itemStr))) {
      fprintf(stderr, "%s: can't parse %s as item of %s kind (idef[%u])\n",
              me, idef[idi]->itemStr, vol->kind->enm->name, idi);
      airMopError(mop); return 1;
    }
    if (!E) ispec = pullInfoSpecNew();
    if (!E) E |= pullInfoSpecAdd(pctx, ispec, idef[idi]->info,
                                 idef[idi]->vname, item);
    if (!E) {
      ispec->zero = idef[idi]->zero;
      ispec->scale = idef[idi]->scale;
      ispec->constraint = 0;
    }
  }
  if (pullInfoSeedThresh != constrInfo) {
    if (pctx->ispec[constrInfo]) {
      pctx->ispec[constrInfo]->constraint = AIR_TRUE;
    } else {
      fprintf(stderr, "%s: can't use %s for constraint, info not set", me,
              airEnumStr(pullInfo, constrInfo));
      airMopError(mop); return 1;
    }
  }
  if (!E) E |= pullEnergySpecSet(pctx->energySpec, ensp->energy, ensp->parm);

  int grads; char *gradsStr;
  nrrdGetenvBool(&grads, &gradsStr, "GRADS");
  if (grads) {
    if (!E) E |= pullStart(pctx);
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(PULL), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble 2:\n%s", me, err);
    airMopError(mop); return 1;
  }

  /* initialize bag and its UI */
  pbag.viewer = viewer;
  pbag.pctx = pctx;
  pbag.nPosOut = nrrdNew();
  pbag.nTenOut = nrrdNew();
  pbag.npos = nrrdNew();
  pbag.nten = nrrdNew();
  pbag.ntmp = nrrdNew();
  
  winy = 10;
  fltk::Window *pwin = new fltk::Window(400, 130, "pull UI");
  pwin->begin();
  pbag.stepButton = new fltk::Button(10, winy, 50, incy=20, "step");
  pbag.stepButton->callback((fltk::Callback*)step_cb, &pbag);

  pbag.tractvis = new fltk::CheckButton(70, winy, 100, incy=20, "show tracts");
  pbag.tractvis->value(false);
  pbag.tractvis->callback(tractvis_cb, &pbag);

  winy += incy;
  pbag.slider = new Deft::Slider(0, winy, pwin->w(), incy=40,
                                 "Seed glyph size");
  pbag.slider->align(fltk::ALIGN_LEFT);
  pbag.slider->range(0, 1);
  pbag.slider->fastUpdate(1);
  pbag.slider->value(0.5);
  pbag.glyph = new Deft::TensorGlyph();
  pbag.glyph->dynamic(true);
  pbag.glyph->twoSided(true);
  pbag.glyph->anisoType(tenAniso_FA);
  pbag.glyph->anisoThreshMin(0.0);
  pbag.glyph->anisoThresh(0.0);
  pbag.glyph->glyphType(tenGlyphTypeSphere);
  pbag.glyph->glyphResolution(7);
  pbag.glyph->glyphScale(pbag.slider->value()*pbag.pctx->radiusSpace);
  pbag.glyph->maskThresh(0.0);
  scene->objectAdd(pbag.glyph);
  pbag.slider->callback(pstgsc_cb, &pbag);

  pwin->end();
  pwin->show();

  pbag.hsline = new Deft::HyperStreamline(vol);
  pbag.hsline->parmCopy(hsline);
  pbag.hsline->visible(pbag.tractvis->value());
  scene->objectAdd(pbag.hsline);
  hslineUI->add(pbag.hsline);
#endif

  // --------------------------------------------------

  fltk::flush();
  // glyph->update();
  fltk::redraw();

#if PST
  /* this is to get "output" prior to any iterations,
     after seeding or initialization */
  outputGet(&pbag);
  outputShow(&pbag);
#endif

  if (!camkeep) {
    viewer->cameraReset();
  }

  fprintf(stderr, "%s: glGetString(GL_VERSION) = %s\n",
          me, glGetString(GL_VERSION));
  
  running = true;

  // this returns when the user quits
  if (!sdaq) {
    ret = fltk::run();
  } else {
    fltk::wait(1);
    fltk::redraw();
    viewer->screenDump();
    ret = 0;
  }

  // this doesn't finish when you close all windows!
  // for (;;) fltk::wait();

  airMopOkay(mop);
  return ret;
}
