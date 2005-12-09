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

#ifndef DEFT_GAGE_INCLUDED
#define DEFT_GAGE_INCLUDED

#include "Deft.h"
#include "Volume.h"

#include <vector>

#include <teem/gage.h>

namespace Deft {

class DEFT_EXPORT Gage {
public: 
  // for now, we only can have a single volume per context, but it wouldn't
  // be hard to change this later.  Use case: probe the T2 reference volume
  // along with the DWI volume, or the DTI along with the DWI volume.  Have
  // to resolve if you can have two volumes of the same kind, since this
  // would make query specification more complicated...
  explicit Gage();
  ~Gage();

  void volume(const Volume *volume);
  const Volume *volume() const { return _volume; }
  
  // for setting and getting kernels
  void kernel(int which, const NrrdKernelSpec *ksp);
  const NrrdKernelSpec *kernel(int which) const;
  void kernelReset();

  // for setting the query, with convenience/simplified versions
  void query(std::vector<std::vector<int> > item);
  void query(std::vector<int> item);
  void query(int item);
  std::vector<std::vector<int> > query() const { return _item; }

  // learning about answer lengths
  unsigned int answerLength(int item) const;
  unsigned int answerLength(std::vector<int> item) const;

  // wraps gageUpdate()
  void update();

  // the return is non-zero if (x,y,z) was outside the volume
  // NOTE: can only probe in world space (not in gage's native index space)
  // NOTE: is it really a const member function?  No compiler complaints!
  int probe(double **answer, gage_t x, gage_t y, gage_t z) const;
  
  // other parameters or flags
  void verbose(int v);
  int verbose();

private:
  const Volume *_volume;
  gageContext *_gctx;
  gagePerVolume *_pvl;
  void _answerInit();
  std::vector<std::vector<int> > _item;
  std::vector<std::vector<const gage_t *> > _answer;
  std::vector<std::vector<unsigned int> > _answerLength;
  std::vector<std::vector<void (*)(double *, const gage_t *)> > _answerCopy;
};

} /* namespace Deft */

#endif /* DEFT_GAGE_INCLUDED */
