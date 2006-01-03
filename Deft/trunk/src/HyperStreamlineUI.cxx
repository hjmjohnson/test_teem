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

#include "HyperStreamlineUI.h"

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

HyperStreamlineUI::HyperStreamlineUI(HyperStreamline *hs, TensorGlyph *tglyph,
                                     TriPlane *tplane, Viewer *vw) {
  // char me[]="HyperStreamlineUI::HyperStreamlineUI";
  const unsigned int W = 400, H = 364, lineH = 20;
  unsigned int winy=0, incy=0;
  const char *defStr;
  int defVal;

  _hsline = hs;
  _tglyph = tglyph;
  _tplane = tplane;
  _viewer = vw;

  _ksp = nrrdKernelSpecNew();

  _win = new fltk::Window(W, H, "Deft::HyperStreamline");
  _win->begin();
  _win->resizable(_win);

  winy += 3;

  // ----------------------------------
  _visibleButton = new fltk::CheckButton(5, winy, W/4, incy=lineH, "Show");
  _visibleButton->callback((fltk::Callback*)visible_cb, this);
  _visibleButton->value(_hsline->visible());

  _computeButton = new fltk::Button(4*W/20, winy, W/6, lineH, "Initialize");
  _computeButton->callback((fltk::Callback*)compute_cb, this);

  _colorQuantityMenu = new fltk::Choice(20*W/40, winy, 9*W/40, lineH, "Color");
  for (unsigned int qi=colorQuantityUnknown+1; qi<colorQuantityLast; qi++) {
    _colorQuantityMenu->add(airEnumStr(colorQuantity, qi), this);
  }
  _colorQuantityMenu->callback((fltk::Callback*)(colorQuantity_cb), this);
  defStr = airEnumStr(colorQuantity, _hsline->colorQuantity());
  _colorQuantityMenu->value(((fltk::Group*)_colorQuantityMenu)
                            ->find(_colorQuantityMenu->find(defStr)));

  _wireframeButton = new fltk::CheckButton(17*W/20, winy, W/8,
                                           lineH, "Wire");
  _wireframeButton->callback((fltk::Callback*)wireframe_cb, this);
  _wireframeButton->value(_hsline->wireframe());

  winy += incy;
  winy += 3;

  _tubeDoButton = new fltk::CheckButton(5, winy, W/8, lineH, "Tube");
  _tubeDoButton->callback((fltk::Callback*)tubeDo_cb, this);

  _tubeFacetInput = new fltk::ValueInput(4*W/16, winy, W/11, lineH,
                                         "Facet");
  // _tubeFacetInput->when(fltk::WHEN_RELEASE);
  _tubeFacetInput->callback((fltk::Callback*)(tubeFacet_cb), this);
  _tubeFacetInput->range(3, 30);
  _tubeFacetInput->step(1);
  _tubeFacetInput->value(_hsline->tubeFacet());
  _tubeRadiusSlider = new Deft::Slider(0, winy, W, incy=40);
  _tubeRadiusSlider->align(fltk::ALIGN_LEFT);
  _tubeRadiusSlider->range(0.0, 2*_hsline->tubeRadius());
  _tubeRadiusSlider->fastUpdate(0);
  _tubeRadiusSlider->value(_hsline->tubeRadius());
  _tubeRadiusSlider->callback((fltk::Callback*)(tubeRadius_cb), this);

  winy += incy;
  winy += 3;

  _stepSlider = new Deft::Slider(0, winy, W, incy=40, "Step Size");
  _stepSlider->align(fltk::ALIGN_LEFT);
  _stepSlider->range(0.0, 1.0);
  _stepSlider->fastUpdate(0);
  _stepSlider->value(_hsline->step());
  _stepSlider->callback((fltk::Callback*)(step_cb), this);

  winy += incy;
  winy += 10;

  _stopAnisoButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                           "Anisotropy");
  _stopAnisoButton->value(_hsline->stopAnisoDo());
  _stopAnisoButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopAnisoTypeMenu = new fltk::Choice(5*W/20, winy, W/8, lineH);
  _stopAnisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_FA), this);
  _stopAnisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cl2), this);
  _stopAnisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cl1), this);
  _stopAnisoTypeMenu->callback((fltk::Callback*)_stopAnisoType_cb, this);
  // pray that someone didn't set a default stopAnisoType not on this menu
  defVal = _hsline->stopAnisoType();
  if (tenAniso_FA == defVal
      || tenAniso_Cl2 == defVal
      || tenAniso_Cl1 == defVal) {
    defStr = airEnumStr(tenAniso, defVal);
    _stopAnisoTypeMenu->value(((fltk::Group*)_stopAnisoTypeMenu)
                              ->find(_stopAnisoTypeMenu->find(defStr)));
  }
  _stopAnisoThresholdSlider = 
    new Deft::Slider(5, winy, W, incy=40);
  _stopAnisoThresholdSlider->align(fltk::ALIGN_LEFT);
  _stopAnisoThresholdSlider->range(0.0, 1.0);
  _stopAnisoThresholdSlider->fastUpdate(0);
  _stopAnisoThresholdSlider->value(_hsline->stopAnisoThreshold());
  _stopAnisoThresholdSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopHalfLengthButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                                "Half Length");
  _stopHalfLengthButton->value(_hsline->stopHalfLengthDo());
  _stopHalfLengthButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopHalfLengthSlider = new Deft::Slider(5, winy, W, incy=40);
  _stopHalfLengthSlider->align(fltk::ALIGN_LEFT);
  _stopHalfLengthSlider->range(0.01, 10);
  _stopHalfLengthSlider->step(0.001);
  _stopHalfLengthSlider->fastUpdate(0);
  _stopHalfLengthSlider->value(_hsline->stopHalfLength());
  _stopHalfLengthSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopHalfStepNumButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                                 "Half #Steps");
  _stopHalfStepNumButton->value(_hsline->stopHalfStepNumDo());
  _stopHalfStepNumButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopHalfStepNumSlider = new Deft::Slider(5, winy, W, incy=40);
  _stopHalfStepNumSlider->align(fltk::ALIGN_LEFT);
  _stopHalfStepNumSlider->range(1, 300);
  _stopHalfStepNumSlider->step(1);
  _stopHalfStepNumSlider->fastUpdate(0);
  _stopHalfStepNumSlider->valueUI(_hsline->stopHalfStepNum());
  _stopHalfStepNumSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopRadiusButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                            "Radius of Curvature");
  _stopRadiusButton->value(_hsline->stopRadiusDo());
  _stopRadiusButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopRadiusSlider = new Deft::Slider(5, winy, W, incy=40);
  _stopRadiusSlider->align(fltk::ALIGN_LEFT);
  _stopRadiusSlider->range(0.0, 1.0);
  _stopRadiusSlider->step(0.01);
  _stopRadiusSlider->fastUpdate(0);
  _stopRadiusSlider->value(_hsline->stopRadius());
  _stopRadiusSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopConfidenceButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                                "Confidence mask");
  _stopConfidenceButton->value(_hsline->stopConfidenceDo());
  _stopConfidenceButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopConfidenceSlider = new Deft::Slider(5, winy, W, incy=40);
  _stopConfidenceSlider->align(fltk::ALIGN_LEFT);
  _stopConfidenceSlider->range(0.0, 1.0);
  _stopConfidenceSlider->step(0.01);
  _stopConfidenceSlider->fastUpdate(0);
  _stopConfidenceSlider->value(_hsline->stopConfidence());
  _stopConfidenceSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _kernelMenu = new fltk::Choice(6*W/20, winy, W/5, incy=lineH, "kernel");
  for (unsigned int ki=kernelUnknown+1; ki<kernelLast; ki++) {
    _kernelMenu->add(kernelStr[ki], this);
  }
  _kernelMenu->callback((fltk::Callback*)(kernel_cb), this);
  _kernelMenu->value(((fltk::Group*)_kernelMenu)
                     ->find(_kernelMenu->find(kernelStr[kernelTent])));

  _integrationMenu = new fltk::Choice(15*W/20, winy, W/5, incy=lineH,
                                      "integration");
  for (unsigned int ii=tenFiberIntgUnknown+1;
       ii<tenFiberIntgLast; ii++) {
    _integrationMenu->add(airEnumStr(tenFiberIntg, ii), this);
  }
  _integrationMenu->callback((fltk::Callback*)(integration_cb), this);
  _integrationMenu->value(((fltk::Group*)_integrationMenu)
                     ->find(_integrationMenu
                            ->find(airEnumStr(tenFiberIntg,
                                              tenFiberIntgRK4))));

  // winy += incy;
  // fprintf(stderr, "!%s: winy = %d\n", me, winy);
  _win->end();

  _nseeds = nrrdNew();
}

HyperStreamlineUI::~HyperStreamlineUI() {
  
  nrrdNuke(_nseeds);
  nrrdKernelSpecNix(_ksp);
}

void
HyperStreamlineUI::show() {
  
  _win->show();
}

void
HyperStreamlineUI::redraw() {
  // char me[]="HyperStreamlineUI::redraw";
  
  _hsline->update();
  _viewer->redraw();
}

void
HyperStreamlineUI::visible_cb(fltk::CheckButton *but,
                              HyperStreamlineUI *ui) {
  ui->_hsline->visible(but->value());
  ui->_viewer->redraw();
}

void
HyperStreamlineUI::colorQuantity_cb(fltk::Choice *menu,
                                    HyperStreamlineUI *ui) {
  // char me[]="TriPlaneUI::colorQuantity_cb";
  unsigned int qi;

  for (qi=colorQuantityUnknown+1;
       strcmp(menu->item()->label(), airEnumStr(colorQuantity, qi));
       qi++);
  ui->_hsline->colorQuantity(qi);
  ui->redraw();
}

void
HyperStreamlineUI::compute_cb(fltk::Button *, HyperStreamlineUI *ui) {

  unsigned int seedNum = ui->_tglyph->glyphPositionsGet(ui->_nseeds);
  if (seedNum) {
    ui->_hsline->seedsSet(ui->_nseeds);
  } else {
    ui->_hsline->seedsSet(NULL);
  }
  ui->redraw();
}

void
HyperStreamlineUI::tubeDo_cb(fltk::CheckButton *but, HyperStreamlineUI *ui) {
  
  ui->_hsline->tubeDo(but->value());
  ui->redraw();
}

void
HyperStreamlineUI::tubeFacet_cb(fltk::ValueInput *val, HyperStreamlineUI *ui) {
  char me[]="HyperStreamlineUI::tubeFacet_cb";
  fprintf(stderr, "%s(%u): %g\n", me, ui->_hsline->tubeFacet(), val->value());
  if (val->value() - ui->_hsline->tubeFacet() <= 1.0) {
    ui->_hsline->tubeFacet(static_cast<unsigned int>(val->value()));
  } else {
    fprintf(stderr, "%s(%u): denying rapid increase to %g\n",
            me, ui->_hsline->tubeFacet(), val->value());
    val->value(ui->_hsline->tubeFacet());
  }
  ui->redraw();
}

void 
HyperStreamlineUI::tubeRadius_cb(Slider *slider, HyperStreamlineUI *ui) {

  ui->_hsline->tubeRadius(slider->value());
  ui->redraw();
}

void
HyperStreamlineUI::wireframe_cb(fltk::CheckButton *but, HyperStreamlineUI *ui) {

  ui->_hsline->wireframe(but->value());
  ui->_viewer->redraw();
}

void 
HyperStreamlineUI::step_cb(Slider *slider, HyperStreamlineUI *ui) {

  ui->_hsline->step(slider->value());
  ui->redraw();
}

void
HyperStreamlineUI::_stopButton_cb(fltk::CheckButton *but,
                                  HyperStreamlineUI *ui) {
  if (but == ui->_stopAnisoButton) {
    if (but->value()) {
      int aniso = airEnumVal(tenAniso,
                             ui->_stopAnisoTypeMenu->item()->label());
      ui->_hsline->stopAniso(aniso,
                             ui->_stopAnisoThresholdSlider->value());
      ui->_hsline->stopAnisoDo(true);
    } else {
      ui->_hsline->stopAnisoDo(false);
    }
  } else if (but == ui->_stopHalfLengthButton) {
    if (but->value()) {
      ui->_hsline->stopHalfLength(ui->_stopHalfLengthSlider->value());
      ui->_hsline->stopHalfLengthDo(true);
    } else {
      ui->_hsline->stopHalfLengthDo(false);
    }
  } else if (but == ui->_stopHalfStepNumButton) {
    if (but->value()) {
      ui->_hsline->stopHalfStepNum(ui->_stopHalfStepNumSlider->valueUI());
      ui->_hsline->stopHalfStepNumDo(true);
    } else {
      ui->_hsline->stopHalfStepNumDo(false);
    }
  } else if (but == ui->_stopConfidenceButton) {
    if (but->value()) {
      ui->_hsline->stopConfidence(ui->_stopConfidenceSlider->value());
      ui->_hsline->stopConfidenceDo(true);
    } else {
      ui->_hsline->stopConfidenceDo(false);
    }
  } else if (but == ui->_stopRadiusButton) {
    if (but->value()) {
      ui->_hsline->stopRadius(ui->_stopRadiusSlider->value());
      ui->_hsline->stopRadiusDo(true);
    } else {
      ui->_hsline->stopRadiusDo(false);
    }
  }
  ui->redraw();
}

void
HyperStreamlineUI::_stopAnisoType_cb(fltk::Choice *menu,
                                     HyperStreamlineUI *ui) {
  ui->_hsline->stopAniso(airEnumVal(tenAniso, menu->item()->label()),
                         ui->_hsline->stopAnisoThreshold());
  ui->redraw();
}

void
HyperStreamlineUI::_stopSlider_cb(Deft::Slider *slider, 
                                  HyperStreamlineUI *ui) {
  if (slider == ui->_stopAnisoThresholdSlider) {
    ui->_hsline->stopAniso(ui->_hsline->stopAnisoType(), slider->value());
  } else if (slider == ui->_stopHalfLengthSlider) {
    ui->_hsline->stopHalfLength(slider->value());
  } else if (slider == ui->_stopHalfStepNumSlider) {
    ui->_hsline->stopHalfStepNum(slider->valueUI());
  } else if (slider == ui->_stopConfidenceSlider) {
    ui->_hsline->stopConfidence(slider->value());
  } else if (slider == ui->_stopRadiusSlider) {
    ui->_hsline->stopRadius(slider->value());
  }
  ui->redraw();
}

void
HyperStreamlineUI::kernel_cb(fltk::Choice *menu, HyperStreamlineUI *ui) {
  // char me[]="HyperStreamlineUI::kernel_cb";
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
  ui->_hsline->kernel(ui->_ksp);
  ui->redraw();
}

void
HyperStreamlineUI::integration_cb(fltk::Choice *menu, HyperStreamlineUI *ui) {

  ui->_hsline->integration(airEnumVal(tenFiberIntg, menu->item()->label()));
  ui->redraw();
}

} /* namespace Deft */
