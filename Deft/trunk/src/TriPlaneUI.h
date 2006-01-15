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

#ifndef DEFT_TRI_PLANE_UI_INCLUDED
#define DEFT_TRI_PLANE_UI_INCLUDED

#include "Deft.h"
#include "Viewer.h"
#include "Slider.h"
#include "TriPlane.h"

#include <fltk/Group.h>
#include <fltk/Choice.h>
#include <fltk/CheckButton.h>
#include <fltk/ValueInput.h>
#include <fltk/ValueSlider.h>
#include <fltk/Output.h>

namespace Deft {

class DEFT_EXPORT TriPlaneUI {
public: 
  explicit TriPlaneUI(TriPlane *tp, Viewer *vw);
  ~TriPlaneUI();
  void show();
  void redraw();
private:
  NrrdKernelSpec *_ksp; // owned
  TriPlane *_triplane;  // not owned
  Viewer *_viewer;      // not owned

  fltk::Window *_win;  

  fltk::ValueInput *_samplingInput;
  static void sampling_cb(fltk::ValueInput *val, TriPlaneUI *ui);

  fltk::CheckButton *_visibleButton[4];
  static void visible_cb(fltk::CheckButton *but, TriPlaneUI *ui);

  fltk::CheckButton *_wireframeButton[4];
  static void wireframe_cb(fltk::CheckButton *but, TriPlaneUI *ui);

  fltk::Choice *_colorQuantityMenu[4];
  static void colorQuantity_cb(fltk::Choice *menu, TriPlaneUI *ui);

  fltk::ValueSlider *_sampleSlider[4];
  static void sample_cb(fltk::ValueSlider *val, TriPlaneUI *ui);

  fltk::ValueSlider *_seedSampleSlider[4];
  static void seedSample_cb(fltk::ValueSlider *val, TriPlaneUI *ui);

  fltk::CheckButton *_glyphsDoButton[4];
  static void glyphsDo_cb(fltk::CheckButton *but, TriPlaneUI *ui);

  fltk::CheckButton *_tractsDoButton[4];
  static void tractsDo_cb(fltk::CheckButton *but, TriPlaneUI *ui);

  Slider *_positionSlider[3];
  static void position_cb(Slider *slider, TriPlaneUI *ui);

  fltk::ValueSlider *_brightSlider;
  static void bright_cb(fltk::ValueSlider *val, TriPlaneUI *ui);

  fltk::Choice *_alphaMaskQuantityMenu;
  static void alphaMaskQuantity_cb(fltk::Choice *menu, TriPlaneUI *ui);
  fltk::ValueSlider *_alphaMaskThresholdSlider;
  static void alphaMaskThreshold_cb(fltk::ValueSlider *val, TriPlaneUI *ui);

  fltk::Choice *_kernelMenu;
  static void kernel_cb(fltk::Choice *menu, TriPlaneUI *ui);

  fltk::Button *_hestButton;
  static void _hest_cb(fltk::Button *but, TriPlaneUI *ui);

  // fltk::Button *_maskButton;
  // static void _mask_cb(fltk::Button *but, TriPlaneUI *ui);

};

} /* namespace Deft */

#endif /* DEFT_TRI_PLANE_UI_INCLUDED */
