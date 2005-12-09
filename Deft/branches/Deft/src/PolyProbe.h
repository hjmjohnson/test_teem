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

#ifndef DEFT_POLY_PROBE_INCLUDED
#define DEFT_POLY_PROBE_INCLUDED

#include "Deft.H"
#include "PolyData.H"
#include "Gage.H"
#include "Cmap.H"

namespace Deft {

class DEFT_EXPORT PolyProbe : public PolyData {
public: 
  explicit PolyProbe();
  ~PolyProbe();

  // TERRIBLE: API copied from DeftGage ... 
  void volume(const Volume *volume);
  const Volume *volume() const { return _gage->volume(); }
  void kernel(int which, const NrrdKernelSpec *ksp);
  const NrrdKernelSpec *kernel(int which) const;
  void kernelReset();

  // this at once sets both a query and colormap
  void color(bool doit);
  bool color() const { return _colorDoit; }
  void colorQuantity(int quantity);
  int colorQuantity() const { return _colorQuantity; }

  // for alpha masking, both the quantity and threshold have
  // to be indicated explicitly
  // Note: this used to take a proper gage-style std::vector<int> query,
  // but it was more annoying to attach a bidirectional user interface:
  // the UI has to learn the object state and set itself, and a single
  // enum value makes that easiest
  void alphaMask(bool doit);
  bool alphaMask() const { return _alphaMaskDoit; }
  void alphaMaskQuantity(int quantity);
  int alphaMaskQuantity() const { return _alphaMaskQuantity; }
  void alphaMaskThreshold(double thresh);
  double alphaMaskThreshold() const { return _alphaMaskThreshold; }

  // per-component gamma-ish brightening
  void brightness(double brightness);
  double brightness() const { return _brightness; }

  // RGB(evec) parameters.  HEY: API copied from DeftCmap
  void evecRgbConfThresh(double confThresh);
  void evecRgbAnisoGamma(double gamma);
  void evecRgbGamma(double gamma);
  void evecRgbBgGray(double bgGray);
  void evecRgbIsoGray(double isoGray);
  void evecRgbMaxSat(double maxSat);
  double evecRgbConfThresh() const { return _cmap->evecRgbConfThresh(); }
  double evecRgbAnisoGamma() const { return _cmap->evecRgbAnisoGamma(); }
  double evecRgbGamma() const { return _cmap->evecRgbGamma(); }
  double evecRgbBgGray() const { return _cmap->evecRgbBgGray(); }
  double evecRgbIsoGray() const { return _cmap->evecRgbIsoGray(); }
  double evecRgbMaxSat() const { return _cmap->evecRgbMaxSat(); }

  // returns false when update was a no-up
  bool update(bool geometryChanged);

private:
  bool _flag[DEFT_FLAG_NUM_MAX];
  bool _colorDoit, _alphaMaskDoit;
  int _colorQuantity, _alphaMaskQuantity;
  std::vector<int> _queryColor, _queryAlphaMask;
  // _query[0] and [1] get reset with every update()
  std::vector<std::vector<int> > _query;
  unsigned char _brightnessLut[256];
  void _brightnessLutSet(double br);
  double _alphaMaskThreshold, _brightness;
  Gage *_gage;
  Cmap *_cmap;
  Nrrd *_nlut2D, *_nlut1D, *_nrmap1D;
};

} /* namespace Deft */

#endif /* DEFT_POLY_PROBE_INCLUDED */
