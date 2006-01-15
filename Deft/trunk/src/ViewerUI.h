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

#ifndef DEFT_VIEWER_UI_INCLUDED
#define DEFT_VIEWER_UI_INCLUDED

#include "Deft.h"
#include "Viewer.h"
#include <fltk/FloatInput.h>
#include <fltk/IntInput.h>
#include <fltk/Output.h>

namespace Deft {

class DEFT_EXPORT ViewerUI {
public:
  explicit ViewerUI(Viewer *vw);
  ~ViewerUI();
  void show();
private:
  Viewer *_viewer;

  fltk::Window *_win;  

  fltk::Button *_camResetButton;
  static void camResetButton_cb(fltk::Button *but, ViewerUI *ui);

  fltk::Button *_screenDumpButton;
  static void screenDumpButton_cb(fltk::Button *but, ViewerUI *ui);

  fltk::FloatInput *_fromXInput, *_fromYInput, *_fromZInput,
    *_atXInput, *_atYInput, *_atZInput,
    *_upXInput, *_upYInput, *_upZInput,
    *_fovyInput, *_neerInput, *_faarInput;
  static void cameraInput_cb(fltk::FloatInput *in, ViewerUI *ui);

  fltk::IntInput *_widthInput, *_heightInput;
  static void imageSizeInput_cb(fltk::IntInput *in, ViewerUI *ui);

  fltk::CheckButton *_upFixButton;
  static void upFix_cb(fltk::CheckButton *chbut, ViewerUI *ui);
  fltk::CheckButton *_orthographicButton;
  static void orthographic_cb(fltk::CheckButton *chbut, ViewerUI *ui);

  fltk::Output *_framesPerSecondOutputRender;
  fltk::Output *_framesPerSecondOutputActual;

  static void postDraw_cb(Viewer *viewer, ViewerUI *ui);

  fltk::Button *_hestButton;
  static void _hest_cb(fltk::Button *but, ViewerUI *ui);
};

}

#endif /* DEFT_VIEWER_UI_INCLUDED */
