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

#ifndef DEFT_ANISOCONTOUR_INCLUDED
#define DEFT_ANISOCONTOUR_INCLUDED

#include "Deft.h"
#include "Volume.h"
#include "PolyProbe.h"

namespace Deft {

class DEFT_EXPORT Anisocontour : public PolyProbe {
public:
  explicit Anisocontour(const Volume *vol);
  ~Anisocontour();

  // log_2 of sampling factor for each axis
  void sampling(unsigned int axisIdx, double smpl);
  double sampling(unsigned int axisIdx) const {
    return _sampling[AIR_MIN(axisIdx, 2)]; 
  }
  void kernel(const NrrdKernelSpec *ksp);
  const NrrdKernelSpec *kernel() const { return _ksp; }

  void anisoType(int aniso);
  int anisoType() const { return _anisoType; }
  void maskThresh(double maskThresh);
  double maskThresh() const { return _maskThresh; }

  void isovalue(double isovalue);
  double isovalue() const { return _isovalue; }

  void glyphsDo(bool doit);
  bool glyphsDo() const { return _glyphsDo; }
  
  void tractsDo(bool doit);
  bool tractsDo() const { return _tractsDo; }

  void update();

private:
  bool _flag[128], _glyphsDo, _tractsDo;
  const Volume *_volume;
  NrrdResampleContext *_rsmc;
  seekContext *_sctx;
  int _anisoType;
  double _maskThresh, _isovalue, _sampling[3];
  NrrdKernelSpec *_ksp;
  double kparm[NRRD_KERNEL_PARMS_NUM];
  Nrrd *_ntenRsmp, *_naniso;
};

} /* namespace Deft */

#endif /* DEFT_ANISOCONTOUR_INCLUDED */
