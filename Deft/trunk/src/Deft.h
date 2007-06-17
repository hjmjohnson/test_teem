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

#ifndef DEFT_INCLUDED
#define DEFT_INCLUDED

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>
#include <teem/nrrd.h>
#include <teem/ell.h>
#include <teem/limn.h>
#include <teem/seek.h>
#include <teem/ten.h>

#include <GL/glew.h>  /* includes glu.h */

// not needed? #include <cstdio>
// not needed? #include <iostream>

#include <fltk/events.h>
#include <fltk/gl.h>
#include <fltk/Window.h>
#include <fltk/GlWindow.h>
#include <fltk/Box.h>
#include <fltk/Input.h>
#include <fltk/Slider.h>
#include <fltk/Button.h>
#include <fltk/CheckButton.h>
#include <fltk/Group.h>
#include <fltk/Menu.h>
#include <fltk/run.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(DEFT_STATIC)
#  if defined(DEFT_BUILD)
#    define DEFT_EXPORT extern __declspec(dllexport)
#    define DEFT_API    extern __declspec(dllexport)
#  else
#    define DEFT_EXPORT extern __declspec(dllimport)
#    define DEFT_API    extern __declspec(dllimport)
#  endif
#else /* DEFT_STATIC || UNIX */
#  define DEFT_EXPORT
#  define DEFT_API extern
#endif

namespace Deft {

extern char homeDir[AIR_STRLEN_MED];   // TERRIBLE

/*
** viewerMode* enum
**
** the GUI modes that a DeftViewer can be in.  In Fov and Depth
** (distance from near to far adjusted), the eye and look-at point are
** both fixed.  The eye moves around a fixed look-at point in the
** Rotate and Dolly modes.  The eye and look-at points move together
** in the Translate modes.
*/
enum {
  viewerModeUnknown,     /*  0 */
  viewerModeFov,         /*  1: standard "zoom" */
  viewerModeDepth,       /*  2: adjust distance between near and far planes */
  viewerModeRotateUV,    /*  3: usual rotate (around at point) */
  viewerModeRotateU,     /*  4: rotate around horizontal axis */
  viewerModeRotateV,     /*  5: rotate around vertical axis */
  viewerModeRotateN,     /*  6: in-plane rotate (around at point) */
  viewerModeDolly,       /*  7: fix at, move from, adjust fov: the effect is
                                 direct control on the amount of perspective */
  viewerModeTranslateUV, /*  8: usual translate */
  viewerModeTranslateU,  /*  9: translate only horizontal */
  viewerModeTranslateV,  /* 10: translate only vertical */
  viewerModeTranslateN,  /* 11: translate from *and* at along view direction */
  viewerModeLast
};
#define DEFT_VIEWER_MODE_MAX  11

/*
** maximum possible number of flags needed for internal state updates
*/
#define DEFT_FLAG_NUM_MAX 128

/*
** colorTenQuantity* enum
**
** the quantities that one can expect to see colormapped after having
** probed them in a (diffusion) tensor field.
**
** This is really a weird kind of information, in terms of its
** semantics and generality- both colorQuantityRgbLinear and
** colorQuantityRgbPlanar ultimately set the underlying Deft::Gage to
** query tenGageTensor, but differ in how the Deft::Cmap->_rgbp
** treats that information.  On the other hand, colorQuantityCl and
** colorQuantityCp set queries of tenGageCl2 and tenGageCp2, but both
** use the same (user-supplied) colormap.  So this really conveys
** something about a combination of a query and a coloring.
*/
enum {
  colorTenQuantityUnknown,                /*  0: */

  colorTenQuantityConf,                   /*  1: */

  colorTenQuantityRgbLinear,              /*  2: */
  colorTenQuantityRgbPlanar,              /*  3: */
  colorTenQuantityModeFA,                 /*  4: */
  colorTenQuantityMode,                   /*  5: */
  colorTenQuantityFA,                     /*  6: */
  colorTenQuantityTrace,                  /*  7: */
  colorTenQuantityClCp,                   /*  8: */
  colorTenQuantityCl,                     /*  9: */
  colorTenQuantityCp,                     /* 10: */
  colorTenQuantityCa,                     /* 11: */

  colorTenQuantityTrGradVecDotEvec0,      /* 12: */
  colorTenQuantityTrGradEvec0,            /* 13: */
  colorTenQuantityTrDiffusionAngle,       /* 14: */
  colorTenQuantityTrDiffusionFraction,    /* 15: */

  colorTenQuantityFAGradVecDotEvec0,      /* 16: */
  colorTenQuantityFAGradEvec0,            /* 17: */
  colorTenQuantityFADiffusionAngle,       /* 18: */
  colorTenQuantityFADiffusionFraction,    /* 19: */

  colorTenQuantityOmegaGradVecDotEvec0,   /* 20: */
  colorTenQuantityOmegaNormalDotEvec0,    /* 21: */
  colorTenQuantityOmegaDiffusionAngle,    /* 22: */
  colorTenQuantityOmegaDiffusionFraction, /* 23: */

  colorTenQuantityLast
};
#define DEFT_COLOR_TEN_QUANTITY_MAX 23

/*
** the weirdness here is that while "confidence" is itself
** a probe-able scalar, all the other quantities really
** signify the product of confidence and the measure
*/
enum {
  alphaMaskTenQuantityUnknown,          /* 0 */
  alphaMaskTenQuantityConfidence,       /* 1 */
  alphaMaskTenQuantityFA,               /* 2 */
  alphaMaskTenQuantityCl,               /* 3 */
  alphaMaskTenQuantityCp,               /* 4 */
  alphaMaskTenQuantityCa,               /* 5 */
  alphaMaskTenQuantityFARidgeSurfaceStrength,  /* 6 */
  alphaMaskTenQuantityFAValleySurfaceStrength, /* 7 */
  alphaMaskTenQuantityLast
};
#define DEFT_ALPHA_MASK_TEN_QUANTITY_MAX    7

enum {
  colorDwiQuantityUnknown,          /* 0 */
  colorDwiQuantityB0,               /* 1 */
  colorDwiQuantityMeanDwiValue,     /* 2 */
  colorDwiQuantityRgbLinear,        /* 3 */
  colorDwiQuantity1TensorError,     /* 4 */
  colorDwiQuantity1TensorErrorLog,  /* 5 */
  colorDwiQuantityLast
};
#define DEFT_COLOR_DWI_QUANTITY_MAX    5

enum {
  alphaMaskDwiQuantityUnknown,          /* 0 */
  alphaMaskDwiQuantityB0,               /* 1 */
  alphaMaskDwiQuantityMeanDwiValue,     /* 2 */
  alphaMaskDwiQuantityLast
};
#define DEFT_ALPHA_MASK_DWI_QUANTITY_MAX    2

/* enumsDeft.C */
DEFT_API airEnum *fltkEvent;
DEFT_API airEnum *viewerMode;
DEFT_API airEnum *colorTenQuantity;
DEFT_API airEnum *alphaMaskTenQuantity;
DEFT_API airEnum *colorDwiQuantity;
DEFT_API airEnum *alphaMaskDwiQuantity;

/* miscDeft.C */
DEFT_API void sanity();

} /* namespace Deft */

#endif /* DEFT_INCLUDED */
