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

#include "SeedPointUI.h"

namespace Deft {

static char labelBuff[3][128] = {"Axis 0", "Axis 1", "Axis 2"};

SeedPointUI::SeedPointUI(SeedPoint *sp, Viewer *vw) {
  char me[]="SeedPointUI::SeedPointUI";
  const unsigned int W=400, H=365, lineH=20;
  unsigned int winy, incy;
  const char *def;
  const airEnum *enm;
  int itu, itl;
  double pos[3];
  
  _win = new fltk::Window(W, H, "Deft::SeedPoint");
  _win->begin();
  _win->resizable(_win);

  winy = 0;
  winy += 5;
  _seedpoint = sp;
  _viewer = vw;

  // ----------------------------------
  _cursorShowButton = new fltk::CheckButton(0*W/5, winy, W/5, lineH, "Point");
  _cursorShowButton->value(_seedpoint->cursorShow());
  _cursorShowButton->callback((fltk::Callback*)cursorShow_cb, this);

  // ----------------------------------
  _ten1GlyphDoButton = new fltk::CheckButton(1*W/5, winy, W/5, lineH, "Ten1");
  _ten1GlyphDoButton->value(_seedpoint->ten1GlyphDo());
  _ten1GlyphDoButton->callback((fltk::Callback*)ten1GlyphDo_cb, this);

  // ----------------------------------
  _ten2GlyphDoButton = new fltk::CheckButton(2*W/5, winy, W/5, lineH, "Ten2");
  _ten2GlyphDoButton->value(_seedpoint->ten2GlyphDo());
  _ten2GlyphDoButton->callback((fltk::Callback*)ten2GlyphDo_cb, this);

  // ----------------------------------
  _starGlyphDoButton = new fltk::CheckButton(3*W/5, winy, W/5, lineH, "Star");
  _starGlyphDoButton->value(_seedpoint->starGlyphDo());
  _starGlyphDoButton->callback((fltk::Callback*)starGlyphDo_cb, this);

  // ----------------------------------
  _tractDoButton = new fltk::CheckButton(4*W/5, winy, W/5, lineH, "Tract");
  _tractDoButton->value(_seedpoint->tractDo());
  _tractDoButton->callback((fltk::Callback*)tractDo_cb, this);

  winy += lineH;

  // ----------------------------------
  _seedpoint->positionGet(pos);
  for (unsigned int pli=0; pli<3; pli++) {
    _positionSlider[pli] = new Slider(0, winy, W, incy=40,
                                      labelBuff[pli]);
    _positionSlider[pli]->align(fltk::ALIGN_LEFT);
    _positionSlider[pli]->range(0.0, sp->shape()->size[pli]-1);
    _positionSlider[pli]->fastUpdate(1);
    winy += incy;
    winy += 10;
  }
  for (unsigned int pli=0; pli<3; pli++) {
    _positionSlider[pli]->callback((fltk::Callback*)position_cb, this);
  }
  for (unsigned int pli=0; pli<3; pli++) {
    _positionSlider[pli]->value(pos[pli]);
  }

  _win->end();
}

SeedPointUI::~SeedPointUI() {
  
  //nrrdKernelSpecNix(_ksp);
}

void
SeedPointUI::show() {
  
  _win->show();
}

void
SeedPointUI::redraw() {
  
  _viewer->redraw();
}

void
SeedPointUI::cursorShow_cb(fltk::CheckButton *but, SeedPointUI *ui) {

  ui->_seedpoint->cursorShow(but->value());
  ui->redraw();
}

void
SeedPointUI::ten1GlyphDo_cb(fltk::CheckButton *but, SeedPointUI *ui) {

  ui->_seedpoint->ten1GlyphDo(but->value());
  ui->redraw();
}

void
SeedPointUI::ten2GlyphDo_cb(fltk::CheckButton *but, SeedPointUI *ui) {

  ui->_seedpoint->ten2GlyphDo(but->value());
  ui->redraw();
}

void
SeedPointUI::starGlyphDo_cb(fltk::CheckButton *but, SeedPointUI *ui) {
  char me[]="SeedPointUI::starGlyphDo_cb";

  ui->_seedpoint->starGlyphDo(but->value());
  ui->redraw();
}

void
SeedPointUI::tractDo_cb(fltk::CheckButton *but, SeedPointUI *ui) {
  unsigned int pli;

  ui->_seedpoint->tractDo(but->value());
  ui->redraw();
}

void
SeedPointUI::position_cb(Slider *slider, SeedPointUI *ui) {
  char me[]="SeedPointUI::position_cb";
  
  ui->_seedpoint->positionSet(ui->_positionSlider[0]->value(),
                              ui->_positionSlider[1]->value(),
                              ui->_positionSlider[2]->value());
  ui->_seedpoint->update();
  ui->redraw();
}

} /* namespace Deft */
