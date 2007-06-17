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

#ifndef DEFT_ANISOFEATURE_INCLUDED
#define DEFT_ANISOFEATURE_INCLUDED

#include "Deft.h"
#include "Volume.h"
#include "PolyProbe.h"

namespace Deft {

class DEFT_EXPORT Anisofeature : public PolyProbe {
public:
  explicit Anisofeature(const Volume *vol);
  explicit Anisofeature(const Volume *vol, const limnPolyData *lpld);
  ~Anisofeature();

  // log_2 of sampling factor *all* axes
  void sampling(double smpl);
  double sampling() const;

  // for now, only allow cubic:S,1,0 kernel and its derivatives
  void scale(double scl);
  double scale() const { return _scale; }

  // what type of feature we extract
  // AFTER SETTING THIS YOU MUST (RE)SET THE (gage) ITEMS BELOW
  void type(int type);
  int type() const { return _sctx->type; }

  void itemScalar(int item);
  int itemScalar() const { return _sctx->sclvItem; }
  void itemStrength(int item);
  int itemStrength() const { return _sctx->stngItem; }
  void itemNormal(int item);
  int itemNormal() const { return _sctx->normItem; }
  void itemGradient(int item);
  int itemGradient() const { return _sctx->gradItem; }
  void itemEigensystem(int evalItem, int evecItem);
  int itemEigenvalue() const { return _sctx->evalItem; }
  int itemEigenvector() const { return _sctx->evecItem; }

  void strengthUse(bool doit);
  bool strengthUse() const { return _sctx->strengthUse; } 
  void strength(int sign, double strength);
  int strengthSign() const { return _sctx->strengthSign; }
  double strength() const { return _sctx->strength; }

  void isovalue(double isovalue);
  double isovalue() const { return _sctx->isovalue; }

  void glyphsDo(bool doit);
  bool glyphsDo() const { return _glyphsDo; }
  
  void tractsDo(bool doit);
  bool tractsDo() const { return _tractsDo; }

  void ccDo(bool doit);
  bool ccDo() const { return _ccDo; }
  void ccSingle(bool doit);
  bool ccSingle() const { return _ccSingle; }
  void ccId(unsigned int id);
  unsigned int ccId() const { return _ccId; }
  unsigned int ccNum() const;

  void update();

private:
  bool _flag[128], _ccDo, _ccSingle, _hack;
  unsigned int _ccId;
  const Volume *_volume;
  gageContext *_gctx;
  gagePerVolume *_gpvl;
  seekContext *_sctx;
  limnPolyData *_lpldWhole, *_lpldCC, *_lpldSelect, *_lpldOrig;
  NrrdKernelSpec *_ksp00, *_ksp11, *_ksp22;
  bool _glyphsDo, _tractsDo;
  double _maskThresh, _sampling;
  double kparm[NRRD_KERNEL_PARMS_NUM], _scale;
};

} /* namespace Deft */

#endif /* DEFT_ANISOFEATURE_INCLUDED */
