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

#include "Cmap.h"

namespace Deft {

Cmap::Cmap() {
  _nlut1D = NULL;
  _nrmap1D = NULL;
  _nimap1D = NULL;
  _nlut2D = NULL;

  _range1D = nrrdRangeNew(AIR_NAN, AIR_NAN);
  _range2D0 = nrrdRangeNew(AIR_NAN, AIR_NAN);
  _range2D1 = nrrdRangeNew(AIR_NAN, AIR_NAN);

  _rgbp = tenEvecRGBParmNew();
  _rgbp->typeOut = nrrdTypeUChar;
  _rgbp->genAlpha = AIR_TRUE;
}

Cmap::~Cmap() {
  _rgbp = tenEvecRGBParmNix(_rgbp);
  _range1D = nrrdRangeNix(_range1D);
  _range2D0 = nrrdRangeNix(_range2D0);
  _range2D0 = nrrdRangeNix(_range2D0);
}

void Cmap::mode1D(int m) {
  _mode1D = AIR_CLAMP(cmapMode1DUnknown+1, m, cmapMode1DLast-1);
}
int Cmap::mode1D() const { return _mode1D; }

void Cmap::min1D(double min) { _range1D->min = min; }
void Cmap::max1D(double max) { _range1D->max = max; }
double Cmap::min1D() const { return _range1D->min; }
double Cmap::max1D() const { return _range1D->max; }

void
Cmap::lut1D(const Nrrd *nlut1D) {
  char me[]="Cmap::lut1D";

  if (!(nlut1D
        && 2 == nlut1D->dim
        && 4 == nlut1D->axis[0].size)) {
    fprintf(stderr, "%s: wrong dim/size nrrd for lut1D\n", me);
    return;
  }
  _nlut1D = nlut1D;
  _mode1D = cmapMode1DLut1D;
}
const Nrrd *Cmap::lut1D() const { return _nlut1D; }

void
Cmap::rmap1D(const Nrrd *nrmap1D) {
  char me[]="Cmap::rmap1D";

  if (!(nrmap1D
        && 2 == nrmap1D->dim
        && 4 == nrmap1D->axis[0].size)) {
    fprintf(stderr, "%s: wrong dim/size nrrd for rmap1D\n", me);
    return;
  }
  _nrmap1D = nrmap1D;
  _mode1D = cmapMode1DRmap1D;
}
const Nrrd *Cmap::rmap1D() const { return _nrmap1D; }

void
Cmap::imap1D(const Nrrd *nimap1D) {
  char me[]="Cmap::imap1D";

  if (!(nimap1D
        && 2 == nimap1D->dim
        && 4 == nimap1D->axis[0].size)) {
    fprintf(stderr, "%s: wrong dim/size nrrd for imap1D\n", me);
    return;
  }
  _nimap1D = nimap1D;
  _mode1D = cmapMode1DImap1D;
}
const Nrrd *Cmap::imap1D() const { return _nimap1D; }

void
Cmap::lut2D(const Nrrd *nlut2D) {
  char me[]="Cmap::lut2D";

  if (!(nlut2D
        && 3 == nlut2D->dim
        && 4 == nlut2D->axis[0].size)) {
    fprintf(stderr, "%s: wrong dim/size nrrd for lut2D\n", me);
    return;
  }
  _nlut2D = nlut2D;
}
const Nrrd *Cmap::lut2D() const { return _nlut2D; }

void Cmap::min2D0(double min) { _range2D0->min = min; }
void Cmap::max2D0(double max) { _range2D0->max = max; }
void Cmap::min2D1(double min) { _range2D1->min = min; }
void Cmap::max2D1(double max) { _range2D1->max = max; }
double Cmap::min2D0() const { return _range2D0->min; }
double Cmap::max2D0() const { return _range2D0->max; }
double Cmap::min2D1() const { return _range2D1->min; }
double Cmap::max2D1() const { return _range2D1->max; }

void Cmap::evecRgbWhich(unsigned int which) { 
  _rgbp->which = which; 
}
void Cmap::evecRgbAniso(int aniso) { 
  _rgbp->aniso = aniso; 
}
void Cmap::evecRgbConfThresh(double confThresh) { 
  _rgbp->confThresh = confThresh; 
}
void Cmap::evecRgbAnisoGamma(double gamma) {
  _rgbp->anisoGamma = gamma;
}
void Cmap::evecRgbGamma(double gamma) {
  _rgbp->gamma = gamma;
}
void Cmap::evecRgbBgGray(double bgGray) {
  _rgbp->bgGray = bgGray;
}
void Cmap::evecRgbIsoGray(double isoGray) {
  _rgbp->isoGray = isoGray;
}
void Cmap::evecRgbMaxSat(double maxSat) {
  _rgbp->maxSat = maxSat;
}

void
Cmap::map(Nrrd *nrgba, const Values *val) const {
  char me[]="Cmap::map", *err;
  const char *key=NULL;
  int E=0;
  
  /*
  fprintf(stderr, "!%s: size %u length %u \n", me, 
          AIR_CAST(unsigned int, val->item().size()),
          AIR_CAST(unsigned int, val->length()));
  */
  if (1 == val->item().size()) {
    if (1 == val->length()) {
      // nrrdSave("input.nrrd", val->nrrd(), NULL);
      key = NRRD;
      switch(_mode1D) {
      case cmapMode1DGrayMinMax:
        // HEY
        break;
      case cmapMode1DLut1D:
        E = nrrdApply1DLut(nrgba, val->nrrd(), _range1D,
                           _nlut1D, nrrdTypeUChar, AIR_TRUE);
        break;
      case cmapMode1DRmap1D:
        E = nrrdApply1DRegMap(nrgba, val->nrrd(), _range1D,
                              _nrmap1D, nrrdTypeUChar, AIR_TRUE);
        break;
      case cmapMode1DImap1D:
        E = nrrdApply1DIrregMap(nrgba, val->nrrd(), _range1D,
                                _nimap1D, NULL, nrrdTypeUChar, AIR_TRUE);
        break;
      default:
        fprintf(stderr, "%s: unknown mode1D %d\n", me, _mode1D);
        exit(1);
        break;
      }
    } else if (tenGageKind == val->kind()
               && tenGageTensor == val->item()[0]) {
      key = TEN;
      E = tenEvecRGB(nrgba, val->nrrd(), _rgbp);
    } else if (!strcmp(TEN_DWI_GAGE_KIND_NAME, val->kind()->name)
               && tenDwiGageTensor == val->item()[0]) {
      key = TEN;
      E = tenEvecRGB(nrgba, val->nrrd(), _rgbp);
    } else {
      fprintf(stderr, "%s: unknown non-scalar single item (%s %s = %d)\n", me,
              val->kind()->name, airEnumStr(val->kind()->enm, val->item()[0]),
              val->item()[0]);
      exit(1);
    }
  } else if (2 == val->item().size()) {
    key = NRRD;
    E = nrrdApply2DLut(nrgba, val->nrrd(), 0, _range2D0, _range2D1,
                       _nlut2D, nrrdTypeUChar, AIR_TRUE, AIR_TRUE);
  } else {
    fprintf(stderr, "%s: item().size = " _AIR_SIZE_T_CNV " ???\n",
            me, val->item().size());
    // exit(1);
  }
  if (E) {
    fprintf(stderr, "%s: trouble mapping:\n%s", me, err=biffGetDone(key));
    free(err); exit(1);
  }
}

} /* namespace Deft */
