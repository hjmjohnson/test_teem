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

#include "PolyData.h"

namespace Deft {

void
PolyData::_init() {

  _nrgba = nrrdNew();
}

PolyData::PolyData(const limnPolyData *lpld) {

  _lpld = lpld;
  _lpldOwn = NULL;
  _drawTime = 0;
  _init();
}

PolyData::PolyData(limnPolyData *lpld, bool own) {

  if (own) {
    _lpld = NULL;
    _lpldOwn = lpld;
  } else {
    _lpld = lpld;
    _lpldOwn = NULL;
  }
  _init();
}

void
PolyData::_valuesClear() {

  _valid.clear();
  for (unsigned int vi=0; vi<_values.size(); vi++) {
    delete _values[vi];
  }
  _values.clear();
}

PolyData::~PolyData() {

  if (_lpldOwn) {
    limnPolyDataNix(_lpldOwn);
  }
  nrrdNuke(_nrgba);
  _valuesClear();
}

std::vector<std::vector<int> >
PolyData::query() const {
  std::vector<std::vector<int> > ret;

  ret.resize(_values.size());
  for (unsigned int qi=0; qi<ret.size(); qi++) {
    ret[qi] = _values[qi]->item();
  }
  return ret;
}

std::vector<const Values *>
PolyData::values() const {
  std::vector<const Values *> ret;

  ret.resize(_values.size());
  for (unsigned int qi=0; qi<ret.size(); qi++) {
    ret[qi] = _values[qi];
  }
  return ret;
}

const limnPolyData *
PolyData::polyData() const {

  return _lpldOwn ? _lpldOwn : _lpld;
}


void
PolyData::probe(const Gage *gage) {
  // char me[]="PolyData::probe";
  double *answerAll[128]; // TERRIBLE
  unsigned int length[128]; // TERRIBLE
  /*
  fprintf(stderr, "!%s: gage->query().size() = %u\n", me, 
          AIR_CAST(unsigned int, gage->query().size()));
  */
  if (0 == gage->query().size()) {
    return;
  }

  // HEY: we're using the length of _valid to 
  // indicate the allocated size of all the values
  if (query() != gage->query()
      || lpld()->vertNum != _valid.size()) {
    // fprintf(stderr, "!%s: deleting new values\n", me);
    _valuesClear();    
  }

  // allocate new values if needed
  if (!_values.size()) {
    // fprintf(stderr, "!%s: allocating new values\n", me);
    _valid.resize(lpld()->vertNum);
    _values.resize(gage->query().size());
    /*
    fprintf(stderr, "!%s: &_values=%p, _values.size()=%u\n", me,
            &_values, AIR_CAST(unsigned int, _values.size()));
    */
    for (unsigned int qi=0; qi<_values.size(); qi++) {
      _values[qi] = new Values(lpld()->vertNum, gage->volume()->kind(),
                               gage->query()[qi]);
      /*
      fprintf(stderr, "!%s: _values[%u]->nrrd()->data = %p\n", me, qi,
              _values[qi]->nrrd()->data);
      */
    }
  }
  
  for (unsigned int qi=0; qi<_values.size(); qi++) {
    answerAll[qi] = _values[qi]->data();
    // fprintf(stderr, "!%s: answerAll[%u] = %p\n", me, qi, answerAll[qi]);
    length[qi] = _values[qi]->length();
    // fprintf(stderr, "!%s: length[%u] = %u\n", me, qi, length[qi]);
  }
      
  unsigned int N = lpld()->vertNum;
  const limnVrt *vert = lpld()->vert;
  for (unsigned int I=0; I<N; I++) {
    gage_t xx, yy, zz;
    xx = static_cast<gage_t>(vert[I].xyzw[0] / vert[I].xyzw[3]);
    yy = static_cast<gage_t>(vert[I].xyzw[1] / vert[I].xyzw[3]);
    zz = static_cast<gage_t>(vert[I].xyzw[2] / vert[I].xyzw[3]);
    _valid[I] = !gage->probe(answerAll, xx, yy, zz);
    for (unsigned int qi=0; qi<_values.size(); qi++) {
      answerAll[qi] += length[qi];
    }
  }

  /** for debugging probing results
  for (unsigned int qi=0; qi<_values.size(); qi++) {
    if (_values[qi]->length()) {
      _values[qi]->save();
    }
  }
  */
}

void
PolyData::color(unsigned int valuesIdx, const Cmap *cmap) {
  char me[]="PolyData::color";

  if (!_lpldOwn) {
    fprintf(stderr, "%s: can't modify data that is not owned\n", me);
    return;
  }
  /*
  fprintf(stderr, "!%s: _values.size() = %u\n", me,
          (unsigned int)(_values.size()));
  fprintf(stderr, "!%s: _nrgba->data = %p, _values[%u] = %p\n", me,
          _nrgba->data, valuesIdx, _values[valuesIdx]);
  */
  if (!_values[valuesIdx]->length()) {
    return;
  }

  cmap->map(_nrgba, _values[valuesIdx]);
  unsigned int N = _lpldOwn->vertNum;
  unsigned char *rgba = AIR_CAST(unsigned char*, _nrgba->data);
  limnVrt *vert = _lpldOwn->vert;
  for (unsigned int I=0; I<N; I++) {
    rgba[3] *= _valid[I];   // needs to persist in _nrgba!!
    ELL_4V_COPY(vert[I].rgba, rgba);
    rgba += 4;
  }
  changed();  // to invalidate display lists
}

void
PolyData::alphaMask(unsigned int valuesIdx, double thresh) {
  char me[]="PolyData::alphaMask";

  if (!_lpldOwn) {
    fprintf(stderr, "%s(%p): can't modify data that is not owned\n", me, this);
    return;
  }
  if (!_values[valuesIdx]->length()) {
    return;
  }
  unsigned int N = _lpldOwn->vertNum;
  unsigned char *rgba = AIR_CAST(unsigned char*, _nrgba->data);
  limnVrt *vert = _lpldOwn->vert;
  double *mdata = _values[valuesIdx]->data();
  unsigned int factorNum = _values[valuesIdx]->length();
  if (mdata) {
    switch (factorNum) {
    case 1:
      for (unsigned int I=0; I<N; I++) {
        vert[I].rgba[3] = mdata[I] >= thresh ? rgba[3] : 0;
        rgba += 4;
      }
      break;
    case 2:
      for (unsigned int I=0; I<N; I++) {
        vert[I].rgba[3] = (mdata[0 + 2*I]*mdata[1 + 2*I] >= thresh 
                           ? rgba[3] 
                           : 0);
        rgba += 4;
      }
      break;
    default:
      for (unsigned int I=0; I<N; I++) {
        double mm = 1.0;
        for (unsigned int fi=0; fi<factorNum; fi++) {
          mm *= mdata[fi + factorNum*I];
        }
        vert[I].rgba[3] = mm >= thresh ? rgba[3] : 0;
        rgba += 4;
      }
      break;
    }
    changed();  // to invalidate display lists
  }
}

void
PolyData::RGBLut(unsigned char lut[256]) {
  char me[]="PolyData::RGBLut";

  if (!_lpldOwn) {
    fprintf(stderr, "%s(%p): can't modify data that is not owned\n", me, this);
    return;
  }
  unsigned char *rgba = AIR_CAST(unsigned char*, _nrgba->data);
  if (!rgba) {
    return;
  }

  size_t N = nrrdElementNumber(_nrgba)/4;
  limnVrt *vert = _lpldOwn->vert;
  for (size_t I=0; I<N; I++) {
    vert[I].rgba[0] = lut[rgba[0 + 4*I]];
    vert[I].rgba[1] = lut[rgba[1 + 4*I]];
    vert[I].rgba[2] = lut[rgba[2 + 4*I]];
  }
  changed();
}

void
PolyData::RGBSet(unsigned char R, unsigned char G, unsigned char B) {
  char me[]="PolyData::RGBSet";

  if (!_lpldOwn) {
    fprintf(stderr, "%s(%p): can't modify data that is not owned\n", me, this);
    return;
  }

  size_t N = _lpldOwn->vertNum;
  limnVrt *vert = _lpldOwn->vert;
  for (size_t I=0; I<N; I++) {
    vert[I].rgba[0] = R;
    vert[I].rgba[1] = G;
    vert[I].rgba[2] = B;
  }
  changed();
}

unsigned int
PolyData::verticesGet(Nrrd *npos) {
  char me[]="PolyData::verticesGet", *err;
  
  const limnPolyData *lpld = this->polyData();
  if (nrrdMaybeAlloc_va(npos, nrrdTypeFloat, 2,
                        AIR_CAST(size_t, 3),
                        AIR_CAST(size_t, lpld->vertNum))) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: couldn't allocate output:\n%s", me, err);
    free(err); exit(1);
  }
  float *pos = static_cast<float *>(npos->data);
  for (unsigned int ii=0; ii<lpld->vertNum; ii++) {
    ELL_34V_HOMOG(pos + 3*ii, lpld->vert[ii].xyzw);
  }
  return lpld->vertNum;
}

void
PolyData::drawImmediate() {
  // char me[]="PolyData::drawImmediate";
  limnVrt *vrt;
  const limnPolyData *lpld = this->lpld();
  int glWhat;
  const GLenum glpt[LIMN_PRIMITIVE_MAX+1] = {
    GL_POINTS,         /* 0: limnPrimitiveUnknown */
    GL_TRIANGLES,      /* 1: limnPrimitiveTriangles */
    GL_TRIANGLE_STRIP, /* 2: limnPrimitiveTriangleStrip */
    GL_TRIANGLE_FAN,   /* 3: limnPrimitiveTriangleFan */
    GL_QUADS,          /* 4: limnPrimitiveQuads */
    GL_LINE_STRIP      /* 5: limnPrimitiveLineStrip */
  };
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  if (!( _visible && lpld )) {
    /* there's nothing to render here */
    return;
  }

  /* {
    unsigned int I = 5370;
    fprintf(stderr, "!%s: vert[%u].rgba = (%u,%u,%u,%u)\n",
            me, I, lpld->vert[I].rgba[0], lpld->vert[I].rgba[1],
            lpld->vert[I].rgba[2], lpld->vert[I].rgba[3]);
    I == 5350;
    fprintf(stderr, "!%s: vert[%u].rgba = (%u,%u,%u,%u)\n",
            me, I, lpld->vert[I].rgba[0], lpld->vert[I].rgba[1],
            lpld->vert[I].rgba[2], lpld->vert[I].rgba[3]);
            } */

  /*
  fprintf(stderr, "!%s: HELLO _colorUse = %s\n", me,
          _colorUse ? "true" : "false");
  fprintf(stderr, "!%s: _compiling = %s\n", me, _compiling ? "true" : "false");
  */
  vrt = lpld->vert;
  if (_transformUse) {
    float tmpMat[16];
    glPushMatrix();
    ELL_4M_TRANSPOSE(tmpMat, _transform);
    glMultMatrixf(tmpMat);
  }
  if (_normalizeUse) {
    glEnable(GL_NORMALIZE);
  }
  if (_colorUse) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  } else {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
    // glColor4fv(white);
  }
  if (_lightingUse) {
    glEnable(GL_LIGHTING);
  }
  if (!_compiling) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    if (_colorUse) {
      glEnableClientState(GL_COLOR_ARRAY);
    }
    glVertexPointer(4, GL_FLOAT, sizeof(limnVrt), vrt[0].xyzw);
    glNormalPointer(GL_FLOAT, sizeof(limnVrt), vrt[0].norm);
    if (_colorUse) {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(limnVrt), vrt[0].rgba);
    }
  }
  unsigned int vertCnt, vertIdx = 0;
  if (_wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  if (_twoSided) {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  } else {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  }
  /*
  fprintf(stderr, "!%s: _colorUse = %s\n", me, _colorUse ? "true" : "false");
  fprintf(stderr, "!%s: _compiling = %s\n", me, _compiling ? "true" : "false");
  */
  if (!_compiling) {
    for (unsigned int primIdx=0; primIdx<lpld->primNum; primIdx++) {
      vertCnt = lpld->vcnt[primIdx];
      glWhat = glpt[lpld->type[primIdx]];
      glDrawElements(glWhat, vertCnt, GL_UNSIGNED_INT, lpld->indx + vertIdx);
      vertIdx += vertCnt;
    }
  } else {
    for (unsigned int primIdx=0; primIdx<lpld->primNum; primIdx++) {
      vertCnt = lpld->vcnt[primIdx];
      glWhat = glpt[lpld->type[primIdx]];
      glBegin(glWhat);
      for (unsigned int vii=0; vii<vertCnt; vii++) {
        glNormal3fv(vrt[(lpld->indx + vertIdx)[vii]].norm);
        if (_colorUse) {
          glColor4ubv(vrt[(lpld->indx + vertIdx)[vii]].rgba);
        }
        glVertex4fv(vrt[(lpld->indx + vertIdx)[vii]].xyzw);
      }
      glEnd();
      vertIdx += vertCnt;
    }
  }
  if (!_compiling) {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    if (_colorUse) {
      glDisableClientState(GL_COLOR_ARRAY);
    }
  }
  if (_lightingUse) {
    glDisable(GL_LIGHTING);
  }
  if (_colorUse) {
    glDisable(GL_COLOR_MATERIAL);
  }
  if (_normalizeUse) {
    glDisable(GL_NORMALIZE);
  }
  if (_transformUse) {
    glPopMatrix();
  }
  return;
}

void
PolyData::boundsGet(float min[3], float max[3]) const {
  // char me[]="PolyData::boundsGet";
  const limnPolyData *lpld = this->lpld();
  unsigned int vi, vertNum;
  float vertB[4], vertC[4];
  limnVrt *vrt;

  if (lpld) {
    vrt = lpld->vert;
    vertNum = lpld->vertNum;
    for (vi=0; vi<vertNum; vi++) {
      /*
      fprintf(stderr, "!%s: xyzw[%u] = (%g,%g,%g,%g)", me, vi,
              vrt[vi].xyzw[0], vrt[vi].xyzw[1],
              vrt[vi].xyzw[2], vrt[vi].xyzw[3]);
      */
      ELL_4MV_MUL(vertB, _transform, vrt[vi].xyzw);
      /*
      fprintf(stderr, " --> (%g,%g,%g,%g)", 
              vertB[0], vertB[1], vertB[2], vertB[3]);
      */
      ELL_4V_HOMOG(vertC, vertB);
      // fprintf(stderr, " --> (%g,%g,%g)\n", vertC[0], vertC[1], vertC[2]);
      if (!vi) {
        ELL_3V_COPY(min, vertC);
        ELL_3V_COPY(max, vertC);
      } else {
        ELL_3V_MIN(min, min, vertC);
        ELL_3V_MAX(max, max, vertC);
      }
    }
  } else {
    ELL_3V_SET(min, 0, 0, 0);
    ELL_3V_SET(max, 0, 0, 0);
  }
  return;
}

} /* namespace Deft */

