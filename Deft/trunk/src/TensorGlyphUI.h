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

#ifndef DEFT_TENSOR_GLYPH_UI_INCLUDED
#define DEFT_TENSOR_GLYPH_UI_INCLUDED

#include "Deft.h"
#include "Viewer.h"
#include "Slider.h"
#include "TensorGlyph.h"
#include <fltk/Group.h>
#include <fltk/Choice.h>
#include <fltk/CheckButton.h>
#include <fltk/ValueInput.h>
#include <fltk/Output.h>

namespace Deft {

class DEFT_EXPORT TensorGlyphUI {
public:
  explicit TensorGlyphUI(TensorGlyph *tg, Viewer *vw);
  ~TensorGlyphUI();
  void show();
  void redraw();

private:
  // we do not own these 
  TensorGlyph *_tglyph;
  Viewer *_viewer;

  fltk::Window *_win;  

  fltk::CheckButton *_visibleButton;
  static void visible_cb(fltk::CheckButton *menu, TensorGlyphUI *ui);

  fltk::Choice *_glyphTypeMenu;
  static void glyphType_cb(fltk::Choice *menu, TensorGlyphUI *ui);

  fltk::Choice *_glyphColorMenu;
  static void glyphColor_cb(fltk::Choice *menu, TensorGlyphUI *ui);

  fltk::Choice *_anisoTypeMenu;
  static void anisoType_cb(fltk::Choice *menu, TensorGlyphUI *ui);

  fltk::ValueInput *_glyphResInput;
  static void glyphRes_cb(fltk::ValueInput *val, TensorGlyphUI *ui);

  fltk::ValueInput *_superquadSharpnessInput;
  static void superquadSharpness_cb(fltk::ValueInput *val, TensorGlyphUI *ui);

  Slider *_glyphScaleSlider;
  static void glyphScale_cb(Slider *slider, TensorGlyphUI *ui);

  Slider *_anisoThreshSlider;
  static void anisoThresh_cb(Slider *slider, TensorGlyphUI *ui);

  Slider *_anisoThreshMinSlider;
  static void anisoThreshMin_cb(Slider *slider, TensorGlyphUI *ui);

  fltk::Output *_drawnNumOutput;
  fltk::Output *_polygonsPerGlyphOutput;
  fltk::Output *_KglyphsPerSecondOutput;
  fltk::Output *_MpolygonsPerSecondOutput;
  static void postDraw_cb(TensorGlyph *tglyph, TensorGlyphUI *ui);

  fltk::Button *_hestButton;
  static void _hest_cb(fltk::Button *but, TensorGlyphUI *ui);
};

}

#endif /* DEFT_TENSOR_GLYPH_UI_INCLUDED */
