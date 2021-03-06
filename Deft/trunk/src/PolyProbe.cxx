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

#include "PolyProbe.h"

// #include <iostream>

enum {
  flagUnknown,
  flagGeometry,
  flagVolume,
  flagQueryColor,
  flagQueryNoColor,
  flagQueryAlphaMask,
  flagQuery,
  flagGageKernels,
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
    _flag[fi] = false;
  }
  _colorQuantityEnum = NULL;
  _alphaMaskQuantityEnum = NULL;

  _gage = new Gage();
  _cmap = new Cmap();
  _query.resize(2);
  _colorQuantity = 0; /* 0 is "unknown" for all kinds */
  _queryColor.clear();
  _queryNoColor.clear();
  _queryAlphaMask.clear();
  _brightnessLutSet(_brightness = 1);
  _colorDoit = false;
  _alphaMaskDoit = false;
  _alphaMaskQuantity = 0; /* 0 is "unknown" for all kinds */
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
  char me[]="PolyProbe::volume";

  _gage->volume(vol);
  if (tenGageKind == vol->kind()) {
    _colorQuantityEnum = colorTenQuantity;
    _alphaMaskQuantityEnum = alphaMaskTenQuantity;
  } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, vol->kind()->name)) {
    _colorQuantityEnum = colorDwiQuantity;
    _alphaMaskQuantityEnum = alphaMaskDwiQuantity;
  } else if (gageKindScl == vol->kind()) {
    _colorQuantityEnum = colorSclQuantity;
    _alphaMaskQuantityEnum = alphaMaskSclQuantity;
  } else {
    fprintf(stderr, "!%s: unrecognized kind %s!!!\n", me, vol->kind()->name);
    return;
  }
  _flag[flagVolume] = true;
}

void
PolyProbe::kernel(int which, const NrrdKernelSpec *ksp) {
  _gage->kernel(which, ksp);
  _flag[flagGageKernels] = true;
}

const NrrdKernelSpec *
PolyProbe::kernel(int which) const {
  return _gage->kernel(which);
}

void
PolyProbe::kernelReset() { 
  _gage->kernelReset();
  _flag[flagGageKernels] = true;
}

void
PolyProbe::lpldCopy(const limnPolyData *lpld) {
  limnPolyDataCopy(_lpldOwn, lpld);
  _flag[flagGeometry] = true;
  return;
}

void
PolyProbe::color(bool doit) {

  /*
  fprintf(stderr, "!%s: _colorDoit = %s, doit = %s\n", 
          "PolyProbe::color", 
          _colorDoit ? "true" : "false",
          doit ? "true" : "false");
  */
  if (_colorDoit != doit) {
    _colorDoit = doit;
    _flag[flagQueryColor] = true;
    /*
    fprintf(stderr, "!%s: _flag[flagQueryColor] = %s\n", 
            "PolyProbe::color", 
            _flag[flagQueryColor] ? "true" : "false");
    */
  }
}

void
PolyProbe::noColorQuery(int item) {
  
  // HEY: need a way to test no change from last time
  
  this->color(false);
  _queryNoColor.resize(1);
  _queryNoColor[0] = item;
  _flag[flagQueryNoColor] = true;
}

void
PolyProbe::noColorQuery(int item0, int item1) {
  
  // HEY: need a way to test no change from last time
  
  this->color(false);
  _queryNoColor.resize(2);
  _queryNoColor[0] = item0;
  _queryNoColor[1] = item1;
  _flag[flagQueryNoColor] = true;
}

void
PolyProbe::noColorQuery(int item0, int item1, int item2) {
  
  // HEY: need a way to test no change from last time
  
  this->color(false);
  _queryNoColor.resize(3);
  _queryNoColor[0] = item0;
  _queryNoColor[1] = item1;
  _queryNoColor[2] = item2;
  _flag[flagQueryNoColor] = true;
}

void
PolyProbe::colorQuantity(int quantity) {
  char me[]="PolyProbe::colorQuantity";
  char fname[AIR_STRLEN_MED];
  int lret=0;

  if (airEnumValCheck(this->colorQuantityEnum(), quantity)) {
    fprintf(stderr, "%s: invalid %s quantity %d\n", me,
            this->colorQuantityEnum()->name, quantity);
    return;
  }
  /*
  fprintf(stderr, "%s: got %s %s=%d, have %s=%d\n", me, 
          this->colorQuantityEnum()->name,
          airEnumStr(this->colorQuantityEnum(), quantity),
          quantity,
          airEnumStr(this->colorQuantityEnum(), _colorQuantity),
          _colorQuantity);
  */
  if (_colorQuantity == quantity) {
    // no state change needed
    // fprintf(stderr, "!%s: no change needed\n", me);
    return;
  }
  // else
  strcpy(fname, homeDir); // start making cmap filenames
  fprintf(stderr, "%s: fname = |%s|\n", me, fname);
  if (tenGageKind == _gage->volume()->kind()) {
    switch(quantity) {
    case colorTenQuantityConf:
      _queryColor.resize(1);
      _queryColor[0] = tenGageConfidence;
      strcat(fname, "cmap/isobow4-1D.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(1);
      _cmap->max1D(1.77);
      break;
    case colorTenQuantityRgbLinear:
      _queryColor.resize(1);
      _queryColor[0] = tenGageTensor;
      _cmap->evecRgbWhich(0);
      _cmap->evecRgbAniso(tenAniso_Cl2);
      break;
    case colorTenQuantityRgbPlanar:
      _queryColor.resize(1);
      _queryColor[0] = tenGageTensor;
      _cmap->evecRgbWhich(2);
      _cmap->evecRgbAniso(tenAniso_Cp2);
      break;
    case colorTenQuantityModeFA:
      _queryColor.resize(2);
      _queryColor[0] = tenGageMode;
      _queryColor[1] = tenGageCa2;
      strcat(fname, "cmap/isobow4-2D.nrrd");
      lret = nrrdLoad(_nlut2D, fname, NULL);
      _cmap->lut2D(_nlut2D);
      _cmap->min2D0(-1);
      _cmap->max2D0(1);
      _cmap->min2D1(0);
      _cmap->max2D1(1);
      break;
    case colorTenQuantityMode:
      _queryColor.resize(1);
      _queryColor[0] = tenGageMode;
      strcat(fname, "cmap/isobow4-1D.nrrd");
      lret = nrrdLoad(_nlut1D, fname, NULL);
      _cmap->lut1D(_nlut1D);
      _cmap->min1D(-1);
      _cmap->max1D(1);
      break;
    case colorTenQuantityFA:
      _queryColor.resize(1);
      _queryColor[0] = tenGageFA;
      strcat(fname, "cmap/gray.nrrd");
      // strcat(fname, "cmap/lines.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      _cmap->max1D(0.881);
      break;
    case colorTenQuantityOmega:
      _queryColor.resize(1);
      _queryColor[0] = tenGageOmega;
      strcat(fname, "cmap/gray.nrrd");
      // strcat(fname, "cmap/lines.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      _cmap->max1D(0.881);
      break;
    case colorTenQuantityTrace:
      _queryColor.resize(1);
      _queryColor[0] = tenGageTrace;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(0.01); // HEY: hack!
      break;
    case colorTenQuantityClCp:
      _queryColor.resize(2);
      _queryColor[0] = tenGageCl2;
      _queryColor[1] = tenGageCp2;
      strcat(fname, "cmap/clcp.nrrd");
      lret = nrrdLoad(_nlut2D, fname, NULL);
      _cmap->lut2D(_nlut2D);
      _cmap->min2D0(0);
      _cmap->max2D0(1);
      _cmap->min2D1(0);
      _cmap->max2D1(1);
      break;
    case colorTenQuantityCl:
      _queryColor.resize(1);
      _queryColor[0] = tenGageCl2;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      break;
    case colorTenQuantityCp:
      _queryColor.resize(1);
      _queryColor[0] = tenGageCp2;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      break;
    case colorTenQuantityCa:
      _queryColor.resize(1);
      _queryColor[0] = tenGageCa2;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      break;
    case colorTenQuantityTrGradVecDotEvec0:
    case colorTenQuantityFAGradVecDotEvec0:
    case colorTenQuantityOmegaGradVecDotEvec0:
      _queryColor.resize(1);
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      switch(quantity) {
      case colorTenQuantityTrGradVecDotEvec0:
        _queryColor[0] = tenGageTraceGradVecDotEvec0;
        _cmap->max1D(0.002); // HEY: hack!
        break;
      case colorTenQuantityFAGradVecDotEvec0:
        _queryColor[0] = tenGageFAGradVecDotEvec0;
        _cmap->max1D(0.33); // HEY: hack!
        break;
      case colorTenQuantityOmegaGradVecDotEvec0:
        _queryColor[0] = tenGageOmegaGradVecDotEvec0;
        _cmap->max1D(0.33); // HEY: hack!
        break;
      }
      break;
    case colorTenQuantityTrDiffusionAlign:
    case colorTenQuantityFADiffusionAlign:
    case colorTenQuantityOmegaDiffusionAlign:
    case colorTenQuantityConfDiffusionAlign:
      _queryColor.resize(1);
      strcat(fname, "cmap/isobow4-1D.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      switch(quantity) {
      case colorTenQuantityTrDiffusionAlign:
        _queryColor[0] = tenGageTraceDiffusionAlign;
        break;
      case colorTenQuantityFADiffusionAlign:
        _queryColor[0] = tenGageFADiffusionAlign;
        break;
      case colorTenQuantityOmegaDiffusionAlign:
        _queryColor[0] = tenGageOmegaDiffusionAlign;
        break;
      case colorTenQuantityConfDiffusionAlign:
        _queryColor[0] = tenGageConfDiffusionAlign;
        break;
      }
      break;
    case colorTenQuantityTrDiffusionFraction:
    case colorTenQuantityFADiffusionFraction:
    case colorTenQuantityOmegaDiffusionFraction:
    case colorTenQuantityConfDiffusionFraction:
      _queryColor.resize(1);
      strcat(fname, "cmap/isobow4-1D.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0.26); // HEY: hack!
      _cmap->max1D(0.44); // HEY: hack!
      switch(quantity) {
      case colorTenQuantityTrDiffusionFraction:
        _queryColor[0] = tenGageTraceDiffusionFraction;
        break;
      case colorTenQuantityFADiffusionFraction:
        _queryColor[0] = tenGageFADiffusionFraction;
        break;
      case colorTenQuantityOmegaDiffusionFraction:
        _queryColor[0] = tenGageOmegaDiffusionFraction;
        break;
      case colorTenQuantityConfDiffusionFraction:
        _queryColor[0] = tenGageConfDiffusionFraction;
        break;
      }
      break;
    case colorTenQuantityTrGradEvec0:
      _queryColor.resize(2);
      _queryColor[0] = tenGageTraceDiffusionAlign;
      _cmap->min2D0(0);
      _cmap->max2D0(1);
      _queryColor[1] = tenGageTraceGradMag;
      _cmap->min2D1(0);
      _cmap->max2D1(0.001); // HEY: hack!
      strcat(fname, "cmap/isobow4-2D.nrrd");
      lret = nrrdLoad(_nlut2D, fname, NULL);
      _cmap->lut2D(_nlut2D);
      break;
    case colorTenQuantityFAGradEvec0:
      _queryColor.resize(2);
      _queryColor[0] = tenGageFADiffusionAlign;
      _cmap->min2D0(0);
      _cmap->max2D0(1);
      _queryColor[1] = tenGageFAGradMag;
      _cmap->min2D1(0);
      _cmap->max2D1(0.33); // HEY: hack!
      strcat(fname, "cmap/isobow4-2D.nrrd");
      lret = nrrdLoad(_nlut2D, fname, NULL);
      _cmap->lut2D(_nlut2D);
      break;
    case colorTenQuantityCurving:
    case colorTenQuantityDispersion:
      _queryColor.resize(1);
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0.0);
      _cmap->max1D(0.1); // HEY: hack!
      switch(quantity) {
      case colorTenQuantityCurving:
        _queryColor[0] = tenGageFiberCurving;
        break;
      case colorTenQuantityDispersion:
        _queryColor[0] = tenGageFiberDispersion;
        break;
      }
      break;
    default:
      fprintf(stderr, "%s: unknown colorTenQuantity %d ??? \n", me, quantity);
      exit(1);
      break;
    }
  } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, _gage->volume()->kind()->name)) {
    switch(quantity) {
    case colorDwiQuantityB0:
      _queryColor.resize(1);
      _queryColor[0] = tenDwiGageB0;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(2500); // HEY: hack!
      break;
    case colorDwiQuantityFA:
      _queryColor.resize(1);
      _queryColor[0] = tenDwiGageFA;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      break;
    case colorDwiQuantityMeanDwiValue:
      _queryColor.resize(1);
      _queryColor[0] = tenDwiGageMeanDWIValue;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(200); // HEY: hack!
      break;
    case colorDwiQuantity1TensorError:
      _queryColor.resize(1);
      _queryColor[0] = tenDwiGageTensorError;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(75); // HEY: hack!
      break;
    case colorDwiQuantity1TensorErrorLog:
      _queryColor.resize(1);
      _queryColor[0] = tenDwiGageTensorErrorLog;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(0.5); // HEY: hack!
      break;
    case colorDwiQuantityRgbLinear:
      _queryColor.resize(1);
      _queryColor[0] = tenDwiGageTensor;
      _cmap->evecRgbWhich(0);
      _cmap->evecRgbAniso(tenAniso_Cl2);
      break;
    default:
      fprintf(stderr, "%s: unknown colorDwiQuantity %d ??? \n", me, quantity);
      exit(1);
      break;
    }
    /*
    fprintf(stderr, "%s: %s %s=%d --> %s %s=%d\n", me, 
            colorDwiQuantity->name, 
            airEnumStr(colorDwiQuantity, quantity),
            quantity,
            _gage->volume()->kind()->enm->name,
            airEnumStr(_gage->volume()->kind()->enm, _queryColor[0]),
            _queryColor[0]);
    */
  } else if (gageKindScl == _gage->volume()->kind()) {
    switch(quantity) {
    case colorSclQuantityValue:
      _queryColor.resize(1);
      _queryColor[0] = gageSclValue;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      fprintf(stderr, "%s: nrrdLoad(%p, %s) = %d\n",
              me, _nrmap1D, fname, lret);
      _cmap->rmap1D(_nrmap1D);
      // HEY: HACK hack!
      /* this value required to make the lung images for ssp paper */
      // _cmap->max1D(65535);  /
      _cmap->min1D(0);
      _cmap->max1D(1);
      _cmap->max1D(65535);
      if (AIR_EXISTS(colorSclQuantityValueMinMax[0]) &&
          AIR_EXISTS(colorSclQuantityValueMinMax[1])) {
        _cmap->min1D(colorSclQuantityValueMinMax[0]);
        _cmap->max1D(colorSclQuantityValueMinMax[1]);
      }
      break;
    case colorSclQuantityGradMag:
      _queryColor.resize(1);
      _queryColor[0] = gageSclGradMag;
      strcat(fname, "cmap/gray.nrrd");
      lret = nrrdLoad(_nrmap1D, fname, NULL);
      _cmap->rmap1D(_nrmap1D);
      _cmap->min1D(0);
      _cmap->max1D(1);
      break;
    case colorSclQuantityHessEvalMode:
      _queryColor.resize(1);
      _queryColor[0] = gageSclHessMode;
      strcat(fname, "cmap/isobow4-1D.nrrd");
      lret = nrrdLoad(_nlut1D, fname, NULL);
      _cmap->lut1D(_nlut1D);
      _cmap->min1D(-1);
      _cmap->max1D(1);
      break;
    default:
      fprintf(stderr, "%s: unknown colorSclQuantity %d ??? \n", me, quantity);
      exit(1);
      break;
    }
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
    // fprintf(stderr, "!%s: %g\n", me, br);
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
  // char me[]="PolyProbe::alphaMaskQuantity";

  if (_alphaMaskQuantity == quantity) {
    return;
  }
  // else
  if (tenGageKind == _gage->volume()->kind()) {
    switch(quantity) {
    case alphaMaskTenQuantityConfidence:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = tenGageConfidence;
      break;
    case alphaMaskTenQuantityFA:
      _queryAlphaMask.resize(2);
      _queryAlphaMask[0] = tenGageConfidence;
      _queryAlphaMask[1] = tenGageFA;
      break;
    case alphaMaskTenQuantityCl:
      _queryAlphaMask.resize(2);
      _queryAlphaMask[0] = tenGageConfidence;
      _queryAlphaMask[1] = tenGageCl2;
      break;
    case alphaMaskTenQuantityCp:
      _queryAlphaMask.resize(2);
      _queryAlphaMask[0] = tenGageConfidence;
      _queryAlphaMask[1] = tenGageCp2;
      break;
    case alphaMaskTenQuantityCa:
      _queryAlphaMask.resize(2);
      _queryAlphaMask[0] = tenGageConfidence;
      _queryAlphaMask[1] = tenGageCa2;
      break;
    case alphaMaskTenQuantityFARidgeSurfaceStrength:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = tenGageFARidgeSurfaceStrength;
      break;
    case alphaMaskTenQuantityFAValleySurfaceStrength:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = tenGageFAValleySurfaceStrength;
      break;
    }
  } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, _gage->volume()->kind()->name)) {
    switch(quantity) {
    case alphaMaskDwiQuantityB0:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = tenDwiGageB0;
      break;
    case alphaMaskDwiQuantityMeanDwiValue:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = tenDwiGageMeanDWIValue;
      break;
    case alphaMaskDwiQuantityConfidence:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = tenDwiGageConfidence;
      break;
    }
  } else if (gageKindScl == _gage->volume()->kind()) {
    switch(quantity) {
    case alphaMaskSclQuantityValue:
      _queryAlphaMask.resize(1);
      _queryAlphaMask[0] = gageSclValue;
      break;
    }
  }
  _alphaMaskQuantity = quantity;
  _flag[flagQueryAlphaMask] = true;
}

void
PolyProbe::alphaMaskThreshold(double thresh) {
  // char me[]="PolyProbe::alphaMaskThreshold";

  // fprintf(stderr, "!%s: hi\n", me);
  if (_alphaMaskThreshold != thresh) {
    // fprintf(stderr, "!%s: %g -> %g\n", me, _alphaMaskThreshold, thresh);
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
  char me[]="PolyProbe::update";
  bool ret = false;

  // well this is weird- the polyprobe class has to be told when its
  // own polydata has changed?  Shouldn't it have a way of knowing?
  // Fri Aug 17 09:14:43 EDT 2007: now its "|=" instead of "="
  _flag[flagGeometry] |= geometryChanged;
  /*
  fprintf(stderr, "!%s(%p): _flag[QueryColor,QueryAlphaMask] = %s,%s\n",
          me, this, _flag[flagQueryColor] ? "true" : "false",
          _flag[flagQueryAlphaMask] ? "true" : "false");
  fprintf(stderr, "!%s: _colorDoit = %s\n", 
          me, _colorDoit ? "true" : "false");
  */
  // fprintf(stderr, "!%s: not checking flagVolume?\n", me);
  if (_flag[flagQueryColor]
      || _flag[flagQueryNoColor]
      || _flag[flagQueryAlphaMask]) {
    if (_colorDoit) {
      _query[0] = _queryColor;
    } else {
      _query[0] = _queryNoColor;
    }
    if (_alphaMaskDoit) {
      _query[1] = _queryAlphaMask;
    } else {
      _query[1].clear();
    }
    _gage->query(_query);
    _flag[flagQueryColor] = false;
    _flag[flagQueryNoColor] = false;
    _flag[flagQueryAlphaMask] = false;
    _flag[flagQuery] = true;
  }
  /*
  fprintf(stderr, "!%s: _flag[flagQuery], _flag[flagGageKernels] = %s %s\n",me,
          _flag[flagQuery] ? "true" : "false",
          _flag[flagGageKernels] ? "true" : "false");
  */
  if (_flag[flagQuery]
      || _flag[flagGageKernels]) {

    fprintf(stderr, "!%s: _gage->update(): %u %u %u\n", me, 
            (unsigned int)(_gage->query().size()),
            (unsigned int)(_gage->query()[0].size()),
            (unsigned int)(_gage->query()[1].size()));

    if (_gage->querySet()) { // NOT ->query().size()
      _gage->update();
    }
    _flag[flagQuery] = false;
    _flag[flagGageKernels] = false;
    _flag[flagGageContext] = true;
  }
  /*
  fprintf(stderr, "!%s(%s,%s): visible = %s, _queryNoColor.size() = %u\n", me,
          _flag[flagGeometry] ? "true" : "false",
          _flag[flagGageContext] ? "true" : "false",
          this->visible() ? "true" : "false",
          (unsigned int)(_queryNoColor.size()));
  */
  if (_flag[flagGeometry]
      || _flag[flagGageContext]) {
    if (_gage->querySet() 
        && this->polyData()->xyzwNum) { // NOT ->query().size()
      probe(_gage);
    }
    _flag[flagGeometry] = false;
    _flag[flagGageContext] = false;
    _flag[flagProbedValues] = true;
  }
  /*
  fprintf(stderr, "!%s(%s,%s): _colorDoit = %s, _gage->query().size() = %u\n",
          me,
          _flag[flagProbedValues] ? "true" : "false",
          _flag[flagColorMap] ? "true" : "false",
          _colorDoit ? "true" : "false",
          (unsigned int)(_gage->query().size()));
  */
  if (_flag[flagProbedValues]
      || _flag[flagColorMap]) {
    // NOT ->query().size(), but for reasons GLK can't remember
    if (_gage->querySet()
        && this->polyData()->xyzwNum
        && _colorDoit) { 
      /*
      fprintf(stderr, "!%s: lpld->rgba = %p\n", me, 
              dynamic_cast<PolyData*>(this)->lpld()->rgba);
      */
      dynamic_cast<PolyData*>(this)->color(0, _cmap);
    }
    _flag[flagProbedValues] = false;
    _flag[flagColorMap] = false;
    _flag[flagInitialRGBA] = true;
  }
  /*
  fprintf(stderr, "!%s: _flag[flagBrightness] = %s; %s; %u\n", me, 
          _flag[flagBrightness] ? "true" : "false",
          _flag[flagInitialRGBA] ? "true" : "false",
          (unsigned int)(_gage->query().size()));
  */
  if (_flag[flagBrightness]
      || _flag[flagInitialRGBA]) {
    _brightnessLutSet(_brightness);
    if (_colorDoit && this->polyData()->xyzwNum) {
      dynamic_cast<PolyData*>(this)->RGBLut(_brightnessLut);
    } else {
      // this was the solution to the uncolored fibers being a random
      // color: I had forgotten that the RGBLut was what actually set the
      // vertex RGB from the "initial" (colormap output) RGB.  Lacking
      // a colormap, no assignment was done, and they were never otherwise
      // set, so the vertex RGB were bogus
      dynamic_cast<PolyData*>(this)->RGBSet(255, 255, 255);
    }
    _flag[flagBrightness] = false;
  }
  if (_flag[flagInitialRGBA]
      || _flag[flagAlphaMaskThreshold]) {
    if (_alphaMaskDoit
	&& _gage->querySet()
	&& this->polyData()->xyzwNum) { // NOT ->query().size()
      dynamic_cast<PolyData*>(this)->alphaMask(1, _alphaMaskThreshold);
    }
    _flag[flagInitialRGBA] = false;
    _flag[flagAlphaMaskThreshold] = false;
  }
  return ret;
}

} /* namespace Deft */
