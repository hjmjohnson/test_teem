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

#ifndef DEFT_SEEDPOINT_UI_INCLUDED
#define DEFT_SEEDPOINT_UI_INCLUDED

#include "Deft.h"
#include "Viewer.h"
#include "Slider.h"
#include "SeedPoint.h"

#include <fltk/Group.h>
#include <fltk/Choice.h>
#include <fltk/CheckButton.h>
#include <fltk/ValueInput.h>
#include <fltk/ValueSlider.h>
#include <fltk/Output.h>

namespace Deft {

class DEFT_EXPORT SeedPointUI {
public: 
  explicit SeedPointUI(SeedPoint *sp, Viewer *vw);
  ~SeedPointUI();
  void show();
  void redraw();
private:
  NrrdKernelSpec *_ksp;  // owned
  SeedPoint *_seedpoint; // not owned
  Viewer *_viewer;       // not owned

  fltk::Window *_win;  

  fltk::CheckButton *_cursorShowButton;
  static void cursorShow_cb(fltk::CheckButton *but, SeedPointUI *ui);

  fltk::CheckButton *_starGlyphDoButton;
  static void starGlyphDo_cb(fltk::CheckButton *but, SeedPointUI *ui);

  fltk::CheckButton *_ten1GlyphDoButton;
  static void ten1GlyphDo_cb(fltk::CheckButton *but, SeedPointUI *ui);

  fltk::CheckButton *_ten2GlyphDoButton;
  static void ten2GlyphDo_cb(fltk::CheckButton *but, SeedPointUI *ui);

  fltk::CheckButton *_fiberDoButton;
  static void fiberDo_cb(fltk::CheckButton *but, SeedPointUI *ui);

  Slider *_scaleSlider;
  static void scale_cb(Slider *slider, SeedPointUI *ui);

  Slider *_positionSlider[3];
  static void position_cb(Slider *slider, SeedPointUI *ui);

  fltk::Choice *_fiberTypeMenu;
  static void fiberType_cb(fltk::Choice *menu, SeedPointUI *ui);
  
};

} /* namespace Deft */

#endif /* DEFT_SEEDPOINT_UI_INCLUDED */
