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

#ifndef DEFT_HYPER_STREAMLINE_UI_INCLUDED
#define DEFT_HYPER_STREAMLINE_UI_INCLUDED

#include "Deft.h"
#include "Viewer.h"
#include "Slider.h"
#include "TensorGlyph.h"
#include "TriPlane.h"
#include "HyperStreamline.h"

#include <fltk/Choice.h>
#include <fltk/CheckButton.h>
#include <fltk/ValueInput.h>
#include <fltk/ValueSlider.h>
// #include <fltk/Output.h>

namespace Deft {

class DEFT_EXPORT HyperStreamlineUI {
public: 
  // TERRIBLE: it is a total hack to be taking pointers to TensorGlyph
  // and TriPlane here; HyperStreamline and TensorGlyph really need to
  // be peers, with some kind of bi-directional communication mediated
  // by a higher authority
  explicit HyperStreamlineUI(HyperStreamline *hs, TensorGlyph *tglyph,
                             TriPlane *tplane, Viewer *vw);
  ~HyperStreamlineUI();
  void show();
  void redraw();

private:
  // we do not own any of these
  HyperStreamline *_hsline;
  TensorGlyph *_tglyph;
  TriPlane *_tplane;
  Viewer *_viewer;

  NrrdKernelSpec *_ksp; // owned
  
  // TERRIBLE: part of same awful hack described above;
  Nrrd *_nseeds;

  fltk::Window *_win;  

  fltk::CheckButton *_visibleButton;
  static void visible_cb(fltk::CheckButton *but, HyperStreamlineUI *ui);
  fltk::CheckButton *_wireframeButton;
  static void wireframe_cb(fltk::CheckButton *but, HyperStreamlineUI *ui);
  fltk::CheckButton *_colorButton;
  static void color_cb(fltk::CheckButton *but, HyperStreamlineUI *ui);
  fltk::Choice *_colorQuantityMenu;
  static void colorQuantity_cb(fltk::Choice *menu, HyperStreamlineUI *ui);
  fltk::Button *_computeButton;
  static void compute_cb(fltk::Button *but, HyperStreamlineUI *ui);
  fltk::Button *_tubeDoButton;
  static void tubeDo_cb(fltk::CheckButton *but, HyperStreamlineUI *ui);
  fltk::CheckButton *_stopColorDoButton;
  static void stopColorDo_cb(fltk::CheckButton *but, HyperStreamlineUI *ui);
  fltk::ValueInput *_tubeFacetInput;
  static void tubeFacet_cb(fltk::ValueInput *val, HyperStreamlineUI *ui);
  Deft::Slider *_tubeRadiusSlider;
  static void tubeRadius_cb(Deft::Slider *val, HyperStreamlineUI *ui);

  Slider *_stepSlider;
  static void step_cb(Slider *slider, HyperStreamlineUI *ui);

  fltk::CheckButton *_stopAnisoButton;
  static void _stopButton_cb(fltk::CheckButton *but, HyperStreamlineUI *ui);
  fltk::Choice *_stopAnisoTypeMenu;
  static void _stopAnisoType_cb(fltk::Choice *menu, HyperStreamlineUI *ui);
  Deft::Slider *_stopAnisoThresholdSlider;
  static void _stopSlider_cb(Deft::Slider *val, HyperStreamlineUI *ui);

  fltk::CheckButton *_stopHalfLengthButton;
  Deft::Slider *_stopHalfLengthSlider;

  fltk::CheckButton *_stopHalfStepNumButton;
  Deft::Slider *_stopHalfStepNumSlider;

  fltk::CheckButton *_stopConfidenceButton;
  Deft::Slider *_stopConfidenceSlider;
  
  fltk::CheckButton *_stopRadiusButton;
  Deft::Slider *_stopRadiusSlider;
  
  fltk::Choice *_kernelMenu;
  static void kernel_cb(fltk::Choice *menu, HyperStreamlineUI *ui);

  fltk::Choice *_integrationMenu;
  static void integration_cb(fltk::Choice *menu, HyperStreamlineUI *ui);

  fltk::ValueSlider *_brightSlider;
  static void bright_cb(fltk::ValueSlider *val, HyperStreamlineUI *ui);
};

} /* namespace Deft */

#endif /* DEFT_HYPER_STREAMLINE_UI_INCLUDED */
