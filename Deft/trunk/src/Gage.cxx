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

#include "Gage.h"

#define GAGE_ERROR \
      fprintf(stderr, "%s: problem:\n%s", me, err=biffGetDone(GAGE)); \
      err = AIR_CAST(char *, airFree(err)); \
      return

namespace Deft {

#define answerCopy(N) \
void \
_answerCopy_##N (double *dst, const gage_t *src) { \
  for (unsigned int ii=0; ii<N; ii++) { \
    *dst++ = *src++; \
  } \
}

answerCopy(1);
answerCopy(2);
answerCopy(3);
answerCopy(4);
answerCopy(5);
answerCopy(6);
answerCopy(7);
answerCopy(8);
answerCopy(9);
answerCopy(10);
answerCopy(11);
answerCopy(12);

void
(*_answerCopy_[13])(double *dst, const gage_t *src) = {
  NULL,
  _answerCopy_1,
  _answerCopy_2,
  _answerCopy_3,
  _answerCopy_4,
  _answerCopy_5,
  _answerCopy_6,
  _answerCopy_7,
  _answerCopy_8,
  _answerCopy_9,
  _answerCopy_10,
  _answerCopy_11,
  _answerCopy_12
};

Gage::Gage() {
  _gctx = gageContextNew();
  _pvl = NULL;
}

Gage::~Gage() {
  _gctx = gageContextNix(_gctx);
}

// we maintain things so that there is at most one volume in the context
void
Gage::volume(const Volume *vol) {
  char me[]="Gage::volume", *err;

  if (_gctx->pvlNum) {
    gagePerVolume *pvltmp = _gctx->pvl[0];
    gagePerVolumeDetach(_gctx, pvltmp);
    gagePerVolumeNix(pvltmp);
  }
  if ( !(_pvl = gagePerVolumeNew(_gctx, vol->nrrd(), vol->kind()))
       || gagePerVolumeAttach(_gctx, _pvl) ) {
    GAGE_ERROR;
  }
  _volume = vol;
  _item.clear();
  _answer.clear();
  _answerLength.clear();
  _answerCopy.clear();
  return;
}

void
Gage::kernel(int which, const NrrdKernelSpec *ksp) {
  char me[]="Gage::kernel", *err;
  int ret;

  ret = gageKernelSet(_gctx, which, ksp->kernel, ksp->parm);
  if (ret) {
    GAGE_ERROR;
  }
}

const NrrdKernelSpec *
Gage::kernel(int which) const {
  return (AIR_IN_OP(gageKernelUnknown, which, gageKernelLast)
          ? _gctx->ksp[which]
          : NULL);
}

void
Gage::kernelReset() {
  gageKernelReset(_gctx);
}

void
Gage::query(std::vector<std::vector<int> > item) {
  _item = item;
}

void
Gage::query(std::vector<int> item) {
  _item.resize(1);
  _item[0] = item;
}

void
Gage::query(int item) {
  _item.resize(1);
  _item[0].resize(1);
  _item[0][0] = item;
}

unsigned int
Gage::answerLength(int item) const {
  return gageKindAnswerLength(_pvl->kind, item);
}

unsigned int
Gage::answerLength(std::vector<int> item) const {
  unsigned int ret = 0;
  for (unsigned int ii=0; ii<item.size(); ii++) {
    ret += answerLength(item[ii]);
  }
  return ret;
}

void
Gage::_answerInit() {
  _answer.resize(_item.size());
  _answerLength.resize(_item.size());
  _answerCopy.resize(_item.size());
  for (unsigned int qi=0; qi<_item.size(); qi++) {
    _answer[qi].resize(_item[qi].size());
    _answerLength[qi].resize(_item[qi].size());
    _answerCopy[qi].resize(_item[qi].size());
    for (unsigned int ii=0; ii<_item[qi].size(); ii++) {
      _answer[qi][ii] = gageAnswerPointer(_gctx, _pvl, _item[qi][ii]);
      _answerLength[qi][ii] = answerLength(_item[qi][ii]);
      _answerCopy[qi][ii] = _answerCopy_[_answerLength[qi][ii]];
    }
  }
}

void
Gage::update() {
  char me[]="Gage::update", *err;
  int E = 0;

  // fprintf(stderr, "%s: _gctx = %p, _pvl = %p, _item.size()=%u\n",
  // me, _gctx, _pvl, _item.size());
  if (!E) E |= gageQueryReset(_gctx, _pvl);
  for (unsigned int qi=0; qi<_item.size(); qi++) {
    for (unsigned int ii=0; ii<_item[qi].size(); ii++) {
      if (!E) E |= gageQueryItemOn(_gctx, _pvl, _item[qi][ii]);
    }
  }
  if (!E) E |= gageUpdate(_gctx);
  if (E) {
    GAGE_ERROR;
  }
  _answerInit();
}

int
Gage::probe(double **answerAll, gage_t x, gage_t y, gage_t z) const {
  // static int count = 0;
  // char me[]="Gage::_probe";
  double *answer;
  int ret;

  /*
  if (count < 10) {
    fprintf(stderr, "%s: (%g,%g,%g)\n", me, x, y, z);
  }
  */
  ret = gageProbeSpace(_gctx, x, y, z, AIR_FALSE, AIR_FALSE);
  if (ret) {
    return ret;
  }
  for (unsigned int qi=0; qi<_item.size(); qi++) {
    answer = answerAll[qi];
    for (unsigned int ii=0; ii<_item[qi].size(); ii++) {
      /*
      if (count < 10 
          || (x == 7.5 && y == 52)) {
      fprintf(stderr, "%s(%g,%g,%g): [%u][%u] answer = %p\n", me,
              x, y, z, qi, ii, answer);
      }
      */
      _answerCopy[qi][ii](answer, _answer[qi][ii]);
      answer += _answerLength[qi][ii];
    }
  }
  // count++;
  return 0;
}

void
Gage::verbose(int v) {
  gageParmSet(_gctx, gageParmVerbose, AIR_CAST(gage_t, v));
}

int
Gage::verbose() {
  return _gctx->verbose;
}


} /* namespace Deft */
