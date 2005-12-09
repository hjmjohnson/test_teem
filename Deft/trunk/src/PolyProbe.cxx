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

#include "PolyProbe.h"

enum {
  flagUnknown,
  flagGeometry,
  flagVolume,
  flagQueryColor,
  flagQueryAlphaMask,
  flagQuery,
  flagGageParms,
  flagGageContext,
  flagProbedValues,
  flagColorMap,
  flagInitialRGBA,
  flagBrightness,
  flagAlphaMaskThreshold,
  flagLast
};

namespace Deft {

/* NOTE: we set up the PolyData with _lpldOwn because we plan on using
** Gage to modify the attributes of the fiber path vertices, and the
** probe() method of PolyData can only work with modifiable polydata.
** However, this means that anything inheriting from PolyProbe does not
** have the flexibility of swapping in a different _lpldOwn, it has
** to do a full limnPolyDataCopy, which is lame...
*/
PolyProbe::PolyProbe() : PolyData(limnPolyDataNew(), true) {
  // char me[]="PolyProbe::PolyProbe";

  for (unsigned int fi=flagUnknown+1; fi<flagLast; fi++) {
    _flag[fi] = true;
  }

  _gage = new Gage();
  _cmap = new Cmap();
  _query.resize(2);
  _colorQuantity = colorQuantityUnknown;
  _queryColor.clear();
  _queryAlphaMask.clear();
  _brightnessLutSet(_brightness = 1);
  _colorDoit = true;
  _alphaMaskDoit = true;
  _alphaMaskQuantity = alphaMaskQuantityUnknown;
  _alphaMaskThreshold = 0.5;
  _nlut2D = nrrdNew();
  _nlut1D = nrrdNew();
  _nrmap1D = nrrdNew();
}

PolyProbe::~PolyProbe() {

  delete _gage;
  delete _cmap;
  nrrdNuke(_nlut2D);
  nrrdNuke(_nlut1D);
  nrrdNuke(_nrmap1D);
}

void
PolyProbe::volume(const Volume *vol) {
  _gage->volume(vol);
  _flag[flagVolume] = true;
}

void
PolyProbe::kernel(int which, const NrrdKernelSpec *ksp) {
  _gage->kernel(which, ksp);
  _flag[flagGageParms] = true;
}

const NrrdKernelSpec *
PolyProbe::kernel(int which) const {
  return _gage->kernel(which);
}

void
PolyProbe::kernelReset() { 
  _gage->kernelReset();
  _flag[flagGageParms] = true;
}

void
PolyProbe::color(bool doit) {

  if (_colorDoit != doit) {
    _colorDoit = doit;
    _flag[flagQueryColor] = true;
  }
}

void
PolyProbe::colorQuantity(int quantity) {
  char me[]="PolyProbe::colorQuantity";
  char fname[AIR_STRLEN_MED];
  int lret=0;

  if (airEnumValCheck(Deft::colorQuantity, quantity)) {
    fprintf(stderr, "%s: invalid quantity %d\n", me, quantity);
    return;
  }
  if (_colorQuantity == quantity) {
    // no state change needed
    return;
  }
  // else
  strcpy(fname, homeDir); // start making cmap filenames
  switch(quantity) {
  case colorQuantityRgbLinear:
    _queryColor.resize(1);
    _queryColor[0] = tenGageTensor;
    _cmap->evecRgbWhich(0);
    _cmap->evecRgbAniso(tenAniso_Cl2);
    break;
  case colorQuantityRgbPlanar:
    _queryColor.resize(1);
    _queryColor[0] = tenGageTensor;
    _cmap->evecRgbWhich(2);
    _cmap->evecRgbAniso(tenAniso_Cp2);
    break;
  case colorQuantityModeFA:
    _queryColor.resize(2);
    _queryColor[0] = tenGageTheta;
    _queryColor[1] = tenGageCa2;
    strcat(fname, "Cmap/isobow4-2D.nrrd");
    lret = nrrdLoad(_nlut2D, fname, NULL);
    _cmap->lut2D(_nlut2D);
    _cmap->min2D0(0);
    _cmap->max2D0(1);
    _cmap->min2D1(0);
    _cmap->max2D1(1);
    break;
  case colorQuantityMode:
    _queryColor.resize(1);
    _queryColor[0] = tenGageTheta;
    strcat(fname, "Cmap/isobow4-1D.nrrd");
    lret = nrrdLoad(_nlut1D, fname, NULL);
    _cmap->lut1D(_nlut1D);
    _cmap->min1D(0);
    _cmap->max1D(1);
    break;
  case colorQuantityFA:
    _queryColor.resize(1);
    _queryColor[0] = tenGageFA;
    strcat(fname, "Cmap/gray.nrrd");
    lret = nrrdLoad(_nrmap1D, fname, NULL);
    _cmap->rmap1D(_nrmap1D);
    _cmap->min1D(0);
    _cmap->max1D(1);
    break;
  case colorQuantityClCp:
    _queryColor.resize(2);
    _queryColor[0] = tenGageCl2;
    _queryColor[1] = tenGageCp2;
    strcat(fname, "Cmap/clcp.nrrd");
    lret = nrrdLoad(_nlut2D, fname, NULL);
    _cmap->lut2D(_nlut2D);
    _cmap->min2D0(0);
    _cmap->max2D0(1);
    _cmap->min2D1(0);
    _cmap->max2D1(1);
    break;
  case colorQuantityCl:
    _queryColor.resize(1);
    _queryColor[0] = tenGageCl2;
    strcat(fname, "Cmap/gray.nrrd");
    lret = nrrdLoad(_nrmap1D, fname, NULL);
    _cmap->rmap1D(_nrmap1D);
    _cmap->min1D(0);
    _cmap->max1D(1);
    break;
  case colorQuantityCp:
    _queryColor.resize(1);
    _queryColor[0] = tenGageCp2;
    strcat(fname, "Cmap/gray.nrrd");
    lret = nrrdLoad(_nrmap1D, fname, NULL);
    _cmap->rmap1D(_nrmap1D);
    _cmap->min1D(0);
    _cmap->max1D(1);
    break;
  case colorQuantityCa:
    _queryColor.resize(1);
    _queryColor[0] = tenGageCa2;
    strcat(fname, "Cmap/gray.nrrd");
    lret = nrrdLoad(_nrmap1D, fname, NULL);
    _cmap->rmap1D(_nrmap1D);
    _cmap->min1D(0);
    _cmap->max1D(1);
    break;
  default:
    fprintf(stderr, "%s: unknown colorQuantity %d ??? \n", me, quantity);
    exit(1);
    break;
  }
  if (lret) {
    fprintf(stderr, "%s: PANIC: couldn't load colormap \"%s\"\n", me, fname);
    exit(1);
  }
  _colorQuantity = quantity;
  _flag[flagQueryColor] = true;
  _flag[flagColorMap] = true;
}

void
PolyProbe::_brightnessLutSet(double br) {

  br = AIR_CLAMP(FLT_EPSILON, br, 2.0 - FLT_EPSILON);
  for (unsigned int li=0; li<=255; li++) {
    double val = AIR_AFFINE(0, li, 255, 0.0, 1.0);
    if (br < 1.0) {
      /* go dimmer; use normal gamma */
      val = pow(val, 1.0/br);
    } else {
      /* go brighter */
      val = 1.0 - val;
      val = pow(val, 1.0/(2 - br));
      val = 1.0 - val;
    }
    _brightnessLut[li] = airIndexClamp(0, val, 1, 256);
  }
}

void
PolyProbe::brightness(double br) {
  // char me[]="void PolyProbe::brightness";

  if (_brightness != br) {
    _brightness = br;
    _flag[flagBrightness] = true;
  }
}

void
PolyProbe::alphaMask(bool doit) {

  if (_alphaMaskDoit != doit) {
    _alphaMaskDoit = doit;
    _flag[flagQueryAlphaMask] = true;
  }
}

void
PolyProbe::alphaMaskQuantity(int quantity) {
  
  if (_alphaMaskQuantity == quantity) {
    return;
  }
  // else
  switch(quantity) {
  case alphaMaskQuantityConfidence:
    _queryAlphaMask.resize(1);
    _queryAlphaMask[0] = tenGageConfidence;
    break;
  case alphaMaskQuantityFA:
    _queryAlphaMask.resize(2);
    _queryAlphaMask[0] = tenGageConfidence;
    _queryAlphaMask[1] = tenGageFA;
    break;
  case alphaMaskQuantityCl:
    _queryAlphaMask.resize(2);
    _queryAlphaMask[0] = tenGageConfidence;
    _queryAlphaMask[1] = tenGageCl2;
    break;
  case alphaMaskQuantityCp:
    _queryAlphaMask.resize(2);
    _queryAlphaMask[0] = tenGageConfidence;
    _queryAlphaMask[1] = tenGageCp2;
      break;
  case alphaMaskQuantityCa:
    _queryAlphaMask.resize(2);
    _queryAlphaMask[0] = tenGageConfidence;
    _queryAlphaMask[1] = tenGageCa2;
    break;
  }
  _alphaMaskQuantity = quantity;
  _flag[flagQueryAlphaMask] = true;
}

void
PolyProbe::alphaMaskThreshold(double thresh) {

  if (_alphaMaskThreshold != thresh) {
    _alphaMaskThreshold = thresh;
    _flag[flagAlphaMaskThreshold] = true;
  }
}

void
PolyProbe::evecRgbConfThresh(double confThresh) {
  _cmap->evecRgbConfThresh(confThresh);
  _flag[flagColorMap] = true;
}
void
PolyProbe::evecRgbAnisoGamma(double gamma) {
  _cmap->evecRgbAnisoGamma(gamma);
  _flag[flagColorMap] = true;
}
void
PolyProbe::evecRgbGamma(double gamma) {
  _cmap->evecRgbGamma(gamma);
  _flag[flagColorMap] = true;
}
void
PolyProbe::evecRgbBgGray(double bgGray) {
  _cmap->evecRgbBgGray(bgGray);
  _flag[flagColorMap] = true;
}
void
PolyProbe::evecRgbIsoGray(double isoGray) {
  _cmap->evecRgbIsoGray(isoGray);
  _flag[flagColorMap] = true;
}
void
PolyProbe::evecRgbMaxSat(double maxSat) {
  _cmap->evecRgbMaxSat(maxSat);
  _flag[flagColorMap] = true;
}

bool
PolyProbe::update(bool geometryChanged) {
  // char me[]="PolyProbe::update";
  bool ret = false;

  // well this is weird- the polyprobe class has to be told when its
  // own polydata has changed?  Shouldn't it have a way of knowing?
  _flag[flagGeometry] = geometryChanged;
  /*
  std::cerr << me << ": _flag[flagPlane] = " << _flag[flagPlane] << std::endl;

  std::cerr << me << ": _flag[flagQueryColor,flagQueryAlphaMask] = "
            << _flag[flagQueryColor] << "," 
            << _flag[flagQueryAlphaMask] << std::endl;
  */
  if (_flag[flagQueryColor]
      || _flag[flagQueryAlphaMask]) {
    if (_colorDoit) {
      _query[0] = _queryColor;
    } else {
      _query[0].clear();
    }
    if (_alphaMaskDoit) {
      _query[1] = _queryAlphaMask;
    } else {
      _query[1].clear();
    }
    _gage->query(_query);
    _flag[flagQueryColor] = false;
    _flag[flagQueryAlphaMask] = false;
    _flag[flagQuery] = true;
  }
  if (_flag[flagQuery]
      || _flag[flagGageParms]) {
    /*
    fprintf(stderr, "%s: _gage->update(): %u %u %u\n", me, 
            (unsigned int)(_gage->query().size()),
            (unsigned int)(_gage->query()[0].size()),
            (unsigned int)(_gage->query()[1].size()));
    */
    _gage->update();
    _flag[flagQuery] = false;
    _flag[flagGageParms] = false;
    _flag[flagGageContext] = true;
  }
  if (_flag[flagGeometry]
      || _flag[flagGageContext]) {
    if (_gage->query().size()) {
      probe(_gage);
    }
    _flag[flagGeometry] = false;
    _flag[flagGageContext] = false;
    _flag[flagProbedValues] = true;
  }
  if (_flag[flagProbedValues]
      || _flag[flagColorMap]) {
    if (_colorDoit && _gage->query().size()) {
      dynamic_cast<PolyData*>(this)->color(0, _cmap);
    }
    _flag[flagProbedValues] = false;
    _flag[flagColorMap] = false;
    _flag[flagInitialRGBA] = true;
  }
  /*
  fprintf(stderr, "%s: _flag[flagBrightness] = %s,  = %s\n", me, 
          _flag[flagBrightness] ? "true" : "false",
          _flag[flagInitialRGBA] ? "true" : "false");
  */
  if (_flag[flagBrightness]
      || _flag[flagInitialRGBA]) {
    _brightnessLutSet(_brightness);
    if (_gage->query().size()) {
      dynamic_cast<PolyData*>(this)->RGBLut(_brightnessLut);
    }
    _flag[flagBrightness] = false;
  }
  if (_flag[flagInitialRGBA]
      || _flag[flagAlphaMaskThreshold]) {
    if (_alphaMaskDoit && _gage->query().size()) {
      dynamic_cast<PolyData*>(this)->alphaMask(1, _alphaMaskThreshold);
    }
    _flag[flagInitialRGBA] = false;
    _flag[flagAlphaMaskThreshold] = false;
  }
  return ret;
}

} /* namespace Deft */
