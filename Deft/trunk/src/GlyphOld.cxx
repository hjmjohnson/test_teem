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
#include "GlyphOld.h"

namespace Deft {

GlyphOld::GlyphOld() {
  naniso = nrrdNew();
  nten = NULL;
  gparm = tenGlyphParmNew();
  // enstate sensible defaults
  gparm->anisoType = tenAniso_FA;
  gparm->colEvec = 0;
  gparm->colAnisoType = tenAniso_Cl2;
  gparm->confThresh = 0.5;
  gparm->anisoThresh = 1.0;
  gparm->glyphType = tenGlyphTypeSuperquad;
  gparm->facetRes = 6;
}

GlyphOld::~GlyphOld() {
  naniso = nrrdNuke(naniso);
  gparm = tenGlyphParmNix(gparm);
}

void
GlyphOld::volumeSet(const Nrrd *nin) {
  char me[]="Deft::GlyphOld::volumeSet", *err;

  if (tenTensorCheck(nin, nrrdTypeFloat, AIR_TRUE, AIR_TRUE)
      || tenAnisoVolume(naniso, nin, gparm->anisoType, gparm->confThresh)) {
    err = biffGetDone(TEN);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    free(err); return;
  }
  nten = nin;
  return;
}

void
GlyphOld::anisoSet(int aniso) {
  char me[]="Deft::GlyphOld::anisoSet", *err;

  if (nten) {
    if (tenAnisoVolume(naniso, nten, aniso, gparm->confThresh)) {
      err = biffGetDone(TEN);
      fprintf(stderr, "%s: trouble:\n%s", me, err);
      free(err); return;
    }
  }
  gparm->anisoType = aniso;
  return;
}

void
GlyphOld::anisoThreshSet(float athresh) {

  gparm->anisoThresh = athresh;
}

void
GlyphOld::typeSet(int glyphType) {
  
  gparm->glyphType = glyphType;
}

void
GlyphOld::resolutionSet(int res) {
  
  gparm->facetRes = res;
}

void
GlyphOld::scaleSet(float scale) {
  
  gparm->glyphScale = scale;
}

// NOTE: this over-estimates the number of glyphs when
// there are negative eigenvalues, but it shouldn't under-estimate
int
GlyphOld::glyphNum() {
  size_t I, N;
  float *aniso, *tdata;
  int num;

  aniso = (float*)(naniso->data);
  tdata = (float*)(nten->data);
  N = nrrdElementNumber(naniso);
  num = 0;
  for (I=0; I<N; I++) {
    if (!( TEN_T_EXISTS(tdata + 7*I) )) {
      continue;
    }
    if (!( tdata[0 + 7*I] >= gparm->confThresh )) {
      continue;
    }
    if (!( aniso[I] >= gparm->anisoThresh )) {
      continue;
    }
    num += 1;
  }
  return num;
}

// facePerGlyph() and vertPerGlyph() are based on a
// close reading of limn/src/shapes.c
int
GlyphOld::facePerGlyph() {
  int ret=0, thRes;
  
  switch(gparm->glyphType) {
  case tenGlyphTypeBox:
    ret = 6;
    break;
  case tenGlyphTypeSphere:
  case tenGlyphTypeSuperquad:
    thRes = 2*gparm->facetRes;
    ret = 2*thRes + thRes*(gparm->facetRes - 2);
    break;
  case tenGlyphTypeCylinder:
    ret = 2 + gparm->facetRes;
    break;
  }
  return ret;
}

int
GlyphOld::vertPerGlyph() {
  int ret=0, thRes;

  switch(gparm->glyphType) {
  case tenGlyphTypeBox:
    ret = 8;
    break;
  case tenGlyphTypeSphere:
  case tenGlyphTypeSuperquad:
    thRes = 2*gparm->facetRes;
    ret = 2 + thRes*(gparm->facetRes - 1);
    break;
  case tenGlyphTypeCylinder:
    ret = 2*gparm->facetRes;
    break;
  }
  return ret;
}

void
GlyphOld::generate(limnObject *obj) {
  char me[]="Deft::Glyph::generate", *err;
  int gnum;

  limnObjectEmpty(obj);
  gnum = glyphNum();
  limnObjectPreSet(obj, gnum, gnum, vertPerGlyph(), 0, facePerGlyph());
  if (tenGlyphGen(obj, NULL, gparm, nten, NULL, NULL)) {
    err = biffGetDone(TEN);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    free(err); return;
  }
  limnObjectFaceNormals(obj, limnSpaceWorld);
  limnObjectVertexNormals(obj);
  
  return;
}

} /* namespace Deft */
