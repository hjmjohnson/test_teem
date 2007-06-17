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

#include "AnisofeatureUI.h"

namespace Deft {

AnisofeatureUI::AnisofeatureUI(Anisofeature *aniso, Viewer *vw) {
  char me[]="AnisofeatureUI::AnisofeatureUI";
  const unsigned int W = 400, H = 230, lineH = 20;
  unsigned int incy, winy;
  const char *def;

  _aniso = aniso;
  _viewer = vw;

  winy = 5;
  _win = new fltk::Window(W, H, "Deft::Anisofeature");
  _win->begin();
  _win->resizable(_win);

  incy=lineH;
  _sampleSlider = new fltk::ValueSlider(67*W/100, winy,
                                        33*W/100, lineH, "Grid");
  _sampleSlider->range(-1, 2.5);
  _sampleSlider->step(0.01);
  _sampleSlider->align(fltk::ALIGN_LEFT);
  _sampleSlider->box(fltk::THIN_DOWN_BOX);
  _sampleSlider->color(fltk::GRAY40);
  _sampleSlider->when(fltk::WHEN_RELEASE);
  _sampleSlider->callback((fltk::Callback*)sample_cb, this);
  double smp = _aniso->sampling();
  _sampleSlider->value(smp);

  winy += incy;
  _visibleButton = new fltk::CheckButton(0, winy, 13*W/100, lineH, "Show");
  _visibleButton->callback((fltk::Callback*)visible_cb, this);
  _visibleButton->value(_aniso->visible());


  _wireframeButton = new fltk::CheckButton(14*W/100, winy, 
                                           20*W/100, lineH, "Wire");
  _wireframeButton->callback((fltk::Callback*)wireframe_cb, this);
  _wireframeButton->value(_aniso->wireframe());


  /*
  _glyphsDoButton = new fltk::CheckButton(0, winy + lineH,
                                          W/5, lineH, "Glyph");
  _glyphsDoButton->callback((fltk::Callback*)glyphsDo_cb, this);
  _glyphsDoButton->value(_aniso->glyphsDo());

  _tractsDoButton = new fltk::CheckButton(14*W/100, winy + lineH,
                                          W/5, lineH, "Tract");
  _tractsDoButton->callback((fltk::Callback*)tractsDo_cb, this);
  _tractsDoButton->value(_aniso->tractsDo());
  */

  _colorButton = new fltk::CheckButton(27*W/100, winy, 
                                       20*W/100, lineH, "Color");
  _colorButton->callback((fltk::Callback*)color_cb, this);
  _colorButton->value(_aniso->color());

  _colorQuantityMenu = new fltk::Choice(39*W/100, winy,
                                        22*W/100, lineH);
  const airEnum *enm = aniso->colorQuantityEnum();
  int itu = airEnumUnknown(enm);
  int itl = airEnumLast(enm);
  for (int qi=itu+1; qi<=itl; qi++) {
    _colorQuantityMenu->add(airEnumStr(enm, qi), this);
  }
  _colorQuantityMenu->callback((fltk::Callback*)(colorQuantity_cb), this);
  def = airEnumStr(enm, _aniso->colorQuantity());
  _colorQuantityMenu->value(((fltk::Group*)_colorQuantityMenu)
                            ->find(_colorQuantityMenu
                                   ->find(def)));

  /*
  winy += 5*incy/2;
  _anisoTypeMenu = new fltk::Choice(W/6, winy, W/8, incy=lineH, "Anisotropy");
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_FA), this);
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cl2), this);
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cp2), this);
  _anisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Ca2), this);
  _anisoTypeMenu->callback((fltk::Callback*)(anisoType_cb), this);
  def = airEnumStr(tenAniso, _aniso->anisoType());
  fltk::Group *tgroup = (fltk::Group*)(_anisoTypeMenu);
  _anisoTypeMenu->value(tgroup->find(_anisoTypeMenu->find(def)));
  */

  _strengthSlider = new Deft::Slider(0, winy, W, incy=40);
  _strengthSlider->align(fltk::ALIGN_LEFT);
  _strengthSlider->range(0.0, 1.0);
  _strengthSlider->color(0xFFFFFF00);
  _strengthSlider->fastUpdate(0);
  _strengthSlider->value(_aniso->strength());
  _strengthSlider->callback((fltk::Callback*)strength_cb, this);

  winy += incy;

  _brightSlider = new fltk::ValueSlider(53*W/80, winy, W/3, incy=lineH,
                                        "Bright");
  _brightSlider->callback((fltk::Callback*)(bright_cb), this);
  _brightSlider->range(0.3, 1.9);
  _brightSlider->step(0.01);
  _brightSlider->value(_aniso->brightness());
  _brightSlider->align(fltk::ALIGN_LEFT);
  _brightSlider->box(fltk::THIN_DOWN_BOX);
  _brightSlider->color(fltk::GRAY40);

  winy += incy;
  _updateButton = new fltk::Button(8*W/12, winy, W/4, incy=lineH,
                                   "Update");
  _updateButton->callback((fltk::Callback*)updateButton_cb, this);

// TERRIBLE: copied from TriPlaneUI.C
  winy += incy;
  _alphaMaskQuantityMenu = new fltk::Choice(7*W/80, winy, W/7, lineH, "Mask");
  enm = aniso->alphaMaskQuantityEnum();
  itu = airEnumUnknown(enm);
  itl = airEnumLast(enm);
  for (int qi=itu+1; qi<=itl; qi++) {
    _alphaMaskQuantityMenu->add(airEnumStr(enm, qi), this);
  }
  _alphaMaskQuantityMenu->callback((fltk::Callback*)(alphaMaskQuantity_cb),
                                   this);
  def = airEnumStr(enm, _aniso->alphaMaskQuantity());
  fprintf(stderr, "anisofeature def = %s\n", def);

  _alphaMaskQuantityMenu->value(((fltk::Group*)_alphaMaskQuantityMenu)
                                ->find(_alphaMaskQuantityMenu
                                       ->find(def)));

  winy += incy;
  _alphaMaskThresholdSlider = new Deft::Slider(0, winy, W, incy=40);
  _alphaMaskThresholdSlider->align(fltk::ALIGN_LEFT);
  _alphaMaskThresholdSlider->range(0.0, 1.0);
  _alphaMaskThresholdSlider->color(0xFFFFFF00);
  _alphaMaskThresholdSlider->fastUpdate(0);
  _alphaMaskThresholdSlider->value(_aniso->strength());
  _alphaMaskThresholdSlider->callback((fltk::Callback*)strength_cb, this);

  _alphaMaskThresholdSlider->callback((fltk::Callback*)alphaMaskThreshold_cb,
                                      this);
  _alphaMaskThresholdSlider->value(_aniso->alphaMaskThreshold());

  winy += incy;
  _ccDoButton = new fltk::CheckButton(27*W/100, winy, 
                                      20*W/100, lineH, "CC");
  _ccDoButton->callback((fltk::Callback*)ccDo_cb, this);
  _ccDoButton->value(_aniso->ccDo());
  _ccSingleButton = new fltk::CheckButton(37*W/100, winy, 
                                          20*W/100, lineH, "Single");
  _ccSingleButton->callback((fltk::Callback*)ccSingle_cb, this);
  _ccSingleButton->value(_aniso->ccSingle());

  _ccIdSlider = new Deft::Slider(0, winy, W, incy=40);
  _ccIdSlider->align(fltk::ALIGN_LEFT);
  _ccIdSlider->range(0, 1);
  _ccIdSlider->color(0xFFFFFF00);
  _ccIdSlider->fastUpdate(0);
  _ccIdSlider->value(0);
  _ccIdSlider->callback((fltk::Callback*)ccId_cb, this);
  _ccIdSlider->value(_aniso->ccId());
  _ccIdSlider->step(1.0);

  _once = false;

  _win->end();
  fprintf(stderr, "%s: bye\n", me);
}

AnisofeatureUI::~AnisofeatureUI() {

}

void
AnisofeatureUI::show() {
  
  _win->show();
}

void
AnisofeatureUI::redraw() {
  
  // _aniso->update();
  dynamic_cast<PolyProbe*>(_aniso)->update(true);
  _viewer->redraw();
}

void
AnisofeatureUI::sample_cb(fltk::ValueSlider *val, AnisofeatureUI *ui) {
  char me[]="AnisofeatureUI::sample_cb";

  fprintf(stderr, "!%s: hi\n", me);
  ui->_aniso->sampling(val->value());
  fprintf(stderr, "!%s: bye\n", me);
}

void
AnisofeatureUI::visible_cb(fltk::CheckButton *but, AnisofeatureUI *ui) {

  ui->_aniso->visible(but->value());
  ui->redraw();
}

void
AnisofeatureUI::wireframe_cb(fltk::CheckButton *but, AnisofeatureUI *ui) {

  ui->_aniso->wireframe(but->value());
  ui->redraw();
}

void
AnisofeatureUI::color_cb(fltk::CheckButton *but,
                         AnisofeatureUI *ui) {

  ui->_aniso->color(but->value());
  dynamic_cast<PolyProbe*>(ui->_aniso)->update(false);
  ui->redraw();
}

void
AnisofeatureUI::colorQuantity_cb(fltk::Choice *menu, AnisofeatureUI *ui) {
  int qi;

  const airEnum *enm = ui->_aniso->colorQuantityEnum();
  int itu = airEnumUnknown(enm);
  for (qi=itu+1;
       strcmp(menu->item()->label(), airEnumStr(enm, qi));
       qi++);
  ui->_aniso->colorQuantity(qi);
  ui->redraw();
}

/*
void
AnisofeatureUI::anisoType_cb(fltk::Choice *menu, AnisofeatureUI *ui) {

  ui->_aniso->anisoType(airEnumVal(tenAniso, menu->item()->label()));
  ui->redraw();
}
*/

// TERRIBLE: copied from TriPlaneUI.C
void
AnisofeatureUI::alphaMaskQuantity_cb(fltk::Choice *menu, AnisofeatureUI *ui) {
  unsigned int qi;

  const airEnum *enm = ui->_aniso->alphaMaskQuantityEnum();
  int itu = airEnumUnknown(enm);
  for (qi=itu+1;
       strcmp(menu->item()->label(), airEnumStr(enm, qi));
       qi++);
  ui->_aniso->alphaMaskQuantity(qi);
  dynamic_cast<PolyProbe*>(ui->_aniso)->update(true);
  ui->redraw();
}

void
AnisofeatureUI::alphaMaskThreshold_cb(Slider *val, AnisofeatureUI *ui) {

  ui->_aniso->alphaMaskThreshold(val->value());
  dynamic_cast<PolyProbe*>(ui->_aniso)->update(true);
  ui->redraw();
}


void
AnisofeatureUI::strength_cb(Slider *slider, AnisofeatureUI *ui) {

  ui->_aniso->strength(+1, slider->value());
}


void
AnisofeatureUI::bright_cb(fltk::ValueSlider *val, AnisofeatureUI *ui) {

  ui->_aniso->brightness(static_cast<float>(val->value()));
  ui->redraw();
}

void
AnisofeatureUI::updateButton_cb(fltk::Button *, AnisofeatureUI *ui) {

  ui->_aniso->update();
  ui->_ccIdSlider->range(0, ui->_aniso->ccNum()-1);
  ui->_once = true;
  ui->redraw();
}

void
AnisofeatureUI::ccDo_cb(fltk::CheckButton *but, AnisofeatureUI *ui) {

  ui->_aniso->ccDo(but->value());
  if (ui->_once) {
    ui->_aniso->update();
  }
  ui->redraw();
}

void
AnisofeatureUI::ccSingle_cb(fltk::CheckButton *but, AnisofeatureUI *ui) {

  ui->_aniso->ccSingle(but->value());
  if (ui->_once) {
    ui->_aniso->update();
  }
  ui->redraw();
}

void
AnisofeatureUI::ccId_cb(Slider *val, AnisofeatureUI *ui) {

  ui->_aniso->ccId(static_cast<unsigned int>(val->value()));
  if (ui->_once) {
    ui->_aniso->update();
  }
  ui->redraw();
}

} /* namespace Deft */
