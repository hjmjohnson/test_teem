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
      || lpld()->xyzwNum != _valid.size()) {
    // fprintf(stderr, "!%s: deleting new values\n", me);
    _valuesClear();    
  }

  // allocate new values if needed
  if (!_values.size()) {
    // fprintf(stderr, "!%s: allocating new values\n", me);
    _valid.resize(lpld()->xyzwNum);
    _values.resize(gage->query().size());
    /*
    fprintf(stderr, "!%s: &_values=%p, _values.size()=%u\n", me,
            &_values, AIR_CAST(unsigned int, _values.size()));
    */
    for (unsigned int qi=0; qi<_values.size(); qi++) {
      _values[qi] = new Values(lpld()->xyzwNum, gage->volume()->kind(),
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
      
  unsigned int N = lpld()->xyzwNum;
  const float *xyzw = lpld()->xyzw;
  for (unsigned int I=0; I<N; I++) {
    double xyz[3];
    ELL_34V_HOMOG(xyz, xyzw + 4*I);
    _valid[I] = !gage->probe(answerAll, xyz[0], xyz[1], xyz[2]);
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
  if (!cmap) {
    fprintf(stderr, "%s: got NULL cmap\n", me);
    return;
  }
  if (!_lpldOwn->rgba || _lpldOwn->rgbaNum != _lpldOwn->xyzwNum) {
    fprintf(stderr, "!%s: have to allocate RGB!!!!!!!!!\n", me);
    if (limnPolyDataAlloc(_lpldOwn,
                          (limnPolyDataInfoBitFlag(_lpldOwn)
                           | (1 << limnPolyDataInfoRGBA)),
                          _lpldOwn->xyzwNum,
                          _lpldOwn->indxNum,
                          _lpldOwn->primNum)) {
      fprintf(stderr, "%s: PROBLEM:\n%s", me, biffGetDone(LIMN));
    }
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
  unsigned int N = _lpldOwn->xyzwNum;
  unsigned char *nrgba = AIR_CAST(unsigned char*, _nrgba->data);
  unsigned char *vrgba = _lpldOwn->rgba;
  if (!(nrgba && vrgba)) {
    fprintf(stderr, "%s: didn't have non-NULL nrgba %p and vrgba %p\n", me,
            nrgba, vrgba);
    return;
  }
  for (unsigned int I=0; I<N; I++) {
    nrgba[3] *= _valid[I];   // needs to persist in _nrgba!!
    ELL_4V_COPY(vrgba, nrgba);
    nrgba += 4;
    vrgba += 4;
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
  unsigned int N = _lpldOwn->xyzwNum;
  unsigned char *nrgba = AIR_CAST(unsigned char*, _nrgba->data);
  unsigned char *vrgba = _lpldOwn->rgba;
  double *mdata = _values[valuesIdx]->data();
  unsigned int factorNum = _values[valuesIdx]->length();
  if (mdata) {
    switch (factorNum) {
    case 1:
      for (unsigned int I=0; I<N; I++) {
        (vrgba + 4*I)[3] = mdata[I] >= thresh ? nrgba[3] : 0;
        nrgba += 4;
      }
      break;
    case 2:
      for (unsigned int I=0; I<N; I++) {
        (vrgba + 4*I)[3] = (mdata[0 + 2*I]*mdata[1 + 2*I] >= thresh 
                            ? nrgba[3] 
                            : 0);
        nrgba += 4;
      }
      break;
    default:
      for (unsigned int I=0; I<N; I++) {
        double mm = 1.0;
        for (unsigned int fi=0; fi<factorNum; fi++) {
          mm *= mdata[fi + factorNum*I];
        }
        (vrgba + 4*I)[3] = mm >= thresh ? nrgba[3] : 0;
        nrgba += 4;
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
  unsigned char *nrgba = AIR_CAST(unsigned char*, _nrgba->data);
  if (!nrgba) {
    return;
  }

  size_t N = nrrdElementNumber(_nrgba)/4;
  unsigned char *vrgba = _lpldOwn->rgba;
  for (size_t I=0; I<N; I++) {
    (vrgba + 4*I)[0] = lut[nrgba[0 + 4*I]];
    (vrgba + 4*I)[1] = lut[nrgba[1 + 4*I]];
    (vrgba + 4*I)[2] = lut[nrgba[2 + 4*I]];
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

  size_t N = _lpldOwn->xyzwNum;
  unsigned char *vrgba = _lpldOwn->rgba;
  if (vrgba) {
    for (size_t I=0; I<N; I++) {
      (vrgba + 4*I)[0] = R;
      (vrgba + 4*I)[1] = G;
      (vrgba + 4*I)[2] = B;
    }
  }
  changed();
}

unsigned int
PolyData::verticesGet(Nrrd *npos) {
  char me[]="PolyData::verticesGet", *err;
  
  const limnPolyData *lpld = this->polyData();
  if (nrrdMaybeAlloc_va(npos, nrrdTypeFloat, 2,
                        AIR_CAST(size_t, 3),
                        AIR_CAST(size_t, lpld->xyzwNum))) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: couldn't allocate output:\n%s", me, err);
    free(err); exit(1);
  }
  float *pos = static_cast<float *>(npos->data);
  for (unsigned int ii=0; ii<lpld->xyzwNum; ii++) {
    ELL_34V_HOMOG(pos + 3*ii, lpld->xyzw + 4*ii);
  }
  return lpld->xyzwNum;
}

void
PolyData::drawImmediate() {
  char me[]="PolyData::drawImmediate";
  const limnPolyData *lpld = this->lpld();
  int glWhat;
  const GLenum glpt[LIMN_PRIMITIVE_MAX+1] = {
    0,                 /* 0: limnPrimitiveUnknown */
    GL_POINTS,         /* 1: limnPrimitiveNoop */
    GL_TRIANGLES,      /* 2: limnPrimitiveTriangles */
    GL_TRIANGLE_STRIP, /* 3: limnPrimitiveTriangleStrip */
    GL_TRIANGLE_FAN,   /* 4: limnPrimitiveTriangleFan */
    GL_QUADS,          /* 5: limnPrimitiveQuads */
    GL_LINE_STRIP      /* 6: limnPrimitiveLineStrip */
  };
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  if (!( _visible && lpld )) {
    /* there's nothing to render here */
    return;
  }

  /*
  {
    unsigned int I = 5370;
    fprintf(stderr, "!%s: vert[%u].rgba = (%u,%u,%u,%u)\n",
            me, I,
            lpld->rgba + 4*I + 0, lpld->rgba + 4*I + 1,
            lpld->rgba + 4*I + 2, lpld->rgba + 4*I + 3);
  }
  */
  /*
  fprintf(stderr, "!%s: HELLO _colorUse = %s\n", me,
          _colorUse ? "true" : "false");
  fprintf(stderr, "!%s: _compiling = %s\n", me, _compiling ? "true" : "false");
  */
  if (_transformUse) {
    float tmpMat[16];
    glPushMatrix();
    ELL_4M_TRANSPOSE(tmpMat, _transform);
    glMultMatrixf(tmpMat);
  }
  if (_normalizeUse) {
    glEnable(GL_NORMALIZE);
  }
  if (lpld->rgba && _colorUse) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  } else {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
    // glColor4fv(white);
  }
  if (_lightingUse) {
    glEnable(GL_LIGHTING);
  }
  if (0 && _flatShading) {
    glShadeModel(GL_FLAT);
  } else {
    glShadeModel(GL_SMOOTH);
  }
  if (!_compiling) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(4, GL_FLOAT, 4*sizeof(float), lpld->xyzw);
    if (lpld->norm && _normalsUse) {
      glEnableClientState(GL_NORMAL_ARRAY);
      glNormalPointer(GL_FLOAT, 3*sizeof(float), lpld->norm);
    }
    if (lpld->rgba && _colorUse) {
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(4, GL_UNSIGNED_BYTE, 4*sizeof(unsigned char), lpld->rgba);
    }
  }
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
  fprintf(stderr, "!%s: _normalsUse = %s\n", me, _normalsUse ? "true" : "false");
  fprintf(stderr, "!%s: _compiling = %s\n", me, _compiling ? "true" : "false");
  */
  unsigned int vertCnt, vertIdx = 0;
  if (!_compiling) {
    if (0) {
      FILE *file;
      file = fopen("all.lmpd", "wb");
      limnPolyDataLMPDWrite(file, lpld);
      fclose(file);
    }
    for (unsigned int primIdx=0; primIdx<lpld->primNum; primIdx++) {
      vertCnt = lpld->icnt[primIdx];
      if (limnPrimitiveNoop != lpld->type[primIdx]) {
        glWhat = glpt[lpld->type[primIdx]];
        /*
        fprintf(stderr, "!%s: glDrawElements(%u, %u, GL_UNSIGNED_INT, %p)\n",
                me, glWhat, vertCnt, lpld->indx + vertIdx);
        */
        glDrawElements(glWhat, vertCnt, GL_UNSIGNED_INT, lpld->indx + vertIdx);
      }
      vertIdx += vertCnt;
    }
  } else {
    for (unsigned int primIdx=0; primIdx<lpld->primNum; primIdx++) {
      vertCnt = lpld->icnt[primIdx];
      if (limnPrimitiveNoop != lpld->type[primIdx]) {
        glWhat = glpt[lpld->type[primIdx]];
        glBegin(glWhat);
        for (unsigned int vii=0; vii<vertCnt; vii++) {
          if (lpld->norm && _normalsUse) {
            glNormal3fv(lpld->norm + 3*(lpld->indx + vertIdx)[vii]);
          }
          if (lpld->rgba && _colorUse) {
            glColor4ubv(lpld->rgba + 4*(lpld->indx + vertIdx)[vii]);
          }
          glVertex4fv(lpld->xyzw + 4*(lpld->indx + vertIdx)[vii]);
        }
        glEnd();
      }
      vertIdx += vertCnt;
    }
  }
  if (!_compiling) {
    glDisableClientState(GL_VERTEX_ARRAY);
    if (lpld->norm && _normalsUse) {
      glDisableClientState(GL_NORMAL_ARRAY);
    }
    if (lpld->rgba && _colorUse) {
      glDisableClientState(GL_COLOR_ARRAY);
    }
  }
  if (_lightingUse) {
    glDisable(GL_LIGHTING);
  }
  if (lpld->rgba && _colorUse) {
    glDisable(GL_COLOR_MATERIAL);
  }
  if (_normalizeUse) {
    glDisable(GL_NORMALIZE);
  }
  if (_transformUse) {
    glPopMatrix();
  }
  // glFinish(); glFlush(); glFinish(); glFlush();
  // GLenum errCode;
  // while ((errCode = glGetError()) != GL_NO_ERROR) {
  //   fprintf(stderr, "\n%s: !!! OpenGL Error: %s\n\n", me,
  //           gluErrorString(errCode));
  // }
  return;
}

void
PolyData::boundsGet(float min[3], float max[3]) const {
  // char me[]="PolyData::boundsGet";
  const limnPolyData *lpld = this->lpld();
  unsigned int vi, xyzwNum;
  float vertB[4], vertC[4];

  ELL_3V_SET(min, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(max, AIR_NAN, AIR_NAN, AIR_NAN);
  if (lpld) {
    bool starting = true;
    xyzwNum = lpld->xyzwNum;
    for (vi=0; vi<xyzwNum; vi++) {
      /*
      fprintf(stderr, "!%s: xyzw[%u] = (%g,%g,%g,%g)\n", me, vi,
              (lpld->xyzw + 4*vi)[0],
              (lpld->xyzw + 4*vi)[1],
              (lpld->xyzw + 4*vi)[2],
              (lpld->xyzw + 4*vi)[3]);
      */
      if (ELL_4V_EXISTS(lpld->xyzw + 4*vi)) {
        ELL_4MV_MUL(vertB, _transform, lpld->xyzw + 4*vi);
        /*
        fprintf(stderr, " --> (%g,%g,%g,%g)", 
                vertB[0], vertB[1], vertB[2], vertB[3]);
        */
        ELL_4V_HOMOG(vertC, vertB);
        /*
        fprintf(stderr, " --> (%g,%g,%g)\n", vertC[0], vertC[1], vertC[2]);
        */
        if (starting) {
          ELL_3V_COPY(min, vertC);
          ELL_3V_COPY(max, vertC);
          starting = false;
        } else {
          ELL_3V_MIN(min, min, vertC);
          ELL_3V_MAX(max, max, vertC);
        }
      }
    }
  }
  return;
}

void
PolyData::IVWrite(FILE *file) const {
  char me[]="PolyData::IVWrite", *err;

  if (limnPolyDataIVWrite(file, this->polyData())) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: couldn't save:\n%s", me, err);
    free(err);
  }
}


} /* namespace Deft */

