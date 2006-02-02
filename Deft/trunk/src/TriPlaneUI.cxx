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

#include "TriPlaneUI.h"

namespace Deft {

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

char labelBuff[4][128] = {"All", "Axis 0", "Axis 1", "Axis 2"};

TriPlaneUI::TriPlaneUI(TriPlane *tp, Viewer *vw) {
  // char me[]="TriPlaneUI::TriPlaneUI";
  const unsigned int W=400, H=365, lineH=20;
  unsigned int winy, incy;
  const char *def;

  _ksp = nrrdKernelSpecNew();
  _triplane = tp;
  _viewer = vw;

  winy = 0;
  _win = new fltk::Window(W, H, "Deft::TriPlane");
  _win->begin();
  _win->resizable(_win);
  winy += 5;

  // ----------------------------------
  for (unsigned int pli=0; pli<=3; pli++) {
    _visibleButton[pli] = new fltk::CheckButton(0, winy,
                                                W/8, lineH, 
                                                labelBuff[pli]);
    _visibleButton[pli]->callback((fltk::Callback*)visible_cb, this);
    _visibleButton[pli]->value(!pli
                               ? _triplane->visible()
                               : _triplane->plane[pli-1]->visible());

    _wireframeButton[pli] = new fltk::CheckButton(14*W/100, winy,
                                                  W/5, lineH, "Wire");
    _wireframeButton[pli]->callback((fltk::Callback*)wireframe_cb, this);
    _wireframeButton[pli]->value(!pli
                                 ? _triplane->wireframe()
                                 : _triplane->plane[pli-1]->wireframe());

    _colorQuantityMenu[pli] = new fltk::Choice(37*W/100, winy,
                                               22*W/100, lineH, "Color");
    for (unsigned int qi=colorQuantityUnknown+1; qi<colorQuantityLast; qi++) {
      _colorQuantityMenu[pli]->add(airEnumStr(colorQuantity, qi), this);
    }
    _colorQuantityMenu[pli]->callback((fltk::Callback*)(colorQuantity_cb),
                                      this);
    if (pli) {
      def = airEnumStr(colorQuantity,
                       _triplane->plane[pli-1]->colorQuantity());
      _colorQuantityMenu[pli]->value(((fltk::Group*)_colorQuantityMenu[pli])
                                     ->find(_colorQuantityMenu[pli]
                                            ->find(def)));
    }

    _sampleSlider[pli] = new fltk::ValueSlider(67*W/100, winy,
                                               33*W/100, incy=lineH,
                                               "Grid");
    _sampleSlider[pli]->range(-1, 2.5);
    _sampleSlider[pli]->step(0.01);
    _sampleSlider[pli]->align(fltk::ALIGN_LEFT);
    _sampleSlider[pli]->box(fltk::THIN_DOWN_BOX);
    _sampleSlider[pli]->color(fltk::GRAY40);
    _sampleSlider[pli]->when(fltk::WHEN_RELEASE);
    _sampleSlider[pli]->callback((fltk::Callback*)sample_cb, this);
    _sampleSlider[pli]->value(!pli
                              ? _triplane->sampling(0)
                              : _triplane->sampling(pli-1));

    _glyphsDoButton[pli] = new fltk::CheckButton(0, winy + lineH,
                                                 W/5, lineH, "Glyph");
    _glyphsDoButton[pli]->callback((fltk::Callback*)glyphsDo_cb, this);
    _glyphsDoButton[pli]->value(!pli
                                ? _triplane->glyphsDo(0)
                                : _triplane->glyphsDo(pli-1));

    _tractsDoButton[pli] = new fltk::CheckButton(14*W/100, winy + lineH,
                                                 W/5, lineH, "Tract");
    _tractsDoButton[pli]->callback((fltk::Callback*)tractsDo_cb, this);
    _tractsDoButton[pli]->value(!pli
                                ? _triplane->tractsDo(0)
                                : _triplane->tractsDo(pli-1));

    _seedSampleSlider[pli] = new fltk::ValueSlider(28*W/100, winy + lineH,
                                                   13*W/40, incy=lineH
                                                   /*, "Seed" */);
    _seedSampleSlider[pli]->range(-1, 2.5);
    _seedSampleSlider[pli]->step(0.01);
    _seedSampleSlider[pli]->align(fltk::ALIGN_LEFT);
    _seedSampleSlider[pli]->box(fltk::THIN_DOWN_BOX);
    _seedSampleSlider[pli]->color(fltk::GRAY40);
    _seedSampleSlider[pli]->when(fltk::WHEN_RELEASE);
    _seedSampleSlider[pli]->callback((fltk::Callback*)seedSample_cb, this);
    _seedSampleSlider[pli]->value(!pli
                                  ? _triplane->seedSampling(0)
                                  : _triplane->seedSampling(pli-1));

    winy += lineH;

    if (pli) {
      _positionSlider[pli-1] = new Slider(0, winy, W, incy=40);
      _positionSlider[pli-1]->align(fltk::ALIGN_LEFT);
      _positionSlider[pli-1]->range(0.0, tp->shape()->size[pli-1]-1);
      _positionSlider[pli-1]->fastUpdate(1);
      _positionSlider[pli-1]->callback((fltk::Callback*)position_cb, this);
      _positionSlider[pli-1]->value(_triplane->position(pli-1));
      winy += incy;
    } else {
      winy += incy;
    }
    winy += 10;
  }

  _seedAnisoThreshSlider = new Deft::Slider(0, winy, W, incy=40, 
                                            "Seed Aniso Thresh");
  _seedAnisoThreshSlider->align(fltk::ALIGN_LEFT);
  _seedAnisoThreshSlider->range(0.0, 1.0);
  _seedAnisoThreshSlider->fastUpdate(1);
  _seedAnisoThreshSlider->value(_triplane->seedAnisoThresh());
  _seedAnisoThreshSlider->callback((fltk::Callback*)(seedAnisoThresh_cb),
                                   this);
  winy += incy;
  winy += 10;

  _alphaMaskQuantityMenu = new fltk::Choice(7*W/80, winy, W/7, lineH, "Mask");
  for (unsigned int qi=alphaMaskQuantityUnknown+1;
       qi<alphaMaskQuantityLast;
       qi++) {
    _alphaMaskQuantityMenu->add(airEnumStr(alphaMaskQuantity, qi), this);
  }
  _alphaMaskQuantityMenu->callback((fltk::Callback*)(alphaMaskQuantity_cb),
                                   this);
  def = airEnumStr(alphaMaskQuantity,
                   _triplane->plane[0]->alphaMaskQuantity());
  _alphaMaskQuantityMenu->value(((fltk::Group*)_alphaMaskQuantityMenu)
                                ->find(_alphaMaskQuantityMenu
                                       ->find(def)));
  _alphaMaskThresholdSlider = new fltk::ValueSlider(18*W/80, winy, W/3, lineH);
  _alphaMaskThresholdSlider->range(0.0, 1.0);
  _alphaMaskThresholdSlider->step(0.01);
  _alphaMaskThresholdSlider->align(fltk::ALIGN_LEFT);
  _alphaMaskThresholdSlider->box(fltk::THIN_DOWN_BOX);
  _alphaMaskThresholdSlider->color(fltk::GRAY40);
  _alphaMaskThresholdSlider->callback((fltk::Callback*)alphaMaskThreshold_cb,
                                      this);
  _alphaMaskThresholdSlider->value(_triplane->plane[0]->alphaMaskThreshold());

  _brightSlider = new fltk::ValueSlider(53*W/80, winy, W/3, incy=lineH,
                                        "Bright");
  _brightSlider->callback((fltk::Callback*)(bright_cb), this);
  _brightSlider->range(0.3, 1.9);
  _brightSlider->step(0.01);
  _brightSlider->value(_triplane->plane[0]->brightness());
  _brightSlider->align(fltk::ALIGN_LEFT);
  _brightSlider->box(fltk::THIN_DOWN_BOX);
  _brightSlider->color(fltk::GRAY40);

  winy += lineH;
  winy += 10;

  _kernelMenu = new fltk::Choice(6*W/50, winy, 8*W/50, incy=lineH, "Kernel");
  for (unsigned int ki=kernelUnknown+1; ki<kernelLast; ki++) {
    _kernelMenu->add(kernelStr[ki], this);
  }
  _kernelMenu->callback((fltk::Callback*)(kernel_cb), this);
  _kernelMenu->value(((fltk::Group*)_kernelMenu)
                     ->find(_kernelMenu->find(kernelStr[kernelTent])));
}

TriPlaneUI::~TriPlaneUI() {
  
  nrrdKernelSpecNix(_ksp);
}

void
TriPlaneUI::show() {
  
  _win->show();
}

void
TriPlaneUI::redraw() {
  
  _viewer->redraw();
}

void
TriPlaneUI::visible_cb(fltk::CheckButton *but, TriPlaneUI *ui) {
  // char me[]="TriPlaneUI::visible_cb";
  unsigned int pli;

  for (pli=0; ui->_visibleButton[pli] != but; pli++);
  if (!pli) {
    ui->_triplane->plane[0]->visible(but->value());
    ui->_triplane->plane[1]->visible(but->value());
    ui->_triplane->plane[2]->visible(but->value());
    if (but->value()) {
      ui->_triplane->position(0, ui->_triplane->position(0));
      ui->_triplane->position(1, ui->_triplane->position(1));
      ui->_triplane->position(2, ui->_triplane->position(2));
    }
    ui->_visibleButton[1]->value(but->value());
    ui->_visibleButton[2]->value(but->value());
    ui->_visibleButton[3]->value(but->value());
  } else {
    ui->_triplane->plane[pli-1]->visible(but->value());
    if (but->value()) {
      ui->_triplane->position(pli-1, ui->_triplane->position(pli-1));
    }
  }
  ui->redraw();
}

void
TriPlaneUI::wireframe_cb(fltk::CheckButton *but, TriPlaneUI *ui) {
  unsigned int pli;

  for (pli=0; ui->_wireframeButton[pli] != but; pli++);
  if (!pli) {
    ui->_triplane->wireframe(but->value());
    ui->_wireframeButton[1]->value(but->value());
    ui->_wireframeButton[2]->value(but->value());
    ui->_wireframeButton[3]->value(but->value());
  } else {
    ui->_triplane->plane[pli-1]->wireframe(but->value());
  }
  ui->redraw();
}

void
TriPlaneUI::glyphsDo_cb(fltk::CheckButton *but, TriPlaneUI *ui) {
  unsigned int pli;

  for (pli=0; ui->_glyphsDoButton[pli] != but; pli++);
  if (!pli) {
    ui->_triplane->glyphsDo(0, but->value());
    ui->_triplane->glyphsDo(1, but->value());
    ui->_triplane->glyphsDo(2, but->value());
    ui->_glyphsDoButton[1]->value(but->value());
    ui->_glyphsDoButton[2]->value(but->value());
    ui->_glyphsDoButton[3]->value(but->value());
  } else {
    ui->_triplane->glyphsDo(pli-1, but->value());
  }
  ui->redraw();
}

void
TriPlaneUI::tractsDo_cb(fltk::CheckButton *but, TriPlaneUI *ui) {
  unsigned int pli;

  for (pli=0; ui->_tractsDoButton[pli] != but; pli++);
  if (!pli) {
    ui->_triplane->tractsDo(1, but->value());
    ui->_triplane->tractsDo(2, but->value());
    ui->_triplane->tractsDo(3, but->value());
    ui->_tractsDoButton[1]->value(but->value());
    ui->_tractsDoButton[2]->value(but->value());
    ui->_tractsDoButton[3]->value(but->value());
  } else {
    ui->_triplane->tractsDo(pli-1, but->value());
  }
  ui->redraw();
}

void
TriPlaneUI::alphaMaskQuantity_cb(fltk::Choice *menu, TriPlaneUI *ui) {
  // char me[]="TriPlaneUI::alphaMaskQuantity_cb";
  unsigned int qi;

  for (qi=alphaMaskQuantityUnknown+1;
       strcmp(menu->item()->label(), airEnumStr(alphaMaskQuantity, qi));
       qi++);
  ui->_triplane->alphaMaskQuantity(qi);
  ui->_triplane->update();
  ui->redraw();
}

void
TriPlaneUI::sample_cb(fltk::ValueSlider *val, TriPlaneUI *ui) {
  unsigned int pli;

  for (pli=0; ui->_sampleSlider[pli] != val; pli++);
  if (!pli) {
    ui->_triplane->sampling(0, val->value());
    ui->_triplane->sampling(1, val->value());
    ui->_triplane->sampling(2, val->value());
    ui->_sampleSlider[1]->value(val->value());
    ui->_sampleSlider[2]->value(val->value());
    ui->_sampleSlider[3]->value(val->value());
  } else {
    ui->_triplane->sampling(pli-1, val->value());
  }
  ui->redraw();
}

void
TriPlaneUI::seedSample_cb(fltk::ValueSlider *sld, TriPlaneUI *ui) {
  unsigned int pli;

  for (pli=0; ui->_seedSampleSlider[pli] != sld; pli++);
  if (!pli) {
    ui->_triplane->seedSampling(0, sld->value());
    ui->_triplane->seedSampling(1, sld->value());
    ui->_triplane->seedSampling(2, sld->value());
    ui->_triplane->glyphsUpdate(0);
    ui->_triplane->glyphsUpdate(1);
    ui->_triplane->glyphsUpdate(2);
    ui->_triplane->tractsUpdate(0);
    ui->_triplane->tractsUpdate(1);
    ui->_triplane->tractsUpdate(2);
    ui->_seedSampleSlider[1]->value(sld->value());
    ui->_seedSampleSlider[2]->value(sld->value());
    ui->_seedSampleSlider[3]->value(sld->value());
  } else {
    ui->_triplane->seedSampling(pli-1, sld->value());
    // HEY: this logic doesn't belong here! 
    for (unsigned int ii=0; ii<=2; ii++) {
      if (ii != pli-1) {
        ui->_triplane->glyphsUpdate(ii);
        ui->_triplane->tractsUpdate(ii);
      }
    }
  }
  ui->redraw();
}

void
TriPlaneUI::position_cb(Slider *slider, TriPlaneUI *ui) {
  // char me[]="TriPlaneUI::position_cb";
  unsigned int pli;

  for (pli=0; ui->_positionSlider[pli] != slider; pli++);
  ui->_triplane->position(pli, static_cast<float>(slider->value()));
  ui->redraw();
}

void
TriPlaneUI::colorQuantity_cb(fltk::Choice *menu, TriPlaneUI *ui) {
  // char me[]="TriPlaneUI::colorQuantity_cb";
  unsigned int qi, pli;

  for (qi=colorQuantityUnknown+1;
       strcmp(menu->item()->label(), airEnumStr(colorQuantity, qi));
       qi++);
  for (pli=0; ui->_colorQuantityMenu[pli] != menu; pli++);
  if (!pli) {
    ui->_triplane->colorQuantity(qi);
    ui->_colorQuantityMenu[1]->value(menu->value());
    ui->_colorQuantityMenu[2]->value(menu->value());
    ui->_colorQuantityMenu[3]->value(menu->value());
    ui->_triplane->update();
  } else {
    ui->_triplane->plane[pli-1]->colorQuantity(qi);
    ui->_triplane->plane[pli-1]->update();
  }
  ui->redraw();
}

void
TriPlaneUI::seedAnisoThresh_cb(Slider *slider, TriPlaneUI *ui) {

  ui->_triplane->seedAnisoThresh(slider->value());
  ui->redraw();
}

void
TriPlaneUI::bright_cb(fltk::ValueSlider *val, TriPlaneUI *ui) {

  ui->_triplane->brightness(static_cast<float>(val->value()));
  ui->_triplane->update();
  ui->redraw();
}

void
TriPlaneUI::alphaMaskThreshold_cb(fltk::ValueSlider *val, TriPlaneUI *ui) {

  ui->_triplane->alphaMaskThreshold(val->value());
  ui->_triplane->update();
  ui->redraw();
}

void
TriPlaneUI::kernel_cb(fltk::Choice *menu, TriPlaneUI *ui) {
  // char me[]="TriPlaneUI::kernel_cb";
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
  ui->_triplane->kernel(gageKernel00, ui->_ksp);
  ui->_triplane->update();
  ui->redraw();
}

void
TriPlaneUI::_hest_cb(fltk::Button *, TriPlaneUI *ui) {

  AIR_UNUSED(ui);
  // kernels
  // sample factor
  // per plane: visible?, position, what
}

} /* namespace Deft */
