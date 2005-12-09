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

#ifndef DEFT_CMAP_INCLUDED
#define DEFT_CMAP_INCLUDED

#include "Deft.H"

#include "Values.H"

namespace Deft {

enum {
  cmapMode1DUnknown,
  cmapMode1DGrayMinMax,
  cmapMode1DLut1D,
  cmapMode1DRmap1D,
  cmapMode1DImap1D,
  cmapMode1DLast
};

class DEFT_EXPORT Cmap {
public: 
  explicit Cmap();
  ~Cmap();

  void mode1D(int m);
  int mode1D() const;

  void lut1D(const Nrrd *nlut1D);
  const Nrrd *lut1D() const;

  void rmap1D(const Nrrd *nrmap1D);
  const Nrrd *rmap1D() const;

  void imap1D(const Nrrd *nimap1D);
  const Nrrd *imap1D() const;

  // for controlling domain of all 1D maps (luts, rmaps, imaps)
  void min1D(double min);
  void max1D(double max);
  double min1D() const;
  double max1D() const;

  void lut2D(const Nrrd *nlut2D);
  const Nrrd *lut2D() const;

  // for controlling domain of 2D maps (currently only luts)
  void min2D0(double min);
  void max2D0(double max);
  void min2D1(double min);
  void max2D1(double max);
  double min2D0() const;
  double max2D0() const;
  double min2D1() const;
  double max2D1() const;

  // for controlling mapping of 3D eigenvectors
  void evecRgbWhich(unsigned int which);
  void evecRgbAniso(int aniso);
  void evecRgbConfThresh(double confThresh);
  void evecRgbAnisoGamma(double gamma);
  void evecRgbGamma(double gamma);
  void evecRgbBgGray(double bgGray);
  void evecRgbIsoGray(double isoGray);
  void evecRgbMaxSat(double maxSat);
  unsigned int evecRgbWhich() const { return _rgbp->which; }
  int evecRgbAniso() const { return _rgbp->aniso; }
  double evecRgbConfThresh() const { return _rgbp->confThresh; }
  double evecRgbAnisoGamma() const { return _rgbp->anisoGamma; }
  double evecRgbGamma() const { return _rgbp->gamma; }
  double evecRgbBgGray() const { return _rgbp->bgGray; }
  double evecRgbIsoGray() const { return _rgbp->isoGray; }
  double evecRgbMaxSat() const { return _rgbp->maxSat; }

  void map(Nrrd *nrgba, const Values *val) const;

private:
  int _mode1D;        /* from cmapMode1D* enum */
  NrrdRange *_range1D, *_range2D0, *_range2D1;
  const Nrrd *_nlut1D, *_nrmap1D, *_nimap1D, *_nlut2D;
  tenEvecRGBParm *_rgbp;
};

} /* namespace Deft */

#endif /* DEFT_CMAP_INCLUDED */
