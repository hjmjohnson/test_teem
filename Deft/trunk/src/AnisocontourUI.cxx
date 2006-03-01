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

#include "AnisocontourUI.h"

namespace Deft {

// TERRIBLE: copied from TriPlaneUI.C
enum {
  kernelUnknown,
  kernelBox,
  kernelTent,
  kernelCatmulRom,
  kernelCubic333,
  kernelBSpline,
  kernelLast
};

const char kernelStr[6][128] = {
  "(unknown)",
  "box",
  "tent",
  "BC:0,1/2",
  "BC:1/3,1/3",
  "BC:1,0"
};

static char labelBuff[3][128] = {"Grid 0", "Grid 1", "Grid 2"};

AnisocontourUI::AnisocontourUI(Anisocontour *aniso, Viewer *vw) {
  const unsigned int W = 400, H = 200, lineH = 20;
  unsigned int incy, winy;
  const char *def;

  _aniso = aniso;
  _viewer = vw;
  _ksp = nrrdKernelSpecNew();

  winy = 5;
  _win = new fltk::Window(W, H, "Deft::Anisocontour");
  _win->begin();
  _win->resizable(_win);

  incy=lineH;
  for (unsigned int axi=0; axi<3; axi++) {
    _sampleSlider[axi] = new fltk::ValueSlider(67*W/100, winy + axi*incy,
                                               33*W/100, incy,
                                               labelBuff[axi]);
    _sampleSlider[axi]->range(-1, 2.5);
    _sampleSlider[axi]->step(0.01);
    _sampleSlider[axi]->align(fltk::ALIGN_LEFT);
    _sampleSlider[axi]->box(fltk::THIN_DOWN_BOX);
    _sampleSlider[axi]->color(fltk::GRAY40);
    _sampleSlider[axi]->when(fltk::WHEN_RELEASE);
    _sampleSlider[axi]->callback((fltk::Callback*)sample_cb, this);
    _sampleSlider[axi]->value(_aniso->sampling(axi));
  }

  winy += incy/2;
  _visibleButton = new fltk::CheckButton(0, winy, W/8, lineH, "Show");
  _visibleButton->callback((fltk::Callback*)visible_cb, this);
  _visibleButton->value(_aniso->visible());

  _wireframeButton = new fltk::CheckButton(14*W/100, winy,
                                           W/5, lineH, "Wire");
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

  _colorQuantityMenu = new fltk::Choice(35*W/100, winy,
                                        22*W/100, lineH, "Color");
  for (unsigned int qi=colorQuantityUnknown+1; qi<colorQuantityLast; qi++) {
    _colorQuantityMenu->add(airEnumStr(colorQuantity, qi), this);
  }
  _colorQuantityMenu->callback((fltk::Callback*)(colorQuantity_cb), this);
  def = airEnumStr(colorQuantity, _aniso->colorQuantity());
  _colorQuantityMenu->value(((fltk::Group*)_colorQuantityMenu)
                            ->find(_colorQuantityMenu
                                   ->find(def)));

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

  _anisoSlider = new Deft::Slider(0, winy, W, incy=40);
  _anisoSlider->align(fltk::ALIGN_LEFT);
  _anisoSlider->range(0.0, 1.0);
  _anisoSlider->color(0xFFFFFF00);
  _anisoSlider->fastUpdate(0);
  _anisoSlider->value(_aniso->isovalue());
  _anisoSlider->callback((fltk::Callback*)aniso_cb, this);

  winy += incy;

  _kernelMenu = new fltk::Choice(12*W/100, winy, 16*W/100,
                                 incy=lineH, "Kernel");
  for (unsigned int ki=kernelUnknown+1; ki<kernelLast; ki++) {
    _kernelMenu->add(kernelStr[ki], this);
  }
  _kernelMenu->callback((fltk::Callback*)(kernel_cb), this);
  _kernelMenu->value(((fltk::Group*)_kernelMenu)
                     ->find(_kernelMenu->find(kernelStr[kernelTent])));

  _brightSlider = new fltk::ValueSlider(53*W/80, winy, W/3, incy=lineH,
                                        "Bright");
  _brightSlider->callback((fltk::Callback*)(bright_cb), this);
  _brightSlider->range(0.3, 1.9);
  _brightSlider->step(0.01);
  _brightSlider->value(_aniso->brightness());
  _brightSlider->align(fltk::ALIGN_LEFT);
  _brightSlider->box(fltk::THIN_DOWN_BOX);
  _brightSlider->color(fltk::GRAY40);

  _win->end();
}

AnisocontourUI::~AnisocontourUI() {

  _ksp = nrrdKernelSpecNix(_ksp);
}

void
AnisocontourUI::show() {
  
  _win->show();
}

void
AnisocontourUI::redraw() {
  
  _aniso->update();
  dynamic_cast<PolyProbe*>(_aniso)->update(true);
  _viewer->redraw();
}

void
AnisocontourUI::sample_cb(fltk::ValueSlider *val, AnisocontourUI *ui) {
  unsigned int pli;

  for (pli=0; ui->_sampleSlider[pli] != val; pli++);
  ui->_aniso->sampling(pli, val->value());
  ui->redraw();
}

void
AnisocontourUI::visible_cb(fltk::CheckButton *but, AnisocontourUI *ui) {

  ui->_aniso->visible(but->value());
  ui->redraw();
}

void
AnisocontourUI::wireframe_cb(fltk::CheckButton *but, AnisocontourUI *ui) {

  ui->_aniso->wireframe(but->value());
  ui->redraw();
}

void
AnisocontourUI::colorQuantity_cb(fltk::Choice *menu, AnisocontourUI *ui) {
  unsigned int qi;

  for (qi=colorQuantityUnknown+1;
       strcmp(menu->item()->label(), airEnumStr(colorQuantity, qi));
       qi++);
  ui->_aniso->colorQuantity(qi);
  ui->redraw();
}

// TERRIBLE: copied from TriPlaneUI.C
void
AnisocontourUI::kernel_cb(fltk::Choice *menu, AnisocontourUI *ui) {
  // char me[]="AnisocontourUI::kernel_cb";
  unsigned int ki;

  for (ki=kernelUnknown+1;
       strcmp(menu->item()->label(), kernelStr[ki]);
       ki++);
  switch(ki) {
  case kernelBox:
    ui->_ksp->kernel = nrrdKernelBox;
    ELL_3V_SET(ui->_ksp->parm, 1, 0, 0);
    break;
  case kernelTent:
    ui->_ksp->kernel = nrrdKernelTent;
    ELL_3V_SET(ui->_ksp->parm, 1, 0, 0);
    break;
  case kernelCatmulRom:
    ui->_ksp->kernel = nrrdKernelBCCubic,
    ELL_3V_SET(ui->_ksp->parm, 1, 0.0, 0.5);
    break;
  case kernelBSpline:
    ui->_ksp->kernel = nrrdKernelBCCubic,
    ELL_3V_SET(ui->_ksp->parm, 1, 1.0, 0.0);
    break;
  case kernelCubic333:
    ui->_ksp->kernel = nrrdKernelBCCubic,
    ELL_3V_SET(ui->_ksp->parm, 1, 0.333333, 0.33333);
    break;
  }
  ui->_aniso->kernel(ui->_ksp);
  ui->redraw();
}

void
AnisocontourUI::anisoType_cb(fltk::Choice *menu, AnisocontourUI *ui) {

  ui->_aniso->anisoType(airEnumVal(tenAniso, menu->item()->label()));
  ui->redraw();
}

void
AnisocontourUI::aniso_cb(Slider *slider, AnisocontourUI *ui) {

  ui->_aniso->isovalue(slider->value());
  ui->redraw();
}

void
AnisocontourUI::bright_cb(fltk::ValueSlider *val, AnisocontourUI *ui) {

  ui->_aniso->brightness(static_cast<float>(val->value()));
  ui->redraw();
}


} /* namespace Deft */
