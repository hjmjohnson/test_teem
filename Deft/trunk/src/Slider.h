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

#ifndef DEFT_SLIDER_INCLUDED
#define DEFT_SLIDER_INCLUDED

#include "Deft.h"
#include <fltk/Box.h>

namespace Deft {

class DEFT_EXPORT Slider : public fltk::Group {
public:
  explicit Slider(int x, int y, int w, int h, const char *label=0);
  void range(double min, double max);
  double maximum();
  void maximum(double);
  double minimum();
  void minimum(double);
  double value();
  void value(double);
  unsigned int valueUI();
  void valueUI(unsigned int);
  void step(double);
  int fastUpdate();
  void fastUpdate(int);
  void callback(fltk::Callback *, void*);
  void color(fltk::Color);
  fltk::Callback *callback();
private:
  fltk::Callback *_callback;
  void *_data;
  static void range_cb(fltk::Widget *w, void *data);
  static void value_cb(fltk::Widget *w, void *data);
  static void input_cb(fltk::Widget *w, void *data);
  static void slider_cb(fltk::Widget *w, void *data);
  void input__cb();
  void slider__cb();
  fltk::Group *_label;
  fltk::Input *_input, *_min, *_max;
  fltk::CheckButton *_fastUp;
  fltk::Slider *_slider;
};

}

#endif /* DEFT_SLIDER_INCLUDED */
