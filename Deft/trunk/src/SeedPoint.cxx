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

#include "SeedPoint.h"

namespace Deft {

SeedPoint::SeedPoint(const Volume *vol,
                     const limnPolyData *bglyph) : Group(5) {
  char me[]="SeedPoint::SeedPoint", *err;

  _shape = gageShapeNew();
  _vol = vol;
  _bglyph = bglyph;
  if (strcmp(TEN_DWI_GAGE_KIND_NAME, vol->kind()->name)) {
    fprintf(stderr, "%s: sorry, need %s kind (not %s)\n", me,
            TEN_DWI_GAGE_KIND_NAME, vol->kind()->name);
    exit(1);
  }
  if (gageShapeSet(_shape, vol->nrrd(), vol->kind()->baseDim)) {
    fprintf(stderr, "%s: trouble with volume:\n%s", me,
            err = biffGetDone(GAGE)); free(err);
    exit(1);
  }
  if (AIR_EXISTS(vol->nrrd()->measurementFrame[0][0])) {
    _mframe[0] = static_cast<float>(vol->nrrd()->measurementFrame[0][0]);
    _mframe[1] = static_cast<float>(vol->nrrd()->measurementFrame[1][0]);
    _mframe[2] = static_cast<float>(vol->nrrd()->measurementFrame[2][0]);
    _mframe[3] = static_cast<float>(vol->nrrd()->measurementFrame[0][1]);
    _mframe[4] = static_cast<float>(vol->nrrd()->measurementFrame[1][1]);
    _mframe[5] = static_cast<float>(vol->nrrd()->measurementFrame[2][1]);
    _mframe[6] = static_cast<float>(vol->nrrd()->measurementFrame[0][2]);
    _mframe[7] = static_cast<float>(vol->nrrd()->measurementFrame[1][2]);
    _mframe[8] = static_cast<float>(vol->nrrd()->measurementFrame[2][2]);
  } else {
    ELL_3M_IDENTITY_SET(_mframe);
  }

  _scale = 1500;

  limnPolyData *lpld;
  lpld = limnPolyDataNew();
  limnPolyDataSpiralSphere(lpld, 
                           (1 << limnPolyDataInfoRGBA)
                           | (1 << limnPolyDataInfoNorm),
                           20, 10);
  _cursor = new PolyData(lpld, true);
  _cursor->transformUse(true);
  _cursor->wireframe(false);
  _cursor->visible(true);

  _seedpld = limnPolyDataNew();
  limnPolyDataAlloc(_seedpld, 0 /* nothing besides position */,
                    1, 1, 1);  /* will expand later */
  _seedpld->indx[0] = 0;
  _seedpld->type[0] = limnPrimitiveNoop;
  _seedpld->icnt[0] = 1;

  _seedProbe = new PolyProbe();
  _seedProbe->volume(vol);
  _seedProbe->color(false);
  _seedProbe->alphaMask(false);
  _seedProbe->visible(false);
  _seedProbe->noColorQuery(tenDwiGageADC, tenDwiGageFA,
                           tenDwiGageTensorAllDWIError);

  _ten1Probe = new PolyProbe();
  _ten1Probe->volume(vol);
  _ten1Probe->color(false);
  _ten1Probe->alphaMask(false);
  _ten1Probe->visible(false);
  _ten1Probe->noColorQuery(tenDwiGageTensor);

  _ten2Probe = new PolyProbe();
  _ten2Probe->volume(vol);
  _ten2Probe->color(false);
  _ten2Probe->alphaMask(false);
  _ten2Probe->visible(false);
  _ten2Probe->noColorQuery(tenDwiGage2TensorPeled);

  _starGlyph = new StarGlyph();
  _starGlyph->baseGlyph(_bglyph);
  _starGlyph->twoSided(true);
  _starGlyph->visible(true);
  _starGlyph->scale(_scale);
  _starGlyph->anisoThresh(0.0);

  _ten1Glyph = new TensorGlyph();
  _ten1Glyph->visible(false);
  _ten1Glyph->anisoType(tenAniso_FA);
  _ten1Glyph->maskThresh(0.0);
  _ten1Glyph->anisoThreshMin(0.0);
  _ten1Glyph->anisoThresh(0.0);
  _ten1Glyph->glyphType(tenGlyphTypeSuperquad);
  _ten1Glyph->superquadSharpness(3.0);
  _ten1Glyph->glyphResolution(11);
  _ten1Glyph->glyphScale(_scale);
  _ten1Glyph->rgbParmSet(tenAniso_Cl2, 0, 0.7, 1.0, 2.3, 1.0);

  _ten2Glyph = new TensorGlyph();
  _ten2Glyph->visible(false);
  _ten2Glyph->anisoType(tenAniso_FA);
  _ten2Glyph->maskThresh(0.0);
  _ten2Glyph->anisoThreshMin(0.0);
  _ten2Glyph->anisoThresh(0.0);
  _ten2Glyph->glyphType(tenGlyphTypeSuperquad);
  _ten2Glyph->superquadSharpness(3.0);
  _ten2Glyph->glyphResolution(11);
  _ten2Glyph->glyphScale(_scale);
  _ten2Glyph->rgbParmSet(tenAniso_Cl2, 0, 0.7, 1.0, 2.3, 1.0);
  
  this->positionSet(_shape->size[0]/2.0,
                    _shape->size[1]/2.0,
                    _shape->size[2]/2.0);
  this->cursorScale(1);

  hsline = new HyperStreamline(vol);
  hsline->visible(false);
  hsline->verbose(2);

  _npos = nrrdNew(); // will get set dynamically
  _ntmp = nrrdNew();
  _ntmp1 = nrrdNew();
  _ntmp2 = nrrdNew();

  // this is what determines what will be drawn 
  object[0] = _cursor;
  object[1] = _starGlyph;
  object[2] = _ten1Glyph;
  object[3] = _ten2Glyph;
  object[4] = hsline;

  fprintf(stderr, "!%s: ten1Glyph visible = %s %s ***************\n", me, 
          _ten1Glyph->visible() ? "true" : "false",
          this->ten1GlyphDo() ? "true" : "false");

}

SeedPoint::~SeedPoint() {
  
  _npos = nrrdNuke(_npos);
  _ntmp = nrrdNuke(_ntmp);
  _ntmp1 = nrrdNuke(_ntmp1);
  _ntmp2 = nrrdNuke(_ntmp2);
  _seedpld = limnPolyDataNix(_seedpld);
  delete _cursor;
  delete _seedProbe;
  delete _ten1Probe;
  delete _starGlyph;
  delete _ten1Glyph;
  delete _ten2Glyph;
  delete hsline;
}

void
SeedPoint::kernel(int which, const NrrdKernelSpec *ksp) {
  
  _seedProbe->kernel(which, ksp);
  _ten1Probe->kernel(which, ksp);
  _ten2Probe->kernel(which, ksp);
  if (gageKernel00 == which) {
    hsline->kernel(ksp);
  }
}

void
SeedPoint::scale(double scl) {
  
  _ten1Glyph->glyphScale(scl);
  _ten2Glyph->glyphScale(scl);
  _starGlyph->scale(scl);
}

void
SeedPoint::starGlyphDo(bool doit) { 
  char me[]="SeedPoint::starGlyphDo";

  _starGlyph->visible(doit); 
  if (doit) {
    this->update();
  }
}
  
void
SeedPoint::ten1GlyphDo(bool doit) {
  _ten1Glyph->visible(doit);
  if (doit) {
    this->update();
  }
}

void
SeedPoint::ten2GlyphDo(bool doit) {
  _ten2Glyph->visible(doit);
  if (doit) {
    this->update();
  }
}

void
SeedPoint::fiberDo(bool doit) {
  hsline->visible(doit); 
  if (doit) {
    this->update();
  }
}

void
SeedPoint::positionSet(double X, double Y, double Z) {
  char me[]="SeedPoint::positionSet";
  double ipos[4], wpos[4];
  float tmat[16];

  if (nrrdCenterNode == _shape->center) {
    _pos[0] = AIR_CLAMP(0, X, _shape->size[0]-1);
    _pos[1] = AIR_CLAMP(0, Y, _shape->size[1]-1);
    _pos[2] = AIR_CLAMP(0, Z, _shape->size[2]-1);
  } else {
    _pos[0] = AIR_CLAMP(-0.5, X, _shape->size[0]-0.5);
    _pos[1] = AIR_CLAMP(-0.5, Y, _shape->size[1]-0.5);
    _pos[2] = AIR_CLAMP(-0.5, Z, _shape->size[2]-0.5);
  }

  ELL_4V_SET(ipos, _pos[0], _pos[1], _pos[2], 1);
  ELL_4MV_MUL(wpos, _shape->ItoW, ipos);
  ELL_4V_HOMOG(wpos, wpos);

  ELL_4V_COPY(_seedpld->xyzw, wpos);
  _seedProbe->lpldCopy(_seedpld);
  _ten1Probe->lpldCopy(_seedpld);
  _ten2Probe->lpldCopy(_seedpld);
  
  ELL_4V_SET(tmat +  0, _cursScale, 0, 0, wpos[0]);
  ELL_4V_SET(tmat +  4, 0, _cursScale, 0, wpos[1]);
  ELL_4V_SET(tmat +  8, 0, 0, _cursScale, wpos[2]);
  ELL_4V_SET(tmat + 12, 0, 0, 0, 1);
  _cursor->transformSet(tmat);
}

void
SeedPoint::positionGet(double *pos) const {

  if (pos) {
    ELL_3V_COPY(pos, _pos);
  }
}


void
SeedPoint::cursorScale(double scl) {
  char me[]="SeedPoint::cursorScale";
  float tmat[16];

  _cursScale = scl;
  ELL_4V_SET(tmat +  0, _cursScale, 0, 0, _pos[0]);
  ELL_4V_SET(tmat +  4, 0, _cursScale, 0, _pos[1]);
  ELL_4V_SET(tmat +  8, 0, 0, _cursScale, _pos[2]);
  ELL_4V_SET(tmat + 12, 0, 0, 0, 1);
  _cursor->transformSet(tmat);
}

void
SeedPoint::update() {
  char me[]="SeedPoint::update";

  if (_ten1Glyph->visible()) {
    _ten1Probe->update(true);
    nrrdConvert(_ntmp, _ten1Probe->values()[0]->nrrd(), nrrdTypeFloat);
    _ten1Glyph->dataSet(_ntmp->axis[1].size,
                        static_cast<float*>(_ntmp->data), 7,
                        _seedpld->xyzw, 4,
                        _mframe);
    _ten1Glyph->update();
  }
  if (_ten2Glyph->visible()) {
    ptrdiff_t min[2], max[2];

    _ten2Probe->update(true);
    nrrdConvert(_ntmp, _ten2Probe->values()[0]->nrrd(), nrrdTypeFloat);
    _ten2Probe->verticesGet(_ntmp1);
    min[0] = 0;
    min[1] = 0;
    max[0] = 5;
    max[1] = _ntmp1->axis[1].size;
    nrrdPad_nva(_ntmp2, _ntmp1, min, max, nrrdBoundaryWrap, AIR_NAN);
    _ten2Glyph->dataSet(2*_ntmp->axis[1].size,
                        static_cast<float*>(_ntmp->data), 7,
                        static_cast<float*>(_ntmp2->data), 3,
                        _mframe);
    _ten2Glyph->update();
  }
  if (_starGlyph->visible()) {
    _seedProbe->update(true);
    const Nrrd *n0 = _seedProbe->values()[0]->nrrd();
    unsigned int stride = n0->axis[0].size;
    unsigned int NUM = (n0->axis[0].size - 1)/2;
    double *n0d = AIR_CAST(double *, n0->data);
    _starGlyph->dataSet(n0->axis[1].size,
                        n0d, NUM, stride,
                        _seedpld->xyzw, 4,
                        n0d + NUM, stride,
                        n0d + NUM + 1, stride);
    _starGlyph->update();
  }
  if (hsline->visible()) {
    _seedProbe->verticesGet(_ntmp1);
    hsline->seedsSet(_ntmp1);
    hsline->update();
  }
  
}


} /* namespace Deft */
