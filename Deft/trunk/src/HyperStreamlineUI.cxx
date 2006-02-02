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

char _deftTubesDrawnPerSecondStr[]="K tubes / second drawn";
char _deftLinesDrawnPerSecondStr[]="K polylines / second drawn";

HyperStreamlineUI::HyperStreamlineUI(HyperStreamline *hs, TensorGlyph *tglyph,
                                     TriPlane *tplane, Viewer *vw) {
  // char me[]="HyperStreamlineUI::HyperStreamlineUI";
  const unsigned int W=400, H=549, lineH=20;
  unsigned int winy=0, incy=0;
  const char *defStr;
  int defVal;

  _hsline.resize(1);
  _hsline[0] = hs;
  _tglyph = tglyph;
  _tplane = tplane;
  _viewer = vw;
  _hsline[0]->postDrawCallback((Callback*)(postDraw_cb), this);

  _ksp = nrrdKernelSpecNew();

  _win = new fltk::Window(W, H, "Deft::HyperStreamline");
  _win->begin();
  _win->resizable(_win);

  winy += 3;

  // ----------------------------------
  _computeButton = new fltk::Button(2*W/40, winy,
                                    W/6, incy=lineH, "Initialize");
  _computeButton->callback((fltk::Callback*)compute_cb, this);

  winy += incy;
  winy += 3;

  _visibleButton = new fltk::CheckButton(5, winy, W/4, incy=lineH, "Show");
  _visibleButton->callback((fltk::Callback*)visible_cb, this);
  _visibleButton->value(_hsline[0]->visible());

  _wireframeButton = new fltk::CheckButton(7*W/40, winy, W/8,
                                           lineH, "Wire");
  _wireframeButton->callback((fltk::Callback*)wireframe_cb, this);
  _wireframeButton->value(_hsline[0]->wireframe());

  _colorButton = new fltk::CheckButton(13*W/40, winy, W/8,
                                       lineH, "Color");
  _colorButton->callback((fltk::Callback*)color_cb, this);
  _colorButton->value(_hsline[0]->color());

  _colorQuantityMenu = new fltk::Choice(18*W/40, winy, 10*W/40, lineH);
  for (unsigned int qi=colorQuantityUnknown+1; qi<colorQuantityLast; qi++) {
    _colorQuantityMenu->add(airEnumStr(colorQuantity, qi), this);
  }
  _colorQuantityMenu->callback((fltk::Callback*)(colorQuantity_cb), this);
  defStr = airEnumStr(colorQuantity, _hsline[0]->colorQuantity());
  _colorQuantityMenu->value(((fltk::Group*)_colorQuantityMenu)
                            ->find(_colorQuantityMenu->find(defStr)));

  _stopColorDoButton = new fltk::CheckButton(30*W/40, winy, W/8,
                                           lineH, "Tips");
  _stopColorDoButton->callback((fltk::Callback*)stopColorDo_cb, this);
  _stopColorDoButton->value(_hsline[0]->stopColorDo());

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
  _tubeFacetInput->value(_hsline[0]->tubeFacet());
  _tubeRadiusSlider = new Deft::Slider(0, winy, W, incy=40);
  _tubeRadiusSlider->align(fltk::ALIGN_LEFT);
  _tubeRadiusSlider->range(0.0, 2*_hsline[0]->tubeRadius());
  _tubeRadiusSlider->fastUpdate(0);
  _tubeRadiusSlider->value(_hsline[0]->tubeRadius());
  _tubeRadiusSlider->callback((fltk::Callback*)(tubeRadius_cb), this);

  winy += incy;
  winy += 3;

  _stepSlider = new Deft::Slider(0, winy, W, incy=40, "Step Size");
  _stepSlider->align(fltk::ALIGN_LEFT);
  _stepSlider->range(0.0, 2*_hsline[0]->step());
  _stepSlider->fastUpdate(0);
  _stepSlider->value(_hsline[0]->step());
  _stepSlider->callback((fltk::Callback*)(step_cb), this);

  winy += incy;
  winy += 10;

  _stopAnisoButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                           "Anisotropy");
  _stopAnisoButton->value(_hsline[0]->stopAnisoDo());
  _stopAnisoButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopAnisoTypeMenu = new fltk::Choice(5*W/20, winy, W/8, lineH);
  _stopAnisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_FA), this);
  _stopAnisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cl2), this);
  _stopAnisoTypeMenu->add(airEnumStr(tenAniso, tenAniso_Cl1), this);
  _stopAnisoTypeMenu->callback((fltk::Callback*)_stopAnisoType_cb, this);
  // pray that someone didn't set a default stopAnisoType not on this menu
  defVal = _hsline[0]->stopAnisoType();
  if (tenAniso_FA == defVal
      || tenAniso_Cl2 == defVal
      || tenAniso_Cl1 == defVal) {
    defStr = airEnumStr(tenAniso, defVal);
    _stopAnisoTypeMenu->value(((fltk::Group*)_stopAnisoTypeMenu)
                              ->find(_stopAnisoTypeMenu->find(defStr)));
  }
  _stopAnisoThresholdSlider = new Deft::Slider(0, winy, W, incy=40);
  _stopAnisoThresholdSlider->align(fltk::ALIGN_LEFT);
  _stopAnisoThresholdSlider->range(0.0, 1.0);
  _stopAnisoThresholdSlider->color(0xFFFFFF00);
  _stopAnisoThresholdSlider->fastUpdate(0);
  _stopAnisoThresholdSlider->value(_hsline[0]->stopAnisoThreshold());
  _stopAnisoThresholdSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopHalfLengthButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                                "Half Length");
  _stopHalfLengthButton->value(_hsline[0]->stopHalfLengthDo());
  _stopHalfLengthButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopHalfLengthSlider = new Deft::Slider(0, winy, W, incy=40);
  _stopHalfLengthSlider->align(fltk::ALIGN_LEFT);
  _stopHalfLengthSlider->range(0.01, 10);
  _stopHalfLengthSlider->color(0xFF00FF00);
  _stopHalfLengthSlider->step(0.001);
  _stopHalfLengthSlider->fastUpdate(0);
  _stopHalfLengthSlider->value(_hsline[0]->stopHalfLength());
  _stopHalfLengthSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopHalfStepNumButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                                 "Half #Steps");
  _stopHalfStepNumButton->value(_hsline[0]->stopHalfStepNumDo());
  _stopHalfStepNumButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopHalfStepNumSlider = new Deft::Slider(0, winy, W, incy=40);
  _stopHalfStepNumSlider->align(fltk::ALIGN_LEFT);
  _stopHalfStepNumSlider->range(1, 300);
  _stopHalfStepNumSlider->color(0x00FFFF00);
  _stopHalfStepNumSlider->step(1);
  _stopHalfStepNumSlider->fastUpdate(0);
  _stopHalfStepNumSlider->valueUI(_hsline[0]->stopHalfStepNum());
  _stopHalfStepNumSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopRadiusButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                            "Radius of Curvature");
  _stopRadiusButton->value(_hsline[0]->stopRadiusDo());
  _stopRadiusButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopRadiusSlider = new Deft::Slider(0, winy, W, incy=40);
  _stopRadiusSlider->align(fltk::ALIGN_LEFT);
  _stopRadiusSlider->range(0.0, 5.0);
  _stopRadiusSlider->color(0xFFFF0000);
  _stopRadiusSlider->step(0.01);
  _stopRadiusSlider->fastUpdate(0);
  _stopRadiusSlider->value(_hsline[0]->stopRadius());
  _stopRadiusSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _stopConfidenceButton = new fltk::CheckButton(5, winy, W/10, lineH,
                                                "Confidence mask");
  _stopConfidenceButton->value(_hsline[0]->stopConfidenceDo());
  _stopConfidenceButton->callback((fltk::Callback*)_stopButton_cb, this);
  _stopConfidenceSlider = new Deft::Slider(0, winy, W, incy=40);
  _stopConfidenceSlider->align(fltk::ALIGN_LEFT);
  _stopConfidenceSlider->range(0.0, 1.0);
  _stopConfidenceSlider->color(0x8F8F8F00);
  _stopConfidenceSlider->step(0.01);
  _stopConfidenceSlider->fastUpdate(0);
  _stopConfidenceSlider->value(_hsline[0]->stopConfidence());
  _stopConfidenceSlider->callback((fltk::Callback*)_stopSlider_cb, this);

  winy += incy;
  winy += 5;

  _kernelMenu = new fltk::Choice(6*W/50, winy, 8*W/50, incy=lineH, "Kernel");
  for (unsigned int ki=kernelUnknown+1; ki<kernelLast; ki++) {
    _kernelMenu->add(kernelStr[ki], this);
  }
  _kernelMenu->callback((fltk::Callback*)(kernel_cb), this);
  _kernelMenu->value(((fltk::Group*)_kernelMenu)
                     ->find(_kernelMenu->find(kernelStr[kernelTent])));

  _integrationMenu = new fltk::Choice(18*W/50, winy, 10*W/50, incy=lineH,
                                      "Intg");
  for (unsigned int ii=tenFiberIntgUnknown+1;
       ii<tenFiberIntgLast; ii++) {
    _integrationMenu->add(airEnumStr(tenFiberIntg, ii), this);
  }
  _integrationMenu->callback((fltk::Callback*)(integration_cb), this);
  _integrationMenu->value(((fltk::Group*)_integrationMenu)
                          ->find(_integrationMenu
                                 ->find(airEnumStr(tenFiberIntg,
                                                   _hsline[0]->integration()))));

  _brightSlider = new fltk::ValueSlider(33*W/50, winy, 16*W/50, incy=lineH,
                                        "Bright");
  _brightSlider->callback((fltk::Callback*)(bright_cb), this);
  _brightSlider->range(0.3, 1.9);
  _brightSlider->step(0.01);
  _brightSlider->value(_hsline[0]->brightness());
  _brightSlider->align(fltk::ALIGN_LEFT);
  _brightSlider->box(fltk::THIN_DOWN_BOX);
  _brightSlider->color(fltk::GRAY40);

  winy += incy;
  winy += 10;

  _pathsNumOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                     "# paths");
  _pathsNumOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
  _vertsNumOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                     "# K vertices");
  _vertsNumOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
  _pathsPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                           "paths / second computed");
  _pathsPerSecondOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
  _vertsPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                           "K vertices / second computed");
  _vertsPerSecondOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
  _tubesPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                           "tubes / second wrapped");
  _tubesPerSecondOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
  _drawnPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                           (_hsline[0]->tubeDo()
                                            ? _deftTubesDrawnPerSecondStr
                                            : _deftLinesDrawnPerSecondStr));
  _drawnPerSecondOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
  _vertsDrawnPerSecondOutput = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                                "M vertices / second drawn");
  _vertsDrawnPerSecondOutput->box(fltk::NO_BOX);

  winy += incy;
  winy += 2;
  
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
  char buff[AIR_STRLEN_MED];
  // char me[]="HyperStreamlineUI::redraw";
  
  _hsline[0]->update();
  _viewer->redraw();

  sprintf(buff, "%u", _hsline[0]->fiberNum());
  _pathsNumOutput->value(buff);
  sprintf(buff, "%g", _hsline[0]->fiberVertexNum()/1000.0);
  _vertsNumOutput->value(buff);
  double computeTime = (_hsline[0]->fiberAllocatedTime()
                        + _hsline[0]->fiberAllocatedTime()
                        + _hsline[0]->fiberGeometryTime()
                        + _hsline[0]->fiberColorTime()
                        + _hsline[0]->fiberStopColorTime());
  sprintf(buff, "%g", _hsline[0]->fiberNum()/computeTime);
  _pathsPerSecondOutput->value(buff);
  sprintf(buff, "%g", _hsline[0]->fiberVertexNum()/(computeTime*1000.0));
  _vertsPerSecondOutput->value(buff);
  if (_hsline[0]->tubeDo()) {
    double tubeTime = (_hsline[0]->tubeAllocatedTime()
                       + _hsline[0]->tubeGeometryTime()
                       + _hsline[0]->tubeColorTime());
    sprintf(buff, "%g", _hsline[0]->fiberNum()/tubeTime);
    _tubesPerSecondOutput->value(buff);
  } else {
    _tubesPerSecondOutput->value("");
  }
}

void
HyperStreamlineUI::visible_cb(fltk::CheckButton *but,
                              HyperStreamlineUI *ui) {
  char buff[AIR_STRLEN_MED];

  if (!ui->_hsline[0]->visible() && ui->_hsline[0]->fiberNum()) {
    sprintf(buff, "%g",
            ui->_hsline[0]->fiberNum()/(1000*ui->_hsline[0]->drawTime()));
    ui->_drawnPerSecondOutput->value(buff);
  } else {
    ui->_drawnPerSecondOutput->value("");
  }
  ui->_hsline[0]->visible(but->value());
  ui->redraw();
}

void
HyperStreamlineUI::colorQuantity_cb(fltk::Choice *menu,
                                    HyperStreamlineUI *ui) {
  // char me[]="TriPlaneUI::colorQuantity_cb";
  unsigned int qi;

  for (qi=colorQuantityUnknown+1;
       strcmp(menu->item()->label(), airEnumStr(colorQuantity, qi));
       qi++);
  ui->_hsline[0]->colorQuantity(qi);
  ui->redraw();
}

void
HyperStreamlineUI::compute_cb(fltk::Button *, HyperStreamlineUI *ui) {

  unsigned int seedNum = ui->_tglyph->glyphPositionsGet(ui->_nseeds);
  if (seedNum) {
    ui->_hsline[0]->seedsSet(ui->_nseeds);
  } else {
    ui->_hsline[0]->seedsSet(NULL);
  }
  ui->redraw();
}

void
HyperStreamlineUI::tubeDo_cb(fltk::CheckButton *but, HyperStreamlineUI *ui) {
  
  ui->_hsline[0]->tubeDo(but->value());
  ui->_drawnPerSecondOutput->label(but->value()
                                   ? _deftTubesDrawnPerSecondStr
                                   : _deftLinesDrawnPerSecondStr);
  ui->_drawnPerSecondOutput->relayout();
  ui->redraw();
}

void
HyperStreamlineUI::tubeFacet_cb(fltk::ValueInput *val, HyperStreamlineUI *ui) {
  char me[]="HyperStreamlineUI::tubeFacet_cb";
  fprintf(stderr, "%s(%u): %g\n", me,
          ui->_hsline[0]->tubeFacet(), val->value());
  if (val->value() - ui->_hsline[0]->tubeFacet() <= 1.0) {
    ui->_hsline[0]->tubeFacet(static_cast<unsigned int>(val->value()));
  } else {
    fprintf(stderr, "%s(%u): denying rapid increase to %g\n",
            me, ui->_hsline[0]->tubeFacet(), val->value());
    val->value(ui->_hsline[0]->tubeFacet());
  }
  ui->redraw();
}

void 
HyperStreamlineUI::tubeRadius_cb(Slider *slider, HyperStreamlineUI *ui) {

  ui->_hsline[0]->tubeRadius(slider->value());
  ui->redraw();
}

void
HyperStreamlineUI::stopColorDo_cb(fltk::CheckButton *but,
                                  HyperStreamlineUI *ui) {

  ui->_hsline[0]->stopColorDo(but->value());
  ui->redraw();
}

void
HyperStreamlineUI::wireframe_cb(fltk::CheckButton *but,
                                HyperStreamlineUI *ui) {

  ui->_hsline[0]->wireframe(but->value());
  ui->redraw();
}

void
HyperStreamlineUI::color_cb(fltk::CheckButton *but,
                            HyperStreamlineUI *ui) {
  /*
  fprintf(stderr, "!%s: _hsline[0]->color(%s)\n", 
          "HyperStreamlineUI::color_cb", 
          but->value() ? "true" : "false");
  */
  ui->_hsline[0]->color(but->value());
  dynamic_cast<PolyProbe*>(ui->_hsline[0])->update(false);
  ui->redraw();
}

void 
HyperStreamlineUI::step_cb(Slider *slider, HyperStreamlineUI *ui) {

  ui->_hsline[0]->step(slider->value());
  ui->redraw();
}

void
HyperStreamlineUI::_stopButton_cb(fltk::CheckButton *but,
                                  HyperStreamlineUI *ui) {
  if (but == ui->_stopAnisoButton) {
    if (but->value()) {
      int aniso = airEnumVal(tenAniso,
                             ui->_stopAnisoTypeMenu->item()->label());
      ui->_hsline[0]->stopAniso(aniso,
                             ui->_stopAnisoThresholdSlider->value());
      ui->_hsline[0]->stopAnisoDo(true);
    } else {
      ui->_hsline[0]->stopAnisoDo(false);
    }
  } else if (but == ui->_stopHalfLengthButton) {
    if (but->value()) {
      ui->_hsline[0]->stopHalfLength(ui->_stopHalfLengthSlider->value());
      ui->_hsline[0]->stopHalfLengthDo(true);
    } else {
      ui->_hsline[0]->stopHalfLengthDo(false);
    }
  } else if (but == ui->_stopHalfStepNumButton) {
    if (but->value()) {
      ui->_hsline[0]->stopHalfStepNum(ui->_stopHalfStepNumSlider->valueUI());
      ui->_hsline[0]->stopHalfStepNumDo(true);
    } else {
      ui->_hsline[0]->stopHalfStepNumDo(false);
    }
  } else if (but == ui->_stopConfidenceButton) {
    if (but->value()) {
      ui->_hsline[0]->stopConfidence(ui->_stopConfidenceSlider->value());
      ui->_hsline[0]->stopConfidenceDo(true);
    } else {
      ui->_hsline[0]->stopConfidenceDo(false);
    }
  } else if (but == ui->_stopRadiusButton) {
    if (but->value()) {
      ui->_hsline[0]->stopRadius(ui->_stopRadiusSlider->value());
      ui->_hsline[0]->stopRadiusDo(true);
    } else {
      ui->_hsline[0]->stopRadiusDo(false);
    }
  }
  ui->redraw();
}

void
HyperStreamlineUI::_stopAnisoType_cb(fltk::Choice *menu,
                                     HyperStreamlineUI *ui) {
  ui->_hsline[0]->stopAniso(airEnumVal(tenAniso, menu->item()->label()),
                         ui->_hsline[0]->stopAnisoThreshold());
  ui->redraw();
}

void
HyperStreamlineUI::_stopSlider_cb(Deft::Slider *slider, 
                                  HyperStreamlineUI *ui) {
  if (slider == ui->_stopAnisoThresholdSlider) {
    if (ui->_hsline[0]->stopAnisoDo()) {
      ui->_hsline[0]->stopAniso(ui->_hsline[0]->stopAnisoType(),
                                slider->value());
    }
  } else if (slider == ui->_stopHalfLengthSlider) {
    if (ui->_hsline[0]->stopHalfLengthDo()) {
      ui->_hsline[0]->stopHalfLength(slider->value());
    }
  } else if (slider == ui->_stopHalfStepNumSlider) {
    if (ui->_hsline[0]->stopHalfStepNumDo()) {
      ui->_hsline[0]->stopHalfStepNum(slider->valueUI());
    }
  } else if (slider == ui->_stopConfidenceSlider) {
    if (ui->_hsline[0]->stopConfidenceDo()) {
      ui->_hsline[0]->stopConfidence(slider->value());
    }
  } else if (slider == ui->_stopRadiusSlider) {
    if (ui->_hsline[0]->stopRadiusDo()) {
      ui->_hsline[0]->stopRadius(slider->value());
    }
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
  ui->_hsline[0]->kernel(ui->_ksp);
  ui->redraw();
}

void
HyperStreamlineUI::integration_cb(fltk::Choice *menu, HyperStreamlineUI *ui) {

  ui->_hsline[0]->integration(airEnumVal(tenFiberIntg, menu->item()->label()));
  ui->redraw();
}

void
HyperStreamlineUI::bright_cb(fltk::ValueSlider *val, HyperStreamlineUI *ui) {

  ui->_hsline[0]->brightness(static_cast<float>(val->value()));
  dynamic_cast<PolyProbe*>(ui->_hsline[0])->update(false);
  ui->redraw();
}

void
HyperStreamlineUI::postDraw_cb(HyperStreamline *hsline,
                               HyperStreamlineUI *ui) {
  char buff[AIR_STRLEN_MED];

  if (hsline->visible() && hsline->fiberNum()) {
    sprintf(buff, "%g",
            ui->_hsline[0]->fiberNum()/(1000*ui->_hsline[0]->drawTime()));
    ui->_drawnPerSecondOutput->value(buff);
    sprintf(buff, "%g",
            (ui->_hsline[0]->fiberVertexNum()
             / (1000000*ui->_hsline[0]->drawTime())));
    ui->_vertsDrawnPerSecondOutput->value(buff);
  } else {
    ui->_drawnPerSecondOutput->value("");
    ui->_vertsDrawnPerSecondOutput->value("");
  }
}


} /* namespace Deft */
