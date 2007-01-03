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

/* learned:
** this will work inside a function (that isn't main, I guess):
**     Fl_Button *camReset = new Fl_Button(0, 0, 100, 20, "Camera Reset");
** this will segfault and die:
**     Fl_Button camReset(0, 0, 100, 20, "Camera Reset");
** I'm not even using the button again; just creating it.  This must
** have to do with the longevity of the camReset variable WRT to the
** function ...
*/

#include "TensorGlyphUI.h"

namespace Deft {

TensorGlyphUI::TensorGlyphUI(TensorGlyph *tg, Viewer *vw) {
  const unsigned int W = 400, H = 307, lineH = 20;
  unsigned int winy, incy;
  const char *def;
  
  _tglyph.resize(1);
  _tglyph[0] = tg;
  _tglyph[0]->postDrawCallback((Callback*)(postDraw_cb), this);
  _viewer = vw;

  winy = 0;
  _win = new fltk::Window(W, H, "Deft::TensorGlyph");
  _win->begin();
  _win->resizable(_win);

  winy += 5;

  // ----------------------------------
  _visibleButton = new fltk::CheckButton(5, winy, W/4, incy=lineH,
                                         "Show Glyphs");
  _visibleButton->callback((fltk::Callback*)visible_cb, this);
  _visibleButton->value(_tglyph[0]->visible());

  // ----------------------------------
  _glyphTypeMenu = new fltk::Choice(3*W/8, winy, W/5, incy=lineH, "Shape");
  _glyphTypeMenu->add(airEnumStr(tenGlyphType, tenGlyphTypeBox), this);
  _glyphTypeMenu->add(airEnumStr(tenGlyphType, tenGlyphTypeSphere), this);
  _glyphTypeMenu->add(airEnumStr(tenGlyphType, tenGlyphTypeCylinder), this);
  _glyphTypeMenu->add(airEnumStr(tenGlyphType, tenGlyphTypeSuperquad), this);
  _glyphTypeMenu->callback((fltk::Callback*)(glyphType_cb), this);
  def = airEnumStr(tenGlyphType, _tglyph[0]->glyphType());
  fltk::Group *tgroup = (fltk::Group*)(_glyphTypeMenu);
  _glyphTypeMenu->value(tgroup->find(_glyphTypeMenu->find(def)));

  // ----------------------------------
  _glyphColorMenu = new fltk::Choice(7*W/10, winy, W/4, incy=lineH, "Color");
  _glyphColorMenu->add("RGB(linear)", this);
  _glyphColorMenu->add("RGB(planar)", this);
  _glyphColorMenu->add("Mode", this);
  _glyphColorMenu->add("(Mode,FA)", this);
  _glyphColorMenu->add("White", this);
  // _glyphColorMenu->callback((fltk::Callback*)(glyphColor_cb), this);
  // def = airEnumStr(tenGlyphType, _tglyph[0]->glyphType());
  // fltk::Group *tgroup = (fltk::Group*)(_glyphTypeMenu);
  // _glyphTypeMenu->value(tgroup->find(_glyphTypeMenu->find(def)));

  winy += incy;
  winy += 10;

  // ----------------------------------
  _anisoTypeMenu = new fltk::Choice(W/6, winy, W/8, incy=lineH, "Anisotropy");
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_FA), this);
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cl2), this);
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cp2), this);
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Ca2), this);
  _anisoTypeMenu->callback((fltk::Callback*)(anisoType_cb), this);
  def = airEnumStr(tenAniso, _tglyph[0]->anisoType());
  tgroup = (fltk::Group*)(_anisoTypeMenu);
  _anisoTypeMenu->value(tgroup->find(_anisoTypeMenu->find(def)));

  // ----------------------------------
  _glyphResInput = new fltk::ValueInput(9*W/16, winy, W/11, lineH,
                                        "Glyph Facet");
  _glyphResInput->callback((fltk::Callback*)(glyphRes_cb), this);
  _glyphResInput->range(2, 30);
  _glyphResInput->step(1);
  _glyphResInput->value(_tglyph[0]->glyphResolution());

  // ----------------------------------
  _superquadSharpnessInput = new fltk::ValueInput(7*W/8, winy, W/10, lineH,
                                                  "Sqd sharp");
  _superquadSharpnessInput->callback((fltk::Callback*)
                                     (superquadSharpness_cb), this);
  _superquadSharpnessInput->range(0.0, 15.0);
  _superquadSharpnessInput->step(0.1);
  _superquadSharpnessInput->linesize(0.1);
  _superquadSharpnessInput->value(_tglyph[0]->superquadSharpness());

  winy += incy;
  winy += 10;

  // ----------------------------------
  _glyphScaleSlider = new Deft::Slider(0, winy, W, incy=40, 
                                        "Glyph Scaling");
  _glyphScaleSlider->align(fltk::ALIGN_LEFT);
  _glyphScaleSlider->range(0.0, 2*_tglyph[0]->glyphScale());
  _glyphScaleSlider->fastUpdate(1);
  _glyphScaleSlider->value(_tglyph[0]->glyphScale());
  _glyphScaleSlider->callback((fltk::Callback*)(glyphScale_cb), this);

  winy += incy;
  winy += 10;

  // ----------------------------------
  _anisoThreshSlider = new Deft::Slider(0, winy, W, incy=40, 
                                         "Anisotropy Threshold");
  _anisoThreshSlider->align(fltk::ALIGN_LEFT);
  _anisoThreshSlider->range(_tglyph[0]->anisoThreshMin(), 1.0);
  _anisoThreshSlider->fastUpdate(1);
  _anisoThreshSlider->value(_tglyph[0]->anisoThresh());
  _anisoThreshSlider->callback((fltk::Callback*)(anisoThresh_cb), this);

  winy += incy;
  winy += 10;

  // ----------------------------------
  _anisoThreshMinSlider = new Deft::Slider(0, winy, W, incy=40, 
                                            "Threshold Minimum");
  _anisoThreshMinSlider->align(fltk::ALIGN_LEFT);
  _anisoThreshMinSlider->range(0.0, 1.0);
  _anisoThreshMinSlider->fastUpdate(0);
  _anisoThreshMinSlider->value(_tglyph[0]->anisoThreshMin());
  _anisoThreshMinSlider->callback((fltk::Callback*)(anisoThreshMin_cb), this);

  winy += incy;
  winy += 10;

  _drawnNumOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                     "# glyphs drawn");
  _drawnNumOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;

  _polygonsPerGlyphOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                             "polygons / glyph");
  _polygonsPerGlyphOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;

  _KglyphsPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                             "K glyphs / second");
  _KglyphsPerSecondOutput->box(fltk::NO_BOX);

  _hackButton = new fltk::Button(17*W/20, winy, W/10, lineH, "hack");
  _hackButton->callback((fltk::Callback*)_hack_cb, this);

  winy += incy;
  winy += 2;

  _MpolygonsPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                               "M polygons / second");
  _MpolygonsPerSecondOutput->box(fltk::NO_BOX);

  _hestButton = new fltk::Button(17*W/20, winy, W/10, incy=lineH, "hest");
  _hestButton->callback((fltk::Callback*)_hest_cb, this);

  winy += incy;
  _win->end();
}

TensorGlyphUI::~TensorGlyphUI() {

  // nothing?
}

void
TensorGlyphUI::add(TensorGlyph *tg) {
  
  _tglyph.resize(_tglyph.size()+1);
  _tglyph[_tglyph.size()-1] = tg;
}

void
TensorGlyphUI::show() {
  
  _win->show();
}

void
TensorGlyphUI::redraw() {
  char buff[AIR_STRLEN_MED];
  
  for (unsigned int idx=0; idx<_tglyph.size(); idx++) {
    if (_tglyph[idx]->visible()) {
      _tglyph[idx]->update();
    }
  }
  _viewer->redraw();
  if (_tglyph[0]->visible()) {
    sprintf(buff, "%u", _tglyph[0]->glyphsDrawnNum());
    _drawnNumOutput->value(buff);
    sprintf(buff, "%u", _tglyph[0]->polygonsPerGlyph());
    _polygonsPerGlyphOutput->value(buff);
  } else {
    _drawnNumOutput->value("");
    _polygonsPerGlyphOutput->value("");
    _KglyphsPerSecondOutput->value("");
    _MpolygonsPerSecondOutput->value("");
  }
}

void
TensorGlyphUI::visible_cb(fltk::CheckButton *but, TensorGlyphUI *ui) {

  // do not set visibility for slaves
  ui->_tglyph[0]->visible(but->value());
  ui->redraw();
}

void
TensorGlyphUI::anisoType_cb(fltk::Choice *menu, TensorGlyphUI *ui) {

  for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
    ui->_tglyph[idx]->anisoType(airEnumVal(tenAniso, menu->item()->label()));
  }
  ui->redraw();
}

void
TensorGlyphUI::glyphType_cb(fltk::Choice *menu, TensorGlyphUI *ui) {

  for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
    ui->_tglyph[idx]->glyphType(airEnumVal(tenGlyphType,
                                           menu->item()->label()));
  }
  ui->redraw();
}

void
TensorGlyphUI::glyphRes_cb(fltk::ValueInput *val, TensorGlyphUI *ui) {

  if ((unsigned int)val->value() < 2) {
    val->value(2);
  }
  for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
    ui->_tglyph[idx]->glyphResolution((unsigned int)val->value());
  }
  ui->redraw();
}

void
TensorGlyphUI::superquadSharpness_cb(fltk::ValueInput *val,
                                     TensorGlyphUI *ui) {

  for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
    ui->_tglyph[idx]->superquadSharpness(val->value());
  }
  ui->redraw();
}

void
TensorGlyphUI::glyphScale_cb(Slider *slider, TensorGlyphUI *ui) {

  for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
    ui->_tglyph[idx]->glyphScale(slider->value());
  }
  ui->redraw();
}

void
TensorGlyphUI::anisoThresh_cb(Slider *slider, TensorGlyphUI *ui) {

  // For the time being, this applies ONLY to the first attached tglyph
  // for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
  if (slider->value() != ui->_tglyph[0]->anisoThresh()) {
    ui->_tglyph[0]->anisoThresh(slider->value());
  }
  // }
  ui->redraw();
}

void
TensorGlyphUI::anisoThreshMin_cb(Slider *slider, TensorGlyphUI *ui) {

  ui->_anisoThreshSlider->range(slider->value(), 1);
  for (unsigned int idx=0; idx<ui->_tglyph.size(); idx++) {
    ui->_tglyph[idx]->anisoThreshMin(slider->value());
  }
  ui->redraw();
}

void
TensorGlyphUI::postDraw_cb(TensorGlyph *tglyph, TensorGlyphUI *ui) {
  char buff[AIR_STRLEN_MED];

  if (tglyph->visible()) {
    sprintf(buff, "%u", tglyph->glyphsDrawnNum());
    ui->_drawnNumOutput->value(buff);
    sprintf(buff, "%u", tglyph->polygonsPerGlyph());
    ui->_polygonsPerGlyphOutput->value(buff);
    sprintf(buff, "%g",
            tglyph->glyphsDrawnNum()/(1000*tglyph->drawTime()));
    ui->_KglyphsPerSecondOutput->value(buff);
    sprintf(buff, "%g",
            tglyph->polygonsPerGlyph()*tglyph->glyphsDrawnNum()
            /(1000*1000*tglyph->drawTime()));
    ui->_MpolygonsPerSecondOutput->value(buff);
  } else {
    ui->_KglyphsPerSecondOutput->value("");
    ui->_MpolygonsPerSecondOutput->value("");
  }
}

void
TensorGlyphUI::_hest_cb(fltk::Button *, TensorGlyphUI *ui) {

  fprintf(stderr, "-g %s -gsc %g -gr %d -sh %g", 
          airEnumStr(tenGlyphType, ui->_tglyph[0]->glyphType()),
          ui->_tglyph[0]->glyphScale(),
          ui->_tglyph[0]->glyphResolution(),
          ui->_tglyph[0]->superquadSharpness());
  fprintf(stderr, " -a %s -atrm %g -atr %g \\\n", 
          airEnumStr(tenAniso, ui->_tglyph[0]->anisoType()),
          ui->_tglyph[0]->anisoThreshMin(),
          ui->_tglyph[0]->anisoThresh());
}

void
TensorGlyphUI::_hack_cb(fltk::Button *, TensorGlyphUI *ui) {
  
  ui->_tglyph[0]->saveInventor();
}


};
