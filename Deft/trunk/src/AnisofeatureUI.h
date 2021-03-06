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

#ifndef DEFT_ANISOFEATURE_UI_INCLUDED
#define DEFT_ANISOFEATURE_UI_INCLUDED

#include "Deft.h"
#include "Viewer.h"
#include "Anisofeature.h"
#include "Slider.h"

#include <fltk/CheckButton.h>
#include <fltk/Choice.h>
#include <fltk/ValueSlider.h>

namespace Deft {

class DEFT_EXPORT AnisofeatureUI {
public:
  explicit AnisofeatureUI(Anisofeature *ani, Viewer *vw);
  ~AnisofeatureUI();
  void show();
  void redraw();
private:
  Anisofeature *_aniso;
  Viewer *_viewer;
  NrrdKernelSpec *_ksp;

  fltk::Window *_win;  

  fltk::CheckButton *_visibleButton;
  static void visible_cb(fltk::CheckButton *but, AnisofeatureUI *ui);

  fltk::CheckButton *_wireframeButton;
  static void wireframe_cb(fltk::CheckButton *but, AnisofeatureUI *ui);

  fltk::CheckButton *_colorButton;
  static void color_cb(fltk::CheckButton *but, AnisofeatureUI *ui);

  fltk::Choice *_colorQuantityMenu;
  static void colorQuantity_cb(fltk::Choice *menu, AnisofeatureUI *ui);

  fltk::Choice *_kernelMenu;
  static void kernel_cb(fltk::Choice *menu, AnisofeatureUI *ui);

  fltk::ValueSlider *_sampleSlider;
  static void sample_cb(fltk::ValueSlider *val, AnisofeatureUI *ui);

  fltk::CheckButton *_glyphsDoButton;
  static void glyphsDo_cb(fltk::CheckButton *but, AnisofeatureUI *ui);

  fltk::CheckButton *_tractsDoButton;
  static void tractsDo_cb(fltk::CheckButton *but, AnisofeatureUI *ui);

  /*
  fltk::Choice *_anisoTypeMenu;
  static void anisoType_cb(fltk::Choice *menu, AnisofeatureUI *ui);
  */

  Slider *_strengthSlider;
  static void strength_cb(Slider *slider, AnisofeatureUI *ui);

  fltk::ValueSlider *_brightSlider;
  static void bright_cb(fltk::ValueSlider *val, AnisofeatureUI *ui);

  fltk::Button *_updateButton;
  static void updateButton_cb(fltk::Button *but, AnisofeatureUI *ui);

  fltk::Choice *_alphaMaskQuantityMenu;
  static void alphaMaskQuantity_cb(fltk::Choice *menu, AnisofeatureUI *ui);
  Slider *_alphaMaskThresholdSlider;
  static void alphaMaskThreshold_cb(Slider *val, AnisofeatureUI *ui);

  fltk::CheckButton *_ccDoButton;
  static void ccDo_cb(fltk::CheckButton *but, AnisofeatureUI *ui);
  fltk::CheckButton *_ccSingleButton;
  static void ccSingle_cb(fltk::CheckButton *but, AnisofeatureUI *ui);
  Slider *_ccIdSlider;
  static void ccId_cb(Slider *val, AnisofeatureUI *ui);

  bool _once;
};

}

#endif /* DEFT_ANISOFEATURE_UI_INCLUDED */


