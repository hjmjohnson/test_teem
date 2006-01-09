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

#include "Deft.h"
#include "Slider.h"

namespace Deft {

void
Slider::range_cb(fltk::Widget *w, void *data) {
  double maxval=0, minval=0;
  Slider *slider = (Slider*)data;

  AIR_UNUSED(w);
  if (1 != sscanf(slider->_min->value(), "%lg", &minval)) {
    minval = slider->minimum();
  }
  if (1 != sscanf(slider->_max->value(), "%lg", &maxval)) {
    maxval = slider->maximum();
  }
  slider->range(minval, maxval);
}

void
Slider::input__cb() {
  double val=0, clampval;
  char buff[AIR_STRLEN_SMALL];
  if (1 != sscanf(this->_input->value(), "%lg", &val)) {
    val = this->_slider->value();
  }
  clampval = AIR_CLAMP(this->minimum(), val, this->maximum());
  if (val != clampval) {
    sprintf(buff, "%g", clampval);
    this->_input->value(buff);
  }
  this->_slider->value(clampval);
  if (this->_callback) {
    // fprintf(stderr, "%s: doing callack\n", "Slider::input__cb");
    this->_callback(this, this->_data);
  }
}

void
Slider::input_cb(fltk::Widget *w, void *data) {

  AIR_UNUSED(w);
  // fprintf(stderr, "%s: doing callack\n", "Slider::input_cb");
  Slider *slider = (Slider*)data;
  slider->input__cb();
}

void
Slider::slider__cb() {
  char buff[AIR_STRLEN_SMALL];
  // Slider *slider = (Slider*)data;
  sprintf(buff, "%g", this->_slider->value());
  this->_input->value(buff);
  if (this->_callback
      && (this->_fastUp->value() ^ (fltk::RELEASE == fltk::event()))) {
    // fprintf(stderr, "%s: doing callack\n", "Slider::slider__cb");
    this->_callback(this, this->_data);
  }
}

void
Slider::slider_cb(fltk::Widget *w, void *data) {

  AIR_UNUSED(w);
  Slider *slider = (Slider*)data;
  slider->slider__cb();
}

Slider::Slider(int x, int y, int w, int h, const char *label) :
fltk::Group(x, y, w, h, 0) {
  // char labelBuff[AIR_STRLEN_SMALL];
  int marg = 0, butt = 20, NN = 6;
  int sub_h = AIR_ROUNDUP((h - 3*marg)/2.0);
  int sub_w = AIR_ROUNDUP((w - 5*marg)/NN);

  // sprintf(labelBuff, "@s+3%s", label);
  _label = new fltk::Group(x, y, 2*sub_w, sub_h, label);
  // _label->copy_label(labelBuff);
  _label->align(fltk::ALIGN_INSIDE);

  _min = new fltk::Input(x + (NN-2)*sub_w + 3*marg - butt, y + marg,
                         sub_w, sub_h);
  // _min->box(fltk::THIN_DOWN_BOX);
  _min->callback(Slider::range_cb, (void*)this);
  _min->when(fltk::WHEN_ENTER_KEY|fltk::WHEN_NOT_CHANGED);
  
  _max = new fltk::Input(x + (NN-1)*sub_w + 4*marg - butt, y + marg,
                         sub_w, sub_h);
  // _max->box(fltk::THIN_DOWN_BOX);
  _max->callback(Slider::range_cb, (void*)this);
  _max->when(fltk::WHEN_ENTER_KEY|fltk::WHEN_NOT_CHANGED);

  _fastUp = new fltk::CheckButton(w - butt, y + marg, butt, sub_h, "");
  _fastUp->when(fltk::WHEN_CHANGED);
  // _fastUp->box(fltk::THIN_DOWN_BOX);
  _fastUp->labeltype(fltk::NO_LABEL);
  _fastUp->value(0);

  _input = new fltk::Input(x + marg, y + h - marg - sub_h,
                           sub_w, sub_h);
  // _input->box(fltk::THIN_DOWN_BOX);
  _input->callback(Slider::input_cb, (void*)this);
  _input->when(fltk::WHEN_ENTER_KEY|fltk::WHEN_NOT_CHANGED);
  
  _slider = new fltk::Slider(x + w - (NN-1)*sub_w - 3*marg,
                             y + h - marg - sub_h,
                             (NN-1)*sub_w + 2*marg, sub_h);
  _slider->callback(Slider::slider_cb, (void*)this);
  _slider->labeltype(fltk::NO_LABEL);
  _slider->when(fltk::WHEN_CHANGED | fltk::WHEN_RELEASE);
  _slider->box(fltk::THIN_DOWN_BOX);
  _slider->color(fltk::GRAY40);
  // _slider->buttoncolor(0xFF008000); the slider knob itself
  // _slider->highlight_color(0xFF008000); the knob during mouseover
  //_slider->highlight_textcolor(0xFF008000); nothing
  // _slider->labelcolor(0xFF008000); nothing
  // _slider->selection_color(0xFF008000); the knob again
  // _slider->selection_textcolor(0xFF008000); nothing
  // _slider->selection_textcolor(0xFF008000);
  
  end(); // close the group

  _callback = 0;
  _data = 0;

  // initialize input
  Slider::slider_cb(_input, (void*)this);
}

void
Slider::range(double minimum, double maximum) {
  char buff[AIR_STRLEN_SMALL];

  double value = _slider->value();
  _slider->range(minimum, maximum);
  sprintf(buff, "%g", _slider->minimum());
  _min->value(buff);
  sprintf(buff, "%g", _slider->maximum());
  _max->value(buff);
  _slider->value(value);
  _slider->redraw();
}

double
Slider::maximum() { 
  return _slider->maximum();
}

double
Slider::minimum() { 
  return _slider->minimum();
}

void
Slider::maximum(double maxval) {
  char buff[AIR_STRLEN_SMALL];

  _slider->maximum(maxval);
  _slider->redraw();
  sprintf(buff, "%g", _slider->maximum());
  _max->value(buff);
}  

void
Slider::minimum(double minval) {
  char buff[AIR_STRLEN_SMALL];
  
  _slider->maximum(minval);
  _slider->redraw();
  sprintf(buff, "%g", _slider->minimum());
  _min->value(buff);
}  

void
Slider::step(double stp) {
  return _slider->step(stp);
}

double
Slider::value() {
  return _slider->value();
}

unsigned int 
Slider::valueUI() {
  return static_cast<unsigned int>(_slider->value());
}

void
Slider::value(double val) {
  char buff[AIR_STRLEN_SMALL];

  val = AIR_CLAMP(_slider->minimum(), val, _slider->maximum());
  _slider->value(val);
  sprintf(buff, "%g", val);
  _input->value(buff);
  if (this->_callback) {
    // fprintf(stderr, "%s: doing callack\n", "Slider::value");
    this->_callback(this, this->_data);
  }
}

void
Slider::valueUI(unsigned int val) {
  value(val);
}

int
Slider::fastUpdate() {
 
  return this->_fastUp->value();
}

void
Slider::fastUpdate(int val) {
  
  this->_fastUp->value(val ? true : false);
}

void
Slider::callback(fltk::Callback *cb, void *data) {
  _callback = cb;
  _data = data;
}

fltk::Callback *
Slider::callback() {
  return _callback;
}

}
