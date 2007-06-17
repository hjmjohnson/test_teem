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

#include "Gage.h"

#define GAGE_ERROR \
      fprintf(stderr, "%s: problem:\n%s", me, err=biffGetDone(GAGE)); \
      err = AIR_CAST(char *, airFree(err)); \
      return

namespace Deft {

// HEY!!  this is VERY VERY  stupid and I don't know why I'm doing it

#define ansCopy(N) \
void \
_ansCopy_##N (double *dst, const double *src) { \
  for (unsigned int ii=0; ii<N; ii++) { \
    *dst++ = *src++; \
  } \
}

// HEY: yea, this puts an un-advertised upper cap on item length!!

                ansCopy(1);   ansCopy(2);   ansCopy(3);   ansCopy(4); 
  ansCopy(5);   ansCopy(6);   ansCopy(7);   ansCopy(8);   ansCopy(9);
 ansCopy(10);  ansCopy(11);  ansCopy(12);  ansCopy(13);  ansCopy(14);
 ansCopy(15);  ansCopy(16);  ansCopy(17);  ansCopy(18);  ansCopy(19);
 ansCopy(20);  ansCopy(21);  ansCopy(22);  ansCopy(23);  ansCopy(24);
 ansCopy(25);  ansCopy(26);  ansCopy(27);  ansCopy(28);  ansCopy(29);
 ansCopy(30);  ansCopy(31);  ansCopy(32);  ansCopy(33);  ansCopy(34);
 ansCopy(35);  ansCopy(36);  ansCopy(37);  ansCopy(38);  ansCopy(39);
 ansCopy(40);  ansCopy(41);  ansCopy(42);  ansCopy(43);  ansCopy(44);
 ansCopy(45);  ansCopy(46);  ansCopy(47);  ansCopy(48);  ansCopy(49);
 ansCopy(50);  ansCopy(51);  ansCopy(52);  ansCopy(53);  ansCopy(54);
 ansCopy(55);  ansCopy(56);  ansCopy(57);  ansCopy(58);  ansCopy(59);
 ansCopy(60);  ansCopy(61);  ansCopy(62);  ansCopy(63);  ansCopy(64);
 ansCopy(65);  ansCopy(66);  ansCopy(67);  ansCopy(68);  ansCopy(69);
 ansCopy(70);  ansCopy(71);  ansCopy(72);  ansCopy(73);  ansCopy(74);
 ansCopy(75);  ansCopy(76);  ansCopy(77);  ansCopy(78);  ansCopy(79);
 ansCopy(80);  ansCopy(81);  ansCopy(82);  ansCopy(83);  ansCopy(84);
 ansCopy(85);  ansCopy(86);  ansCopy(87);  ansCopy(88);  ansCopy(89);
 ansCopy(90);  ansCopy(91);  ansCopy(92);  ansCopy(93);  ansCopy(94);
 ansCopy(95);  ansCopy(96);  ansCopy(97);  ansCopy(98);  ansCopy(99);
ansCopy(100); ansCopy(101); ansCopy(102); ansCopy(103); ansCopy(104);
ansCopy(105); ansCopy(106); ansCopy(107); ansCopy(108); ansCopy(109);
ansCopy(110); ansCopy(111); ansCopy(112); ansCopy(113); ansCopy(114);
ansCopy(115); ansCopy(116); ansCopy(117); ansCopy(118); ansCopy(119);
ansCopy(120); ansCopy(121); ansCopy(122); ansCopy(123); ansCopy(124);
ansCopy(125); ansCopy(126); ansCopy(127); ansCopy(128); ansCopy(129);
ansCopy(130); ansCopy(131); ansCopy(132); ansCopy(133); ansCopy(134);
ansCopy(135); ansCopy(136); ansCopy(137); ansCopy(138); ansCopy(139);
ansCopy(140); ansCopy(141); ansCopy(142); ansCopy(143); ansCopy(144);
ansCopy(145); ansCopy(146); ansCopy(147); ansCopy(148); ansCopy(149);

void
(*_ansCopyFunc[150])(double *dst, const double *src) = {
  NULL,
                _ansCopy_1,   _ansCopy_2,   _ansCopy_3,   _ansCopy_4, 
  _ansCopy_5,   _ansCopy_6,   _ansCopy_7,   _ansCopy_8,   _ansCopy_9,
 _ansCopy_10,  _ansCopy_11,  _ansCopy_12,  _ansCopy_13,  _ansCopy_14,
 _ansCopy_15,  _ansCopy_16,  _ansCopy_17,  _ansCopy_18,  _ansCopy_19,
 _ansCopy_20,  _ansCopy_21,  _ansCopy_22,  _ansCopy_23,  _ansCopy_24,
 _ansCopy_25,  _ansCopy_26,  _ansCopy_27,  _ansCopy_28,  _ansCopy_29,
 _ansCopy_30,  _ansCopy_31,  _ansCopy_32,  _ansCopy_33,  _ansCopy_34,
 _ansCopy_35,  _ansCopy_36,  _ansCopy_37,  _ansCopy_38,  _ansCopy_39,
 _ansCopy_40,  _ansCopy_41,  _ansCopy_42,  _ansCopy_43,  _ansCopy_44,
 _ansCopy_45,  _ansCopy_46,  _ansCopy_47,  _ansCopy_48,  _ansCopy_49,
 _ansCopy_50,  _ansCopy_51,  _ansCopy_52,  _ansCopy_53,  _ansCopy_54,
 _ansCopy_55,  _ansCopy_56,  _ansCopy_57,  _ansCopy_58,  _ansCopy_59,
 _ansCopy_60,  _ansCopy_61,  _ansCopy_62,  _ansCopy_63,  _ansCopy_64,
 _ansCopy_65,  _ansCopy_66,  _ansCopy_67,  _ansCopy_68,  _ansCopy_69,
 _ansCopy_70,  _ansCopy_71,  _ansCopy_72,  _ansCopy_73,  _ansCopy_74,
 _ansCopy_75,  _ansCopy_76,  _ansCopy_77,  _ansCopy_78,  _ansCopy_79,
 _ansCopy_80,  _ansCopy_81,  _ansCopy_82,  _ansCopy_83,  _ansCopy_84,
 _ansCopy_85,  _ansCopy_86,  _ansCopy_87,  _ansCopy_88,  _ansCopy_89,
 _ansCopy_90,  _ansCopy_91,  _ansCopy_92,  _ansCopy_93,  _ansCopy_94,
 _ansCopy_95,  _ansCopy_96,  _ansCopy_97,  _ansCopy_98,  _ansCopy_99,
_ansCopy_100, _ansCopy_101, _ansCopy_102, _ansCopy_103, _ansCopy_104,
_ansCopy_105, _ansCopy_106, _ansCopy_107, _ansCopy_108, _ansCopy_109,
_ansCopy_110, _ansCopy_111, _ansCopy_112, _ansCopy_113, _ansCopy_114,
_ansCopy_115, _ansCopy_116, _ansCopy_117, _ansCopy_118, _ansCopy_119,
_ansCopy_120, _ansCopy_121, _ansCopy_122, _ansCopy_123, _ansCopy_124,
_ansCopy_125, _ansCopy_126, _ansCopy_127, _ansCopy_128, _ansCopy_129,
_ansCopy_130, _ansCopy_131, _ansCopy_132, _ansCopy_133, _ansCopy_134,
_ansCopy_135, _ansCopy_136, _ansCopy_137, _ansCopy_138, _ansCopy_139,
_ansCopy_140, _ansCopy_141, _ansCopy_142, _ansCopy_143, _ansCopy_144,
_ansCopy_145, _ansCopy_146, _ansCopy_147, _ansCopy_148, _ansCopy_149
};

Gage::Gage() {
  _item.resize(0);
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

bool
Gage::querySet() const {
  unsigned int alen = 0;
  for (unsigned int ii=0; ii<_item.size(); ii++) {
    alen += _item[ii].size();
  }
  return alen > 0;
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
      _answerCopy[qi][ii] = _ansCopyFunc[_answerLength[qi][ii]];
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
Gage::probe(double **answerAll, double x, double y, double z) const {
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
  gageParmSet(_gctx, gageParmVerbose, v);
}

int
Gage::verbose() {
  return _gctx->verbose;
}


} /* namespace Deft */
