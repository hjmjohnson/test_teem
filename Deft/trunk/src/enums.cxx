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

#include "Deft.H"

namespace Deft {

/* ------------------------------------------------------------- */

/*
** this was basically a cut-and-paste from fltk/events.h from
** fltk version 2.0 ...
*/
char
_fltkEventStr[fltk::TOOLTIP+1][AIR_STRLEN_SMALL] = {
  "NO_EVENT",
  "PUSH",
  "RELEASE",
  "ENTER",
  "LEAVE",
  "DRAG",
  "FOCUS",
  "UNFOCUS",
  "KEY",
  "KEYUP",
  "FOCUS_CHANGE",
  "MOVE",
  "SHORTCUT",
  "DEACTIVATE",
  "ACTIVATE",
  "HIDE",
  "SHOW",
  "PASTE",
  "TIMEOUT",
  "MOUSEWHEEL",
  "DND_ENTER",
  "DND_DRAG",
  "DND_LEAVE",
  "DND_RELEASE",
  "TOOLTIP"
};

airEnum
_fltkEvent = {
  "fltk event",
  fltk::TOOLTIP,   // see Deft::sanity()
  _fltkEventStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
fltkEvent = &_fltkEvent;

/* ------------------------------------------------------------- */

char
_viewerModeStr[DEFT_VIEWER_MODE_MAX+1][AIR_STRLEN_SMALL] = {
  "unknown mode",
  "fov",
  "depth",
  "rotate-UV",
  "rotate-U",
  "rotate-V",
  "rotate-N",
  "dolly",
  "translate-UV",
  "translate-U",
  "translate-V",
  "translate-N"
};

airEnum
_viewerMode = {
  "viewer mode",
  DEFT_VIEWER_MODE_MAX,
  _viewerModeStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
viewerMode = &_viewerMode;

/* ------------------------------------------------------------- */

char
_colorQuantityStr[DEFT_COLOR_QUANTITY_MAX+1][AIR_STRLEN_SMALL] = {
  "unknown query",
  "RGB(linear)",
  "RGB(planar)",
  "(Mode,FA)",
  "Mode",
  "FA",
  "(Cl,Cp)",
  "Cl",
  "Cp",
  "Ca"
};

airEnum
_colorQuantity = {
  "tensor colormap query",
  DEFT_COLOR_QUANTITY_MAX,
  _colorQuantityStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
colorQuantity = &_colorQuantity;

/* ------------------------------------------------------------- */

char
_alphaMaskQuantityStr[DEFT_ALPHA_MASK_QUANTITY_MAX+1][AIR_STRLEN_SMALL] = {
  "unknown query",
  "conf",
  "FA",
  "Cl",
  "Cp",
  "Ca"
};

airEnum
_alphaMaskQuantity = {
  "tensor alpha mask query",
  DEFT_ALPHA_MASK_QUANTITY_MAX,
  _alphaMaskQuantityStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
alphaMaskQuantity = &_alphaMaskQuantity;

}
