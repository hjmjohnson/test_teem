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

#include "TriPlane.h"

namespace Deft {

TriPlane::TriPlane(const Volume *vol) : Group(9) {
  char me[]="TriPlane::TriPlane", *err;

  fprintf(stderr, "%s(%u): kind = %p\n", me, __LINE__, vol->kind());
  _shape = gageShapeNew();
  _vol = vol;
  _bglyph = NULL;
  if (gageShapeSet(_shape, vol->nrrd(), vol->kind()->baseDim)) {
    fprintf(stderr, "%s: trouble with volume:\n%s", me,
            err = biffGetDone(GAGE)); free(err);
    exit(1);
  }

  float tmpW[4], umpW[4], tmpI[4], umpI[4];
  ELL_4V_SET(tmpI, 0.0, 0.0, 0.0, 1.0);
  ELL_4MV_MUL_TT(tmpW, float, _shape->ItoW, tmpI);
  ELL_4V_HOMOG(_origW, tmpW);
  fprintf(stderr, "%s: tmpW=(%g,%g,%g,%g) --> _origW=(%g,%g,%g)\n", me,
          tmpW[0], tmpW[1], tmpW[2], tmpW[3],
          _origW[0], _origW[1], _origW[2]);

  ELL_4V_SET(umpI, 1, 0, 0, 1);
  ELL_4MV_MUL_TT(umpW, float, _shape->ItoW, umpI);
  ELL_4V_HOMOG(umpW, umpW);
  ELL_3V_SUB(_interW[0], umpW, _origW);
  ELL_3V_SCALE(_edgeW[0], _shape->size[0]-1, _interW[0]);

  ELL_4V_SET(umpI, 0, 1, 0, 1);
  ELL_4MV_MUL_TT(umpW, float, _shape->ItoW, umpI);
  ELL_4V_HOMOG(umpW, umpW);
  ELL_3V_SUB(_interW[1], umpW, _origW);
  ELL_3V_SCALE(_edgeW[1], _shape->size[1]-1, _interW[1]);

  ELL_4V_SET(umpI, 0, 0, 1, 1);
  ELL_4MV_MUL_TT(umpW, float, _shape->ItoW, umpI);
  ELL_4V_HOMOG(umpW, umpW);
  ELL_3V_SUB(_interW[2], umpW, _origW);
  ELL_3V_SCALE(_edgeW[2], _shape->size[2]-1, _interW[2]);

  _sampling[0] = _seedSampling[0] = 0.0;
  _sampling[1] = _seedSampling[1] = 0.0;
  _sampling[2] = _seedSampling[2] = 0.0;
  _size[0] = _seedSize[0] = _shape->size[0];
  _size[1] = _seedSize[1] = _shape->size[1];
  _size[2] = _seedSize[2] = _shape->size[2];
  _glyphsDo[0] = false;
  _glyphsDo[1] = false;
  _glyphsDo[2] = false;
  _tractsDo[0] = false;
  _tractsDo[1] = false;
  _tractsDo[2] = false;

  plane[0] = new Plane(_size[1], _size[2]);
  seedPlane[0] = new Plane(_seedSize[1], _seedSize[2]);
  plane[1] = new Plane(_size[0], _size[2]);
  seedPlane[1] = new Plane(_seedSize[0], _seedSize[2]);
  plane[2] = new Plane(_size[0], _size[1]);
  seedPlane[2] = new Plane(_seedSize[0], _seedSize[1]);
  /*
  fprintf(stderr, "!%s: plane[0,1,2] = %p %p %p\n", me, 
          plane[0], plane[1], plane[2]);
  fprintf(stderr, "!%s: seedPlane[0,1,2] = %p %p %p\n", me, 
          seedPlane[0], seedPlane[1], seedPlane[2]);
  */
  plane[0]->color(true);
  plane[0]->alphaMask(true);
  seedPlane[0]->color(false);
  seedPlane[0]->alphaMask(false);
  plane[0]->volume(vol);
  seedPlane[0]->volume(vol);
  plane[0]->edgeUSet(_edgeW[1]);
  seedPlane[0]->edgeUSet(_edgeW[1]);
  plane[0]->edgeVSet(_edgeW[2]);
  seedPlane[0]->edgeVSet(_edgeW[2]);
  position(0, static_cast<float>(_size[0]/2));

  plane[1]->color(true);
  plane[1]->alphaMask(true);
  seedPlane[1]->color(false);
  seedPlane[1]->alphaMask(false);
  plane[1]->volume(vol);
  seedPlane[1]->volume(vol);
  plane[1]->edgeUSet(_edgeW[0]);
  seedPlane[1]->edgeUSet(_edgeW[0]);
  plane[1]->edgeVSet(_edgeW[2]);
  seedPlane[1]->edgeVSet(_edgeW[2]);
  position(1, static_cast<float>(_size[1]/2));

  plane[2]->color(true);
  plane[2]->alphaMask(true);
  seedPlane[2]->color(false);
  seedPlane[2]->alphaMask(false);
  plane[2]->volume(vol);
  seedPlane[2]->volume(vol);
  plane[2]->edgeUSet(_edgeW[0]);
  seedPlane[2]->edgeUSet(_edgeW[0]);
  plane[2]->edgeVSet(_edgeW[1]);
  seedPlane[2]->edgeVSet(_edgeW[1]);
  position(2, static_cast<float>(_size[2]/2));

  seedPlane[0]->visible(false);
  seedPlane[1]->visible(false);
  seedPlane[2]->visible(false);
  plane[0]->lightingUse(false);
  plane[1]->lightingUse(false);
  plane[2]->lightingUse(false);

  seedPlane[0]->visible(false);
  seedPlane[1]->visible(false);
  seedPlane[2]->visible(false);
  seedPlane[0]->wireframe(true);
  seedPlane[1]->wireframe(true);
  seedPlane[2]->wireframe(true);
  
  plane[0]->brightness(1.2);
  plane[1]->brightness(1.2);
  plane[2]->brightness(1.2);
  
  if (tenGageKind == _vol->kind()) {
    TensorGlyph *tgl[3];
    glyph[0] = tgl[0] = new TensorGlyph();
    glyph[1] = tgl[1] = new TensorGlyph();
    glyph[2] = tgl[2] = new TensorGlyph();
    tgl[0]->rgbParmSet(tenAniso_Cl2, 0, 0.6, 1.0, 2.1, 1.0);
    tgl[1]->rgbParmSet(tenAniso_Cl2, 0, 0.6, 1.0, 2.1, 1.0);
    tgl[2]->rgbParmSet(tenAniso_Cl2, 0, 0.6, 1.0, 2.1, 1.0);
    tgl[0]->dynamic(true);
    tgl[1]->dynamic(true);
    tgl[2]->dynamic(true);
    seedPlane[0]->noColorQuery(tenGageTensor);
    seedPlane[1]->noColorQuery(tenGageTensor);
    seedPlane[2]->noColorQuery(tenGageTensor);
  } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, vol->kind()->name)) {
    glyph[0] = new StarGlyph();
    glyph[1] = new StarGlyph();
    glyph[2] = new StarGlyph();
    glyph[0]->twoSided(true);
    glyph[1]->twoSided(true);
    glyph[2]->twoSided(true);
//     glyph[0]->wireframe(true);
//     glyph[1]->wireframe(true);
//     glyph[2]->wireframe(true);
//     seedPlane[0]->noColorQuery(tenDwiGageJustDWI, tenDwiGageFA);
//     seedPlane[1]->noColorQuery(tenDwiGageJustDWI, tenDwiGageFA);
//     seedPlane[2]->noColorQuery(tenDwiGageJustDWI, tenDwiGageFA);
    seedPlane[0]->noColorQuery(tenDwiGageADC, tenDwiGageFA,
                               tenDwiGageTensorAllDWIError);
    seedPlane[1]->noColorQuery(tenDwiGageADC, tenDwiGageFA,
                               tenDwiGageTensorAllDWIError);
    seedPlane[2]->noColorQuery(tenDwiGageADC, tenDwiGageFA,
                               tenDwiGageTensorAllDWIError);
  }
  glyph[0]->visible(_glyphsDo[0]);
  glyph[1]->visible(_glyphsDo[1]);
  glyph[2]->visible(_glyphsDo[2]);

  _seedValFloat[0] = nrrdNew();
  _seedValFloat[1] = nrrdNew();
  _seedValFloat[2] = nrrdNew();
  _seedPosFloat[0] = nrrdNew();
  _seedPosFloat[1] = nrrdNew();
  _seedPosFloat[2] = nrrdNew();

  hsline[0] = new HyperStreamline(vol);
  hsline[1] = new HyperStreamline(vol);
  hsline[2] = new HyperStreamline(vol);
  hsline[0]->visible(_tractsDo[0]);
  hsline[1]->visible(_tractsDo[1]);
  hsline[2]->visible(_tractsDo[2]);
  hsline[0]->visible(false);
  hsline[1]->visible(false);
  hsline[2]->visible(false);

  // this is what determines what will be drawn 
  object[0] = plane[0];
  object[1] = plane[1];
  object[2] = plane[2];
  object[3] = glyph[0];
  object[4] = glyph[1];
  object[5] = glyph[2];
  object[6] = hsline[0];
  object[7] = hsline[1];
  object[8] = hsline[2];
}

TriPlane::~TriPlane() {
  
  // HEY: there is stuff to go here!

  delete hsline[0];
  delete hsline[1];
  delete hsline[2];
  nrrdNuke(_seedValFloat[0]);
  nrrdNuke(_seedValFloat[1]);
  nrrdNuke(_seedValFloat[2]);
  nrrdNuke(_seedPosFloat[0]);
  nrrdNuke(_seedPosFloat[1]);
  nrrdNuke(_seedPosFloat[2]);
}

void
TriPlane::baseGlyph(const limnPolyData *bglyph) {

  _bglyph = bglyph;
  if (!strcmp(TEN_DWI_GAGE_KIND_NAME, _vol->kind()->name)) {
    StarGlyph *sgl;
    for (unsigned int pi=0; pi<3; pi++) {
      sgl = dynamic_cast<StarGlyph *>(glyph[pi]);
      sgl->baseGlyph(_bglyph);
    }
  }
}

void
TriPlane::sampling(unsigned int axisIdx, double smpl) {

  axisIdx = AIR_MIN(axisIdx, 2);
  _size[axisIdx] = AIR_CAST(unsigned int,
                            pow(2.0, smpl)*_shape->size[axisIdx]);
  if (0 != axisIdx) {
    plane[0]->resolution(_size[1], _size[2]);
    plane[0]->update();
  }
  if (1 != axisIdx) {
    plane[1]->resolution(_size[0], _size[2]);
    plane[1]->update();
  }
  if (2 != axisIdx) {
    plane[2]->resolution(_size[0], _size[1]);
    plane[2]->update();
  }
  _sampling[axisIdx] = smpl;
}

void
TriPlane::seedSampling(unsigned int axisIdx, double smpl) {
  // char me[]="TriPlane::seedSampling";

  axisIdx = AIR_MIN(axisIdx, 2);
  _seedSize[axisIdx] = AIR_CAST(unsigned int,
                                pow(2.0, smpl)*_shape->size[axisIdx]);
  if (0 != axisIdx) {
    seedPlane[0]->resolution(_seedSize[1], _seedSize[2]);
    seedPlane[0]->update();
  }
  if (1 != axisIdx) {
    seedPlane[1]->resolution(_seedSize[0], _seedSize[2]);
    seedPlane[1]->update();
  }
  if (2 != axisIdx) {
    seedPlane[2]->resolution(_seedSize[0], _seedSize[1]);
    seedPlane[2]->update();
  }
  _seedSampling[axisIdx] = smpl;
}

const gageShape *TriPlane::shape() const { return _shape; }

double TriPlane::sampling(unsigned int ai) const { return _sampling[ai]; }
double TriPlane::seedSampling(unsigned int ai) const { 
  return _seedSampling[ai]; 
}

void
TriPlane::position(unsigned int pIdx, float pos) {
  // char me[]="TriPlane::position";
  float vec[3];

  /*
  fprintf(stderr, "!%s(%u, %g): visible %s -------------------\n",
          me, pIdx, pos, plane[pIdx]->visible() ? "true" : "false");
  */
  pIdx = AIR_MIN(pIdx, 2);
  _posI[pIdx] = pos;
  ELL_3V_SCALE_ADD2(vec, 1.0f, _origW, _posI[pIdx], _interW[pIdx]);
  plane[pIdx]->originSet(vec);
  seedPlane[pIdx]->originSet(vec);
  if (plane[pIdx]->visible()) {
    plane[pIdx]->update();
  } else {
    plane[pIdx]->updateGeometry();
  }
  if (_glyphsDo[pIdx] || _tractsDo[pIdx]) {
    seedPlane[pIdx]->update();
    this->glyphsUpdate(pIdx);
    this->tractsUpdate(pIdx);
  } else {
    seedPlane[pIdx]->updateGeometry();
  }
}

void
TriPlane::glyphsUpdate(unsigned int pIdx) {
  char me[]="TriPlane::glyphsUpdate";

  if (_glyphsDo[pIdx]) {
    nrrdConvert(_seedValFloat[pIdx],
                seedPlane[pIdx]->values()[0]->nrrd(), 
                nrrdTypeFloat);
    if (tenGageKind == _vol->kind()) {
      TensorGlyph *tgl = static_cast<TensorGlyph*>(glyph[pIdx]);
      tgl->dataSet(_seedValFloat[pIdx]->axis[1].size,
                   static_cast<float*>(_seedValFloat[pIdx]->data), 7,
                   seedPlane[pIdx]->polyData()->xyzw, 4, 
                   NULL);
      tgl->update();
    } else {
      StarGlyph *sgl = static_cast<StarGlyph*>(glyph[pIdx]);
      const Nrrd *n0 = seedPlane[pIdx]->values()[0]->nrrd();
      unsigned int stride = n0->axis[0].size;
      unsigned int NUM = (n0->axis[0].size - 1)/2;
      double *n0d = AIR_CAST(double *, n0->data);
      /*
      fprintf(stderr, "!%s: %u=%u x %u; n0 = %p (%u x %u)\n", me, 
              seedPlane[pIdx]->values().size(),
              seedPlane[pIdx]->resolutionU(),
              seedPlane[pIdx]->resolutionV(), n0,
              n0->axis[0].size, n0->axis[1].size);
      */
      sgl->dataSet(n0->axis[1].size,
                   n0d, NUM, stride,
                   seedPlane[pIdx]->polyData()->xyzw, 4,
                   n0d + NUM, stride,
                   n0d + NUM + 1, stride);

//   void dataSet(unsigned int num,
//                const double *radData,
//                unsigned int radLen, unsigned int radStride,
//                const float *posData, unsigned int posStride,
//                const double *anisoData);

//     case colorTenQuantityModeFA:
//       _queryColor.resize(2);
//       _queryColor[0] = tenGageMode;
//       _queryColor[1] = tenGageCa2;

      sgl->update();
    }
  }
}

void
TriPlane::tractsUpdate(unsigned int pIdx) {

  if (_tractsDo[pIdx]) {
    seedPlane[pIdx]->verticesGet(_seedPosFloat[pIdx]);
    hsline[pIdx]->seedsSet(_seedPosFloat[pIdx]);
    hsline[pIdx]->update();
  }
}

void
TriPlane::seedAnisoThresh(double aniso) {
  char me[]="TriPlane::seedAnisoThresh";

  if (tenGageKind == _vol->kind()) {
    static_cast<TensorGlyph*>(this->glyph[0])->anisoThresh(aniso);
    static_cast<TensorGlyph*>(this->glyph[1])->anisoThresh(aniso);
    static_cast<TensorGlyph*>(this->glyph[2])->anisoThresh(aniso);
  } else {
    static_cast<StarGlyph*>(this->glyph[0])->anisoThresh(aniso);
    static_cast<StarGlyph*>(this->glyph[1])->anisoThresh(aniso);
    static_cast<StarGlyph*>(this->glyph[2])->anisoThresh(aniso);
  }
}

double
TriPlane::seedAnisoThresh() const {
  double ret = AIR_NAN;

  if (this->glyph[0]) {
    if (tenGageKind == _vol->kind()) {
      ret = static_cast<TensorGlyph*>(this->glyph[0])->anisoThresh();
    } else {
      ret = static_cast<StarGlyph*>(this->glyph[0])->anisoThresh();
    }
  }
  return ret;
}

float TriPlane::position(unsigned int planeIdx) const { 
  return _posI[AIR_MIN(planeIdx, 2)]; 
}

void
TriPlane::kernel(int which, const NrrdKernelSpec *ksp) {
  // char me[]="TriPlane::kernel";

  plane[0]->kernel(which, ksp);
  plane[1]->kernel(which, ksp);
  plane[2]->kernel(which, ksp);
  seedPlane[0]->kernel(which, ksp);
  seedPlane[1]->kernel(which, ksp);
  seedPlane[2]->kernel(which, ksp);
}

void
TriPlane::colorQuantity(int quantity) {
  char me[]="TriPlane::colorQuantity";

  const airEnum *enm = plane[0]->colorQuantityEnum();
  fprintf(stderr, "!%s: %s = %s (%d)\n", me,
          enm->name, airEnumStr(enm, quantity), quantity);
  plane[0]->colorQuantity(quantity);
  plane[1]->colorQuantity(quantity);
  plane[2]->colorQuantity(quantity);
}

void
TriPlane::alphaMaskQuantity(int quantity) {
  // char me[]="TriPlane::alphaMaskQuantity";

  plane[0]->alphaMaskQuantity(quantity);
  plane[1]->alphaMaskQuantity(quantity);
  plane[2]->alphaMaskQuantity(quantity);
}

void
TriPlane::alphaMaskThreshold(double thresh) {

  plane[0]->alphaMaskThreshold(thresh);
  plane[1]->alphaMaskThreshold(thresh);
  plane[2]->alphaMaskThreshold(thresh);
}

void
TriPlane::glyphsDo(unsigned int axisIdx, bool doit) {

  axisIdx = AIR_MIN(axisIdx, 2);
  bool old = _glyphsDo[axisIdx];
  _glyphsDo[axisIdx] = doit;
  glyph[axisIdx]->visible(_glyphsDo[axisIdx]);
  if (old != doit && doit) {
    // this is a hack to force update of the seedPlane tensor data,
    // since right now TriPlane lacks a full flag[] machinery
    this->position(axisIdx, this->position(axisIdx));
  }
}

void
TriPlane::tractsDo(unsigned int axisIdx, bool doit) {

  axisIdx = AIR_MIN(axisIdx, 2);
  bool old = _tractsDo[axisIdx];
  _tractsDo[axisIdx] = doit;
  hsline[axisIdx]->visible(_tractsDo[axisIdx]);
  if (old != doit && doit) {
    // this is a hack to force update (see above) ...
    this->position(axisIdx, this->position(axisIdx));
  }
}

bool
TriPlane::glyphsDo(unsigned int axisIdx) const {

  axisIdx = AIR_MIN(axisIdx, 2);
  return _glyphsDo[axisIdx];
}

bool
TriPlane::tractsDo(unsigned int axisIdx) const {

  axisIdx = AIR_MIN(axisIdx, 2);
  return _tractsDo[axisIdx];
}

void
TriPlane::brightness(float br) {

  plane[0]->brightness(br);
  plane[1]->brightness(br);
  plane[2]->brightness(br);
}

void
TriPlane::update() {
  // char me[]="TriPlane::update";

  plane[0]->update();
  plane[1]->update();
  plane[2]->update();
  seedPlane[0]->update();
  seedPlane[1]->update();
  seedPlane[2]->update();
  this->glyphsUpdate(0);
  this->glyphsUpdate(1);
  this->glyphsUpdate(2);
  this->tractsUpdate(0);
  this->tractsUpdate(1);
  this->tractsUpdate(2);
}


} /* namespace Deft */
